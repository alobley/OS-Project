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
    char* potentialPtr = (char*)0x40E;
    // If no real mode pointer, check the BIOS area
    char* startPtr = (char*)0x000E0000;
    char* endPtr = (char*)0x000FFFFF;

    char* toFind = "RSD PTR ";

    if(strncmp((char*)potentialPtr, toFind, 8) == 0){
        if(DoChecksum((char*)potentialPtr, sizeof(RSDP_V1_t))){
            // Found it
            if(((RSDP_V1_t*)potentialPtr)->revision == 0){
                acpiInfo.rsdp.rsdpV1 = (RSDP_V1_t*)potentialPtr;
                acpiInfo.version = acpiInfo.rsdp.rsdpV1->revision;
            }else{
                acpiInfo.rsdp.rsdpV2 = (RSDP_V2_t*)potentialPtr;
                acpiInfo.version = acpiInfo.rsdp.rsdpV2->revision;
            }
            acpiInfo.exists = true;
            return;
        }
    }

    // Search the BIOS data area for it if not found
    for(char* i = startPtr; i < endPtr; i += 16){
        if(strncmp(i, toFind, 8) == 0){
            if(DoChecksum(i, sizeof(RSDP_V1_t))){
                // Found it
                if(acpiInfo.rsdp.rsdpV1->revision == 0){
                    acpiInfo.rsdp.rsdpV1 = (RSDP_V1_t*)i;
                    acpiInfo.version = acpiInfo.rsdp.rsdpV1->revision;
                }else{
                    acpiInfo.rsdp.rsdpV2 = (RSDP_V2_t*)i;
                    acpiInfo.version = acpiInfo.rsdp.rsdpV2->revision;
                }
                acpiInfo.exists = true;
                return;
            }
        }
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
    if(acpiInfo.fadt->header.revision > 1){
        // Check the boot architecture flags
        return acpiInfo.fadt->bootArchFlags & 0x02;
    }
    // On version 1, it is always enabled
    return true;
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