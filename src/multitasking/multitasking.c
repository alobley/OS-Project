#include <multitasking.h>
#include <kernel.h>
#include <vfs.h>

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

MUTEXSTATUS PeekMutex(mutex_t* mutex){
    if(mutex->locked){
        return MUTEX_IS_LOCKED;
    }else{
        return MUTEX_IS_UNLOCKED;
    }
}

// Lock a mutex for the current process
MUTEXSTATUS MutexLock(mutex_t* mutex){
    asm volatile("" ::: "memory");
    if(mutex->locked){
        // Mutex is locked, enqueue process
        EnqueueProcess(mutex, currentProcess);
        currentProcess->state = WAITING;
        do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);

        // We will return here when the mutex is unlocked
        return MUTEX_AQUIRED;
    }else{
        asm volatile("lock bts $0, %0" : "+m" (mutex->locked) : : "memory");
        mutex->owner = currentProcess;
        return MUTEX_AQUIRED;
    }
}

// Unlock a mutex
MUTEXSTATUS MutexUnlock(mutex_t* mutex){
    asm volatile("" ::: "memory");
    asm volatile("lock btr $0, %0" : "+m" (mutex->locked) : : "memory");
    if(mutex->waitQueue.first != NULL){
        // Dequeue process
        if(DequeueProcess(mutex)){
            mutex->owner->state = RUNNING;
        }
    }

    return MUTEX_IS_UNLOCKED;
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

extern pcb_t* kernelPCB; // Defined in kernel.c

// Process control functions
// TODO: implement process paging, stack, and heap allocation. For now, we'll just create a process with a NULL stack and heap.
pcb_t* CreateProcess(void (*entryPoint)(void), char* name, char* directory, uid owner, bool priveliged, bool kernel, bool foreground, priority_t priority, uint64_t timeSlice, pcb_t* parent){
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

    if(kernel){
        // Kernel-level processes can do this themselves
        process->pageDirectory = 0; // TODO: implement process paging

        // TODO: implement stack allocation
        process->stackBase = 0;
        process->stackTop = 0;
        process->esp = process->stackTop; // Set ESP to the top of the stack

        // TODO: implement heap allocation
        process->heapBase = 0;
        process->heapEnd = 0;
    }

    // Add process to process list
    process->next = processList;
    processList = process;

    process->name = name;

    process->fileList = CreateFileList();

    return process;
}

void DestroyProcess(pcb_t* process){
    if(process == NULL){
        return;
    }

    if(currentProcess->next != NULL){
        currentProcess->next->previous = currentProcess->previous;
    }

    if(process->fileList != NULL){
        DestroyFileList(process->fileList);
    }
    
    // Recursively kill all children
    // Should I reparent instead?
    pcb_t* child = process->firstChild;
    while(child != NULL){
        pcb_t* nextChild = child->next;
        DestroyProcess(child);
        child = nextChild;
    }

    if(process->workingDirectory != NULL){
        // Free the working directory of the process
        hfree(process->workingDirectory);
    }

    // Deallocate the file descriptor tree...

    // Deallocate other things...

    // Unpage stack and heap...

    // Search the device tree for any devices owned by the process and destroy them... (needed?)

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
        DestroyProcess(currentProcess);
    }
    // Search for the next process to run
    //memcpy(currentProcess->registers, context, sizeof(struct Registers));
    if(currentProcess->next != NULL){
        // Save the registers of the current process
        currentProcess = currentProcess->next;
    }

}