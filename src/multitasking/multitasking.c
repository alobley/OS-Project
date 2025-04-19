#include <multitasking.h>
#include <alloc.h>
#include <string.h>
#include <console.h>
#include <vfs.h>
#include <gdt.h>
#include <time.h>

// REMINDER: I need to update the TTY subsystem and kernel-integrated drivers to comply with the new multitasking and I/O system

// The kernel's PCB
pcb_t* kernelPCB = NULL;

// The currently running process
pcb_t* currentProcess = NULL;

// 65535 total processes should be way more than enough
uint16_t numProcesses = 0;

// 2048 is 65536 / 32
uint32_t pidBitmap[2048] = {0};

// This list allows executing of scheduled processes in a clean and quick manner instead of searching the whole process tree upon every context switch
struct Process_List* scheduledProcesses = NULL;

// Clear a bit in the PID bitmap
void FreePid(pid_t pid){
    size_t bit = (size_t)pid / 32;
    size_t index = pid / 32;
    pidBitmap[index] &= ~(1 << (bit % 32));
}

// Set a bit in the PID bitmap and return its index
pid_t AllocatePid(){
    for(size_t i = 0; i < sizeof(pidBitmap); i++){
        if(pidBitmap[i] != 0xFFFFFFFF){
            for(size_t j = 0; j < 32; j++){
                if(!(pidBitmap[i] & (1 << j))){
                    pidBitmap[i] |= 1 << j;
                    return i * 32 + j;
                }
            }
        }
    }

    return 0x0000;
}

pcb_t* GetCurrentProcess(){
    return currentProcess;
}

void SetCurrentProcess(pcb_t* process){
    currentProcess = process;
}

void SetBitmapBit(uint32_t* bitmap, index_t bit){
    bitmap[bit / 32] |= (1 << (bit % 32));
}

