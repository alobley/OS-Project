#include <acpi.h>
#include <string.h>
#include <console.h>
#include <time.h>

ACPIInfo_t acpiInfo = {0};

bool DoChecksum(char* table, uint32_t length){
    uint32_t sum = 0;
    for(uint32_t i = 0; i < length; i++){
        sum += table[i];
    }

    return sum % 256 == 0;
}

// Locate the RSDP table using brute force
void GetRSDP(){
    // Check if the real mode pointer exists
    uint16_t* potentialPtr = (uint16_t*)0x40E;
    // No real mode pointer, check the BIOS area
    char* startPtr = (char*)0x000E0000;
    char* endPtr = (char*)0x000FFFFF;

    char* toFind = "RSD PTR ";

    // Search painstakingly for the RSDP
    for(char* i = startPtr; i < endPtr; i += 16){
        if(strncmp(i, toFind, 8)){
            // Found it

            if(DoChecksum(i, sizeof(RSDP_V1_t)) == false){
                // Checksum failed
                WriteString("RSDP checksum failed.\n");
                continue;
            }

            if(((RSDP_V1_t*)i)->revision == 0){
                acpiInfo.rsdp.rsdpV1 = (RSDP_V1_t*)i;
                acpiInfo.version = 0;
            }else if(((RSDP_V1_t*)i)->revision == 2){
                // Check the extended checksum

                if(DoChecksum(i, sizeof(RSDP_V2_t)) == false){
                    // Checksum failed
                    WriteString("RSDP checksum failed.\n");
                    continue;
                }

                acpiInfo.rsdp.rsdpV2 = (RSDP_V2_t*)i;
                acpiInfo.version = 2;
            }else{
                // Unsupported revision
                WriteString("Unsupported RSDP revision.\n");
                continue;
            }
            acpiInfo.exists = true;
            return;
        }
    }

    // If no pointer found, try looking for the real mode pointer
    uint32_t ptr = *potentialPtr;
    ptr = ptr << 4;

    if(DoChecksum((char*)ptr, sizeof(RSDP_V1_t)) == false){
        // Checksum failed. There is no ACPI table.
        WriteString("RSDP checksum failed. There is no ACPI table.\n");
        acpiInfo.exists = false;
        return;
    }
    if(((RSDP_V1_t*)ptr)->revision == 0){
        acpiInfo.rsdp.rsdpV1 = (RSDP_V1_t*)ptr;
    }else if(((RSDP_V1_t*)ptr)->revision == 2){
        // Check the extended checksum
        if(DoChecksum((char*)ptr, sizeof(RSDP_V2_t)) == false){
            // Checksum failed
            WriteString("RSDP checksum failed. There is no ACPI table.\n");
            acpiInfo.exists = false;
            return;
        }

        acpiInfo.rsdp.rsdpV2 = (RSDP_V2_t*)ptr;
        acpiInfo.version = 2;
    }else{
        // Unsupported revision
        printf("Unsupported RSDP revision.\n");
        acpiInfo.exists = false;
        return;
    }
}

// This is all we need to worry about in 32-bit systems, but having both is a plus
void* FindFadtVer1(RSDT_t* rsdt){
    uint32_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;

    for(uint32_t i = 0; i < entries; i++){
        SDTHeader_t* header = (SDTHeader_t*)rsdt->tablePointers[i];
        if(strncmp(header->signature, "FACP", 4)){
            return (void*)header;
        }
    }

    return NULL;
}

void* FindFadtVer2(XSDT_t* xsdt){
    uint32_t entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;

    for(uint32_t i = 0; i < entries; i++){
        SDTHeader_t* header = (SDTHeader_t*)((uintptr_t)xsdt->tablePointers[i]);
        if(strncmp(header->signature, "FACP", 4)){
            return (void*)header;
        }
    }

    return NULL;
}


// Print out the RSDP information. Call before any other stuff is done.
void InitializeACPI(){
    GetRSDP();
    if(acpiInfo.rsdp.rsdpV1 != NULL && acpiInfo.version == 0){
        acpiInfo.rsdt.rsdt = (RSDT_t*)acpiInfo.rsdp.rsdpV1->rsdtAddress;
        acpiInfo.fadt = FindFadtVer1(acpiInfo.rsdt.rsdt);
    }else if(acpiInfo.rsdp.rsdpV2 != NULL && acpiInfo.version == 2){
        acpiInfo.rsdt.xsdt = (XSDT_t*)((uintptr_t)acpiInfo.rsdp.rsdpV2->xsdtAddress);
        acpiInfo.fadt = FindFadtVer2((XSDT_t*)acpiInfo.rsdt.xsdt);
    }else{
        WriteString("No valid RSDP found. ACPI is not supported.\n");
        acpiInfo.exists = false;
        return;
    }

    // Update the version to the specific one outlined in the FADT
    acpiInfo.version = acpiInfo.fadt->header.revision;

    // Enable ACPI mode if not already enabled
    if(acpiInfo.fadt->smiCmd != 0 && acpiInfo.fadt->acpiEnable != 0 && acpiInfo.fadt->acpiDisable != 0 && (inw(acpiInfo.fadt->pm1aCtrlBlk) & 0x01) == 0){
        outb(acpiInfo.fadt->smiCmd, acpiInfo.fadt->acpiEnable);
        sleep(3000);    // Wait 3 seconds
        while((inw(acpiInfo.fadt->pm1aCtrlBlk) & 0x01) == 0);
    }
}

bool PS2ControllerExists(){
    // Check the 8042 controller flag
    if(acpiInfo.fadt == NULL){
        return false;
    }
    if(acpiInfo.version > 1){
        if((acpiInfo.fadt->bootArchFlags & 0x0002) == 0x0002){
        return true;
        }else{
            return false;
        }
    }else{
        // On version 1, it is always enabled
        return true;
    }
}

void AcpiShutdown(){
    
}

void AcpiReboot(){
    if(acpiInfo.fadt->header.revision > 1){
        outb(acpiInfo.fadt->resetReg.address, acpiInfo.fadt->resetValue);
    }else{
        outw(0x64, 0xFE);
    }
}