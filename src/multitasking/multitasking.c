#include <multitasking.h>
#include <kernel.h>

pcb_t* currentProcess = NULL;
pcb_t* processList = NULL;
volatile uint16_t numProcesses = 0;                 // 0 is the kernel process

pcb_t* GetCurrentProcess(void){
    return currentProcess;
}

void EnqueueProcess(mutex_t* mutex, pcb_t* process){
    qnode* node = (struct QueueNode*)halloc(sizeof(qnode));
    node->process = process;
    node->next = NULL;

    if(mutex->waitQueue.last == NULL){
        mutex->waitQueue.first = node;
        mutex->waitQueue.last = node;
        return;
    }

    mutex->waitQueue.last->next = node;
    mutex->waitQueue.last = node;
}

pcb_t* DequeueProcess(mutex_t* mutex){
    if(mutex->waitQueue.first == NULL){
        // Failed to dequeue process
        return NULL;
    }

    qnode* node = mutex->waitQueue.first;
    mutex->waitQueue.first = node->next;
    if(mutex->waitQueue.first == NULL){
        mutex->waitQueue.last = NULL;
    }

    mutex->owner = node->process;
    hfree(node);
    return mutex->owner;
}

// Lock a mutex for the current process
void MutexLock(mutex_t* mutex){
    asm volatile("" ::: "memory");
    if(mutex->locked){
        // Mutex is locked, enqueue process
        EnqueueProcess(mutex, currentProcess);
        currentProcess->state = WAITING;
        do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
        return;
    }else{
        asm volatile("lock bts $0, %0" : "+m" (mutex->locked) : : "memory");
        mutex->owner = currentProcess;
    }
}

// Unlock a mutex
void MutexUnlock(mutex_t* mutex){
    asm volatile("" ::: "memory");
    if(mutex->owner != currentProcess){
        // Mutex is not owned by the current process
        return;
    }
    asm volatile("lock btr $0, %0" : "+m" (mutex->locked) : : "memory");
    if(mutex->waitQueue.first != NULL){
        // Dequeue process
        if(DequeueProcess(mutex)){
            mutex->owner->state = RUNNING;
        }
    }
}

// Force unlock a mutex (for kernel use only)
void KernelOverrideUnlock(mutex_t* mutex){
    asm volatile("lock btr $0, %0" : "+m" (mutex->locked) : : "memory");
    if(mutex->waitQueue.first != NULL){
        // Dequeue process
        DequeueProcess(mutex);
        mutex->owner->state = RUNNING;
    }
}

// Spinlock functions...

extern pcb_t* kernelPCB; // Defined in kernel.c

// Process control functions
// TODO: implement process paging, stack, and heap allocation. For now, we'll just create a process with a NULL stack and heap.
pcb_t* CreateProcess(int (*entryPoint)(void), char* name, char* directory, uid owner, bool priveliged, bool kernel, bool foreground, priority_t priority, uint64_t timeSlice, pcb_t* parent){
    pcb_t* process = (pcb_t*)halloc(sizeof(pcb_t));
    if(process == NULL){
        return NULL; // Failed to allocate memory for process
    }
    memset(process, 0, sizeof(pcb_t));
    process->pid = numProcesses++;
    process->flags.priveliged = priveliged;
    process->flags.kernel = kernel;
    process->flags.foreground = foreground;
    process->state = RUNNING;
    process->EntryPoint = entryPoint;
    process->timeSlice = timeSlice;
    process->priority = priority;
    process->owner = owner;

    process->workingDirectory = directory;

    if(parent == NULL && kernelPCB == NULL){
        // This is the kernel process
        parent = NULL;
    }else{
        // Add the process as a child of the kernel and append it to the process list
        process->parent = parent;
        if(kernelPCB->firstChild == NULL){
            kernelPCB->firstChild = process;
        }else{
            pcb_t* current = kernelPCB->firstChild;
            while(current->next != NULL){
                current = current->next;
            }
            current->next = process;
            process->previous = current;
        }
        process->next = NULL;
    }

    process->pageDirectory = 0; // TODO: implement process paging

    // TODO: implement stack allocation
    process->stack = 0;
    process->stackBase = 0;
    process->stackTop = 0;

    // TODO: implement heap allocation
    process->heapBase = 0;
    process->heapEnd = 0;

    // Add process to process list
    process->next = processList;
    processList = process;

    process->registers = halloc(sizeof(struct Registers));
    memset(process->registers, 0, sizeof(struct Registers));

    process->name = name;

    return process;
}

void DestroyProcess(pcb_t* process){
    if(process == NULL){
        return;
    }

    hfree(process->registers);
    hfree(process->workingDirectory);
    // Deallocate other things...

    // Search the device tree for any devices owned by the process and destroy them...

    hfree(process);
    numProcesses--;
}

// More...

// TODO: implement scheduler
void Scheduler(void){
    return;
}

void SwitchToSpecificProcess(pcb_t* process, struct Registers* regs){
    currentProcess = process;
    //memcpy(currentProcess->registers, regs, sizeof(struct Registers));
}

void SetCurrentProcess(pcb_t* process){
    currentProcess = process;
}

void SwitchProcess(bool kill, struct Registers* context){
    if(kill){
        currentProcess->next->previous = currentProcess->previous;
        DestroyProcess(currentProcess);
    }
    // Search for the next process to run
    memcpy(currentProcess->registers, context, sizeof(struct Registers));
    if(currentProcess->next != NULL){
        // Save the registers of the current process
        currentProcess = currentProcess->next;
    }

}