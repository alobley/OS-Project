#include <kernel.h>

// Very simple CLI shell built into the kernel until I get filesystem and ABI support

extern void LittleGame();
extern disk_t* disks[MAX_DRIVES];
extern fat_disk_t* fatdisks[MAX_DRIVES];

bool OnOtherDisk = false;

// Execute a syscall to see what happens
void syscall(){
    eax(0);
    asm volatile("int %0" :: "Nd" (SYSCALL_INT));
}

char* workingDir = ROOT_MNT;

void dir(){
    vfs_disk_t* root = disks[ROOT_INDEX];
    if(root == NULL){
        printk("No root disk found!\n");
        return;
    }
    directory_t* rootDir = root->mountDir;
    if(rootDir == NULL){
        printk("Invalid or corrupt root directory!\n");
        return;
    }
    directory_entry_t* current = rootDir->firstFile;
    if(current == NULL){
        printk("No files found!\n");
        return;
    }
    while(current != NULL){
        //printk("0x%x\n0x%x\n", current, &current->name);
        printk(&current->name);
        printk("  ");
        current = current->next;
    }
    printk("\n");
}

// The shell commands
void ProcessCommand(const char* cmd, mboot_info_t* multibootInfo){
    if(strlen(cmd) == 0){
        return;
    }

    // There has got to be a way to optimize this
    if(strcmp(cmd, "game")){
        LittleGame();

    }else if(strcmp(cmd, "clear")){
        ClearTerminal();

    }else if(strcmp(cmd, "hi")){
        printk("Hello!\n");

    }else if(strcmp(cmd, "reboot")){
        AcpiReboot();
        if(PS2ControllerExists()){
            // Fallback if ACPI reboot fails (QEMU has ACPI V1, so this must be used in it)
            reboot();
        }

    }else if(strcmp(cmd, "shutdown")){
        printk("Trying ACPI shutdown...\n");
        AcpiShutdown();

        printk("ACPI shutdown failed. Trying QEMU/Bochs shutdown...\n");
        shutdown();

        printk("Shutdown failed. Please press the computer's power button.\n");

    }else if(strcmp(cmd, "systest")){
        syscall();

    }else if(strcmp(cmd, "help")){
        printk("game: runs a small game\n");
        printk("hi: say hello!\n");
        printk("systest: execute a system call\n");
        printk("help: view this screen\n");
        printk("dskchk: scans the system for PATA disks\n");
        printk("meminfo: Get memory information\n");
        printk("dir: prints all the entries in the working directory\n");
        printk("clear: clears the terminal screen\n");
        printk("fault: intentionally cause an exception (debug)\n");
        printk("reboot: reboots the machine\n");
        printk("shutdown: shuts down the computer\n");
        printk("lex: loads and executes a program from the disk\n");
        printk("pginfo: show the total number of pages in the system\n");

    }else if(strcmp(cmd, "dskchk")){
        for(int i = 0; i < MAX_DRIVES; i++){
            disk_t* disk = IdentifyDisk(i);
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
            dealloc(disk);
        }

    }else if(strcmp(cmd, "fault")){
        // Intentionally cause an exception to test interrupts and exceptions (these are very important)
        asm volatile("int $0x08");

    }else if(strcmp(cmd, "meminfo")){
        printk("Total memory: %u MiB\n", (GetTotalMemory() / 1024) / 1024);
        uint32 usedMem = GetPages() * PAGE_SIZE;
        if(usedMem == 0){
            printk("Memory error!\n");
            return;
        }
        usedMem /= 1024;
        uint32 remainder = usedMem % 1024;
        usedMem /= 1024;
        remainder = (remainder * 10) / 1024;
        printk("Used memory: %u.%u MiB\n", usedMem, remainder);

    }else if (strcmp(cmd, "dir")){
        dir();

    }else if(strncmp(cmd, "lex", 3)){
        // Expects a file path after the command as well as a space between them
        if(strlen(cmd) < 5){
            printk("No file specified!\n");
            return;
        }
        file_t* program = GetFile(&cmd[4]);
        if(program == NULL){
            printk("File not found!\n");
            return;
        }

        ExecuteProgram(program);
        DeallocFile(program);
        
    }else if(strcmp(cmd, "pginfo")){
        printk("Total pages: %u\n", GetPages());
    }else if(strcmp(cmd, "pwd")){
        printk(workingDir);
        printk("\n");
    }else if(strncmp(cmd, "acpiinfo", 8)){
        //ConfirmRSDP();
    }else{
        printk("Invalid Command!\n");
    }
}

int CliHandler(mboot_info_t* multibootInfo){
    printk("Kernel-Integrated Shell (KISh)\n");
    printk("Enter \"help\" into the console for a list of commands.\n");
    // Allocate 1000 bytes for a command. That means a max of 1000 characters. Should be more than enough.
    char* command = (char*)alloc(1001);
    memset(command, 0, 1001);

    int index = 0;

    printk(workingDir);
    printk("> ");
    while(true){
        uint8 lastKey = GetKey();
        if(lastKey != 0){
            switch (lastKey)
            {
                case '\b':
                    if(GetX() > strlen(workingDir) + 2){
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
                    printk(workingDir);
                    printk("> ");
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