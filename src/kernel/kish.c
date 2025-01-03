#include <isr.h>
#include <irq.h>
#include <idt.h>
#include <vga.h>
#include <keyboard.h>
#include <alloc.h>
#include <time.h>
#include <fpu.h>
#include <pcspkr.h>
#include <string.h>
#include <ata.h>
#include <multiboot.h>
#include <fat.h>
#include <vfs.h>

// Very simple CLI shell built into the kernel until I get filesystem and ABI support

extern void LittleGame();
extern void reboot();
extern void shutdown();
extern disk_t* disks[MAX_DRIVES];
extern fat_disk_t* fatdisks[MAX_DRIVES];

extern int32 ExecuteProgram(void* program);

// Execute a syscall to see what happens
void syscall(){
    asm volatile("int %0" :: "Nd" (SYSCALL_INT));
}

fat_disk_t* supportedDisk;


void fseek(){
    for(int i = 0; i < MAX_DRIVES; i++){
        if(fatdisks[i] != NULL){
            //printk("Disk %d: ", i);
            if(fatdisks[i]->fstype == FS_FAT32){
                supportedDisk = fatdisks[i];
            }
        }
    }

    FAT_cluster_t* rootDir = ReadRootDirectory(supportedDisk);
    FAT_cluster_t* current = rootDir;
    fat_disk_t* fatdisk = supportedDisk;

    printk("Files in the root directory:\n");
    while(true){
        if(current == NULL){
            break;
        }
        size_t bytesPerCluster = fatdisk->paramBlock->sectorsPerCluster * fatdisk->paramBlock->bytesPerSector;
        size_t numEntries = bytesPerCluster / (sizeof(fat_entry_t));

        uint64 offset = 0;
        for(size_t i = 0; i < numEntries; i++){
            fat_entry_t* file = (fat_entry_t* )(current->buffer + offset);
            if(file->name[0] == 0){
                // End of directory entries
                break;
            }
            if(file->name[0] == DELETED){
                if(file->attributes == 0x0F){
                    offset += sizeof(lfn_entry_t);
                }else{
                    offset += sizeof(fat_entry_t);
                }
                continue;
            }
            if(file->attributes == 0x0F){
                // The file is an LFN file. Process it like a regular file for now for debugging purposes.
                WriteStrSize(&file->name, 11);
                char* ptr = (char*)file;
                for(int j = 0; j < sizeof(fat_entry_t); j++){
                    printk("0x%x ", (uint32)((*(ptr + j)) & 0x000000FFU));
                }
                lfn_entry_t* lfn = (lfn_entry_t*)((void*)file);
                file = &lfn->file;
                if(file->name[0] == DELETED){
                    //printk("Deleted!\n");
                    offset += sizeof(fat_entry_t);          // If the entry is deleted, it's there, so we have to skip it.
                    continue;
                }
                // Just in case the 8.3 filename has 0 characters
                printk("LFN file found!\n");

                WriteStrSize(lfn->file.name, 11);
                printk("\n");
                offset += sizeof(lfn_entry_t);
            }else{
                offset += sizeof(fat_entry_t);
            }
            if(file->name[0] != DELETED){
                // The file we're looking for was found and not deleted
                file_t* foundFile = (file_t*)alloc(sizeof(file_t));

                // Copy the file name to the VFS file
                foundFile->name = (char*)alloc(12);
                memcpy(foundFile->name, file->name, 11);
                foundFile->name[11] = '\0';
                for(int i = 0; i < 11; i++){
                    if(foundFile->name[i] == ' ' && i == 7){
                        foundFile->name[i] = '.';
                    }
                }

                uint32 firstFileCluster = file->firstClusterLow | (file->firstClusterHigh << 16);
                uint32 allClusters = file->fileSize / (fatdisk->paramBlock->sectorsPerCluster * fatdisk->paramBlock->bytesPerSector);       // How many clusters the file takes up

                uint64 clusterLba = firstFileCluster * fatdisk->paramBlock->sectorsPerCluster + fatdisk->firstDataSector;
                printk(foundFile->name);
                printk("\n");
                dealloc(foundFile->name);
                dealloc(foundFile);
            }
        }

        current = current->next;
        offset = 0;
    }
    ClearRootDirectory(rootDir);
    printk("End of directory.\n");
}