void ClearBitmapBit(uint32_t* bitmap, index_t bit){
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

int InheritFiles(pcb_t* newProcess, pcb_t* parent){
    if(parent != NULL){
        // Inherit the file table of the parent
        file_table_t* table = (file_table_t*)halloc(sizeof(file_table_t));
        if(table == NULL){
            return STANDARD_FAILURE;
        }
        memset(table, 0, sizeof(file_table_t));
        memcpy(table, parent->fileTable, sizeof(file_table_t));
        newProcess->fileTable = table;
        table->numOpenFiles = parent->fileTable->numOpenFiles;
        table->max_fds = DEFAULT_MAX_FDS;

        table->bitmap = halloc(sizeof(uint32_t) * 2048);
        if(table->bitmap == NULL){
            hfree(table);
            return STANDARD_FAILURE;
        }
        memset(table->bitmap, 0, sizeof(sizeof(uint32_t) * 2048));
        memcpy(table->bitmap, parent->fileTable->bitmap, sizeof(uint32_t) * 2048);

        table->openFiles = halloc(sizeof(file_context_t*) * parent->fileTable->numOpenFiles);
        if(table->openFiles == NULL){
            hfree(table->bitmap);
            hfree(table);
            return STANDARD_FAILURE;
        }
        memset(table->openFiles, 0, table->numOpenFiles * sizeof(file_context_t*));
        memcpy(table->openFiles, parent->fileTable->openFiles, table->numOpenFiles * sizeof(file_context_t*));
        
        for(size_t i = 0; i < table->numOpenFiles; i++){
            file_context_t* context = halloc(sizeof(file_context_t));
            if(context == NULL){
                for(size_t j = 0; j < i; i++){
                    // Free all previously allocated files
                    hfree(table->openFiles[j]);
                }
                hfree(table->bitmap);
                hfree(table->openFiles);
                hfree(table);
                return STANDARD_FAILURE;
            }
            memset(context, 0, sizeof(file_context_t));
            memcpy(context, parent->fileTable->openFiles[i], sizeof(file_context_t));
            table->openFiles[i] = context;
        }
    }else{
        // Allocate a new file table
        file_table_t* table = (file_table_t*)halloc(sizeof(file_table_t));
        if(table == NULL){
            return STANDARD_FAILURE;
        }
        memset(table, 0, sizeof(file_table_t));

        newProcess->fileTable = table;
        table->arrSize = 3;
        table->max_fds = DEFAULT_MAX_FDS;

        table->bitmap = halloc(sizeof(uint32_t) * 2048);
        if(table->bitmap == NULL){
            hfree(table);
            return STANDARD_FAILURE;
        }
        memset(table->bitmap, 0, sizeof(sizeof(uint32_t) * 2048));

        // STDIN, STDOUT, and STDERR
        table->openFiles = halloc(sizeof(file_context_t*) * 3);
        if(table->openFiles == NULL){
            hfree(table->bitmap);
            hfree(table);
            return STANDARD_FAILURE;
        }
        memset(table->openFiles, 0, table->numOpenFiles * sizeof(file_context_t*));

        SetBitmapBit(table->bitmap, STDIN_FILENO);
        SetBitmapBit(table->bitmap, STDOUT_FILENO);
        SetBitmapBit(table->bitmap, STDERR_FILENO);

        vfs_node_t* stdin = VfsFindNode("/dev/stdin");
        vfs_node_t* stdout = VfsFindNode("/dev/stdout");
        vfs_node_t* stderr = VfsFindNode("/dev/stderr");

        CreateFileContext(stdin, newProcess->fileTable, stdin->flags);
        CreateFileContext(stdout, newProcess->fileTable, stdout->flags);
        CreateFileContext(stderr, newProcess->fileTable, stderr->flags);
    }

    return STANDARD_SUCCESS;
}

void UpdateSignals(uint32_t signal){
    // Check the signals and insert the handlers into the user call stack. Stub for now.
    return;
}

void DefaultSignalHandler(){
    // Another stub, need to work on signal handling.
    return;
}

void PrepareSignals(pcb_t* process){
    // I can implement this another time. Probably best not to go all-in at once.
    return;
}

/// @brief Add the process to the list of scheduled processes (it is inserted into the start of the list)
/// @param process The process to schedule
/// @return Success or error code
int ScheduleProcess(pcb_t* process){
    struct Process_List* nextNode = halloc(sizeof(struct Process_List));
    if(nextNode == NULL){
        return STANDARD_FAILURE;
    }
    memset(nextNode, 0, sizeof(struct Process_List));

    process->schedulerNode = nextNode;

    if(scheduledProcesses == NULL){
        scheduledProcesses = nextNode;
    }else{
        // Insert into the start of the list
        nextNode->next = scheduledProcesses;
        scheduledProcesses = nextNode;
    }

    return STANDARD_SUCCESS;
}

void UnscheduleProcess(pcb_t* process){
    if(process->schedulerNode->previous){
        // If there's more than one process running and this isn't the first scheduled process
        process->schedulerNode->previous->next = process->schedulerNode->next;
    }else{
        // All other cases (if no other processes, then scheduledProcesses should still end up being NULL)
        scheduledProcesses = process->schedulerNode->next;
    }
    hfree(process->schedulerNode);
}

pcb_t* GetNextProcess(pcb_t* process){
    struct Process_List* node = process->schedulerNode->next;
    if(node == NULL){
        // If there are no other processes scheduled, then we need to go back to the start of the list
        node = scheduledProcesses;
    }
    
    if(node == NULL){
        // If there are no processes scheduled, then we need to return NULL
        return NULL;
    }

    // Get the next process in the list
    pcb_t* nextProcess = node->next->this;

    bool fullPass = false;
    pcb_t* startProcess = nextProcess;
    while(nextProcess->state != RUNNING){
        if(nextProcess->state == SLEEPING){
            // If the process is sleeping, we need to check if it has timed out
            if(nextProcess->sleepUntil < GetTicks()){
                // The process is still sleeping, so we need to skip it
                nextProcess = node->next->this;
            }else{
                // The process has timed out, so we can wake it up
                nextProcess->state = RUNNING;
                return nextProcess;
            }
        }

        if(node->next == NULL || node->next->this == NULL){
            // If the next process is NULL, then we need to go back to the start of the list
            nextProcess = scheduledProcesses->this;
        }
        nextProcess = node->next->this;
        if(nextProcess == startProcess){
            // If we have gone through the whole list and there were no active programs, then we need to return NULL
            fullPass = true;
            if(process->state == RUNNING){
                // If the process is running, then we can just switch back to it
                return process;
            }
            return NULL;
        }
    }
    // Otherwise, return the next process
    return nextProcess;
}

pcb_t* CreateProcess(const char* processName, uint64_t timeSlice, struct Resource_Limits limits, uid_t user, gid_t group, pid_t processGroup, pid_t sid, vfs_node_t* workingDirectory, pcb_t* parent){
    if(parent == NULL && numProcesses != 0){
        // No orphans allowed (other than the kernel)!
        return NULL;
    }

    pcb_t* newProcess = (pcb_t*)halloc(sizeof(pcb_t));
    if(newProcess == NULL){
        // Out of memory!
        return NULL;
    }

    // So that we don't have to explicitly set every nonset field
    memset(newProcess, 0, sizeof(pcb_t));

    if(parent == NULL && numProcesses == 0){
        newProcess->state = KERNEL;
        newProcess->priority = DONT_SCHEDULE;           // The kernel doesn't need to be scheduled
        newProcess->timeSlice = 0;
    }else{
        // Set the state to running
        newProcess->state = RUNNING;

        // All processes start with default priority
        newProcess->priority = DEFAULT_PRIORITY;

        // Set the time slice of the process
        if(timeSlice == 0){
            // Just putting this if/else statement here in case I make coding errors down the line
            newProcess->timeSlice = DEFAULT_TIMESLICE;
        }else{
            newProcess->timeSlice = timeSlice;
        }
    }

    // Set the user and group IDs provided
    newProcess->user = user;
    newProcess->group = group;

    // These are essentially the same as the regular user/group IDs, but can be changed by the program (to an extent)
    newProcess->effectiveUser = user;
    newProcess->effectiveGroup = group;

    // Get the PID for this process
    newProcess->pid = AllocatePid();
    if(newProcess->pid == 0 && numProcesses > 0){
        // Non-kernel process couldn't get a PID
        hfree(newProcess);
        return NULL;
    }

    if(parent != NULL && processGroup == 0){
        // Set the process group to the parent's process group if the parent exists and no group was defined
        newProcess->processGroupId = parent->processGroupId;
    }else{
        // Otherwise, the process group was defined
        newProcess->processGroupId = processGroup;
    }

    // Set the session ID of the process
    if(parent != NULL && sid == 0){
        newProcess->sessionId = INIT_SID;
    }else{
        newProcess->sessionId = sid;
    }

    // The working directory of the process
    newProcess->workingDirectory = workingDirectory;

    // Set the process name
    if(processName == NULL){
        newProcess->processName = strdup("ERROR");
    }else{
        newProcess->processName = strdup(processName);
    }

    if(newProcess->processName == NULL){
        // Out of memory!
        FreePid(newProcess->pid);
        hfree(newProcess);
        return NULL;
    }

    if(parent != NULL){
        parent->numChildren++;
    }

    // Allocate a linear array of page tables (these will be mapped to virtual memory and memcpy'd to the page directory, but does not include the kernel). We only need the page tables themselves.
    newProcess->pageTables = halloc(sizeof(pde_t) * 1024);
    if(newProcess->pageTables == NULL){
        FreePid(newProcess->pid);
        hfree(newProcess->processName);
        hfree(newProcess);
        return NULL;
    }

    // Inherit the file table, if any, of the parent
    if(InheritFiles(newProcess, parent) == STANDARD_FAILURE){
        FreePid(newProcess->pid);
        hfree(newProcess->processName);
        hfree(newProcess->pageTables);
        hfree(newProcess);
        return NULL;
    }

    // Set the signal handlers for the new process
    for(int i = 0; i < NUM_SIGNALS; i++){
        newProcess->signalInfo.handlers[i] = DefaultSignalHandler;
    }
    newProcess->signalInfo.masked = DEFAULT_MASK;
    newProcess->signalInfo.pending = 0;

    // Insert the new process into the process tree. Insert at the beginning of the parent's child list for speed.
    if(parent){
        newProcess->next = parent->firstChild;
        parent->firstChild = newProcess;
        newProcess->parent = parent;
    }

    // Increment the number of running processes
    numProcesses++;

    newProcess->magic = PCB_MAGIC;
    return newProcess;
}

pcb_t* DuplicateProcess(pcb_t* this){
    // TODO: implement
    return NULL;
}

int LoadExecute(pcb_t* process, vfs_node_t* data){
    // TODO: implement
    return STANDARD_FAILURE;
}

// NOTE: This must unpage the current process and load the new process
int ReplaceProcess(char* newName, void* data, pcb_t* pcb, struct registers* regs){
    // TODO: implement
    return STANDARD_FAILURE;
}

// NOTE: I'll need to implement searching through the list of waiting processes and setting them to running
int DestroyProcess(pcb_t* process){
    // TODO: implement
    return STANDARD_FAILURE;
}

// Queue a process into a process queue
int EnqueueProcess(mutex_t* lock, pcb_t* requester){
    struct Queue_Node* node = halloc(sizeof(struct Queue_Node));
    if(node == NULL){
        return SEM_INTERNAL_ERROR;
    }

    requester->state = WAITING;

    node->process = requester;

    if(lock->waitQueue.first == NULL){
        lock->waitQueue.first = node;
        lock->waitQueue.last = node;
        node->next = NULL;
        node->prev = NULL;
    }else{
        lock->waitQueue.last->next = node;
        node->prev = lock->waitQueue.last;
        lock->waitQueue.last = node;
        node->next = NULL;
    }

    return SEM_LOCKED;
}

// Dequeue the process qhich has been waiting the longest
pcb_t* DequeueProcess(mutex_t* lock){
    struct Queue_Node* node = lock->waitQueue.first;
    if(node != NULL){
        lock->waitQueue.first = node->next;
        if(node->next == NULL){
            lock->waitQueue.last = NULL;
        }
        node->next->prev = NULL;
        pcb_t* nextProcess = node->process;
        nextProcess->waitingFor = NULL;
        nextProcess->state = RUNNING;
        hfree(node);
        return nextProcess;
    }else{
        return NULL;
    }
}

// Remove a specific process from a queue (for example, when killing a process)
int RemoveProcessFromQueue(pcb_t* process, queue_t* queue){
    if(!process || !queue || !queue->first){
        return STANDARD_FAILURE;
    }

    struct Queue_Node* node = queue->first;
    while(node != NULL){
        if(node->process == process){
            node->prev->next = node->next;
            hfree(node);
            return STANDARD_SUCCESS;
        }
        node = node->next;
    }

    return STANDARD_FAILURE;
}

pcb_t* GetProcessByPID(pid_t pid){
    // Search through the list of scheduled processes for a given pid
    pcb_t* pcb = scheduledProcesses->this;
    struct Process_List* list = scheduledProcesses;
    while(pcb != NULL && list != NULL){
        if(pcb->pid == pid){
            return pcb;
        }
        list = list->next;
        pcb = list->this;
    }

    return NULL;
}

int InsertSignals(pcb_t* process, unsigned int signal){
    return STANDARD_FAILURE;
}

int MPeek(mutex_t* lock){
    return Atomic_Test(lock->locked);
}

int MLock(mutex_t* lock, pcb_t* requester){
    if(Atomic_Test_And_Set(lock->locked)){
        // The mutex is already locked!
        requester->state = WAITING;
        requester->waitingFor = lock;
        printk("Mutex already locked!\n");                  // Just for debug to make sure my atomic operations work properly
        return EnqueueProcess(lock, requester);
    }else{
        // The atomic test and set already locked the mutex
        lock->owner = requester;
        return SEM_AQUIRED;
    }
}

void MUnlock(mutex_t* lock){
    lock->owner = DequeueProcess(lock);
    if(lock->owner == NULL){
        Atomic_Test_And_Reset(lock->locked);
    }
}

int SpinPeek(spinlock_t* lock){
    return Atomic_Test(lock->locked);
}

int SpinLock(spinlock_t* lock, pcb_t* requester){
    if(Atomic_Test_And_Set(lock->locked)){
        return SEM_LOCKED;
    }else{
        // The atomic test and set already locked the spinlock
        lock->owner = requester;
        return SEM_AQUIRED;
    }
}

void SpinUnlock(spinlock_t* lock){
    Atomic_Test_And_Reset(lock->locked);
    lock->owner = NULL;
}

// This shall only be called upon an interrupt. Both fields are required.
int ContextSwitch(struct Registers* regs, pcb_t* next){
    if(next == NULL){
        // Find another process to switch to instead
        uint64_t timeout = GetTicks() + 5000;             // 5 second timeout
        while(next == NULL){
            next = GetNextProcess(currentProcess);
            if(GetTicks() >= timeout){
                // We timed out waiting for a process to switch to
                printk("KERNEL PANIC: No processes to switch to!\n");
                // Load init or something, maybe try to locate deadlocks...
                STOP
            }
        }
    }

    // Disable interrupts
    cli

    // Copy the CPU state to the PCB of the process that's switching away
    memcpy(&currentProcess->context, regs, sizeof(struct Registers));

    // Replace the curent process's page tables with the next one's
    ReplacePageTables(next->pageTables);

    // Copy the next process's CPU state onto the stack (the kernel stack will be fine as its virtual address is constant)
    memcpy(regs, &next->context, sizeof(struct Registers));

    // Set the next process
    currentProcess = next;

    // Flush the TLB
    FlushTLB();

    // Change the location of the kernel stack to that of the new process
    tss.esp0 = currentProcess->context.esp;

    // Re-enable interrupts and return
    sti
    return STANDARD_SUCCESS;
}

/// @brief Add a new memory region to a process
/// @param process The process to map a region to
/// @param length The length of the memory region
/// @param flags The flags of the memory region
/// @param start Pointer to the start of the region. To allocate an arbitrary region, both start and end must be NULL.
/// @param end Pointer to the end of the region
/// @param removeMatches The function will remove all memory regions with the same flags as those provided
/// @return Pointer to the memory region
int AddMemoryRegion(pcb_t* process, size_t length, uint32_t flags, void* start, void* end, bool removeMatches){
    size_t usedMem = 0;
    index_t i = 0;

    // Change the length for allocation to round up to the nearest page
    size_t pagesToAdd = (length / PAGE_SIZE) + 1;
    length = pagesToAdd * PAGE_SIZE;

    while(usedMem < process->limits.maxMemUsage && i < process->numRegions){
        if(removeMatches && process->regions[i]->flags == flags){
            // i shouldn't need to be modified iirc
            int result = RemoveMemoryRegion(process, i);
            if(result != STANDARD_SUCCESS){
                return result;
            }
        }
        usedMem += process->regions[i]->length;
        i++;
    }

    if(usedMem + length > process->limits.maxMemUsage){
        return STANDARD_FAILURE;
    }


    void* newStart = start;
    void* newEnd = end;

    if(start == NULL && end == NULL){
        // Add the new region to the end of the virtual memory space, just before the end with the predefined memory regions
        newStart = process->regions[REGION_SHM_INDEX]->start - length;
        newEnd = process->regions[REGION_SHM_INDEX]->start;
    }

    for(int j = 0; j < i -1; j++){
        if((newStart >= process->regions[j]->start && newStart <= process->regions[j]->end) || (newEnd >= process->regions[j]->start && newEnd <= process->regions[j]->end)){
            // Region is poking into another memory region and cannot be used. There isn't enough memory left to allocate.
            return STANDARD_FAILURE;
        }
    }

    memregion_t* newRegion = halloc(sizeof(memregion_t));
    if(newRegion == NULL){
        return STANDARD_FAILURE;
    }

    process->numRegions++;
    process->regions = rehalloc(process->regions, process->numRegions * sizeof(memregion_t*));
    if(process->regions == NULL){
        // Out of memory! The process is in an unstable state!
        return EMERGENCY_NO_MEMORY;
    }
    process->regions[process->numRegions - 1] = newRegion;

    newRegion->start = newStart;
    newRegion->end = newEnd;
    newRegion->size = length;
    newRegion->flags = flags;

    for(size_t i = 0; i < pagesToAdd; i++){
        // Page the memory
        if(user_palloc(newStart + (i * PAGE_SIZE), DEFAULT_USER_PAGE_FLAGS) != PAGE_OK){
            // Out of memory!
            hfree(newRegion);
            process->numRegions--;
            process->regions = rehalloc(process->regions, process->numRegions * sizeof(memregion_t*));
            // Free allocated pages
            for(size_t j = 0; j < i; j++){
                pfree(newStart + (j * PAGE_SIZE));
            }
            if(process->regions == NULL){
                return EMERGENCY_NO_MEMORY;
            }
            return STANDARD_FAILURE;
        }
    }

    return STANDARD_SUCCESS;
}

/// @brief Remove a process's memory region based on a given index
/// @param process The process to modify
/// @param index The index of the memory region
/// @return Success or error code
int RemoveMemoryRegion(pcb_t* process, index_t index){
    if(process == NULL || index >= process->numRegions){
        // Nothing to do
        return STANDARD_FAILURE;
    }

    // The size will always be page-aligned
    size_t size = process->regions[index]->length;

    void* regionStart = process->regions[index]->start;

    hfree(process->regions[index]);
    process->regions[index] = NULL;

    for(index_t i = index; i < process->numRegions; i++){
        // Shift all values in the array
        process->regions[i] = process->regions[i + 1];
    }

    process->numRegions--;
    process->regions = rehalloc(process->regions, process->numRegions * sizeof(memregion_t*));

    // Free allocated pages
    for(size_t j = 0; j < size / PAGE_SIZE; j++){
        user_pfree((virtaddr_t)regionStart + (j * PAGE_SIZE));
    }

    if(process->regions == NULL){
        // Process is in an unstable state!
        return EMERGENCY_NO_MEMORY;
    }

    return STANDARD_SUCCESS;
}

memregion_t* GetRegionWithFlags(pcb_t* process, uint32_t flags){
    for(size_t i = 0; i < process->numRegions; i++){
        if(process->regions[i]->flags == flags){
            return process->regions[i];
        }
    }

    return NULL;
}

// Need process loading and destruction as well as IPC/session functions..