// The shell commands
void ProcessCommand(const char* cmd, mboot_info_t* multibootInfo){
    if(strlen(cmd) == 0){
        return;
    }

    if(strcmp(cmd, "game")){
        LittleGame();

    }else if(strcmp(cmd, "clear")){
        ClearTerminal();

    }else if(strcmp(cmd, "hi")){
        printk("Hello!\n");

    }else if(strcmp(cmd, "reboot")){
        reboot();

    }else if(strcmp(cmd, "shutdown")){
        shutdown();

    }else if(strcmp(cmd, "systest")){
        syscall();

    }else if(strcmp(cmd, "help")){
        printk("game: runs a small game\n");
        printk("hi: say hello!\n");
        printk("systest: execute a system call\n");
        printk("help: view this screen\n");
        printk("dskchk: scans the system for PATA disks\n");
        printk("memsize: get the total system RAM in bytes\n");
        printk("dir: tests the FAT driver by looking for a file on the disk\n");
        printk("clear: clears the terminal screen\n");
        printk("fault: intentionally cause an exception (debug)\n");
        printk("reboot: reboots the machine\n");
        printk("shutdown: shuts down the computer (QEMU/Bochs only)\n");
        printk("lex: loads and executes a program from the disk\n");
        printk("pginfo: show the total number of pages in the system\n");

    }else if(strcmp(cmd, "dskchk")){
        for(int i = 0; i < MAX_DRIVES; i++){
            disk_t* disk = disks[i];
            if(disk != NULL){
                printk("Disk found! Number: %d\n", disk->driveNum);
                printk("Disk Type: %d ", disk->type);
                if(disk->type == PATADISK){
                    printk("(PATA)\n");
                    printk("Disk size in sectors: %u\n", disk->size);
                }else if(disk->type == PATAPIDISK){
                    printk("(PATAPI)\n");
                    if(disk->populated){
                        printk("Populated: YES\n");
                        printk("Disk size in sectors: %u\n", disk->size);
                    }else if(!disk->populated){
                        printk("Populated: NO\n");
                    }
                }else{
                    printk("(UNKNOWN)\n");
                }
                printk("Addressing: %d ", disk->addressing);
                if(disk->addressing == CHS_ONLY){
                    printk("(CHS only)\n");
                }else if(disk->addressing == LBA28){
                    printk("(28-bit LBA)\n");
                }else if(disk->addressing == LBA48){
                    printk("(48-bit LBA)\n");
                }
                printk("\n");
            }
        }

    }else if(strcmp(cmd, "fault")){
        // Intentionally cause an exception to test interrupts and exceptions (these are very important)
        asm volatile("int $0x08");

    }else if(strcmp(cmd, "memsize")){
        printk("Total memory: %u MiB\n", (GetTotalMemory() / 1024) / 1024);

    }else if (strcmp(cmd, "dir")){
        fseek();

    }else if(strncmp(cmd, "lex", 3)){
        // Expects a file name after the command as well as a space between them
        for(int i = 0; i < MAX_DRIVES; i++){
            if(fatdisks[i] != NULL){
                //printk("Disk %d: ", i);
                if(fatdisks[i]->fstype == FS_FAT32){
                    supportedDisk = fatdisks[i];
                }
            }
        }
        file_t* program = SeekFile(supportedDisk, &cmd[4]);
        if(program == NULL){
            printk("File not found!\n");
            return;
        }
        ExecuteProgram(program->data);
        dealloc(program->data);
        dealloc(program->name);
        dealloc(program);
    }else if(strcmp(cmd, "pginfo")){
        printk("Total pages: %u\n", GetPages());
    }else{
        printk("Invalid Command!\n");
    }
}

int CliHandler(mboot_info_t* multibootInfo){
    printk("Thanks for the GRUB!\n");
    printk("Kernel-Integrated Shell (KISh)\n");
    printk("Enter \"help\" into the console for a list of commands.\n");
    // Allocate 1000 bytes for a command. That means a max of 1000 characters. Should be more than enough.
    char* command = (char*)alloc(1000);
    memset(command, 0, 1000);

    int index = 0;

    printk("KISh> ");
    while(true){
        uint8 lastKey = GetKey();
        if(lastKey != 0){
            switch (lastKey)
            {
                case '\b':
                    if(GetX() > 6){
                        index--;
                        command[index] = 0;
                        WriteStrSize(&lastKey, 1);
                    }
                    break;

                case '\n':
                    printk("\n");
                    ProcessCommand(command, multibootInfo);
                    memset(command, 0, 1000);
                    index = 0;
                    printk("KISh> ");
                    break;
                
                default:
                    if(index < 1000){
                        // Just in case allocation wasn't enough, protect from a buffer overflow
                        command[index] = lastKey;
                        index++;
                        WriteStrSize(&lastKey, 1);
                    }
                    break;
            }
        }
    }

    dealloc(command);

    return 1;
}