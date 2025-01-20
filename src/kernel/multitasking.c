#include <multitasking.h>

pcb_t* currentProcess = NULL;       // The currently running process
pcb_t* processList = NULL;          // The list of all processes
uint16 processCount = 0;            // The number of processes in the system

pcb_t* GetCurrentProcess(){
    return currentProcess;
}

void EnqueueProcess(mutex_t* mutex, pcb_t* process){
    // Add the process to the end of the queue
    process_queue_t* queue = &mutex->waiting;
    if(queue->first == NULL){
        queue->first = process;
        queue->last = process;
    }else{
        queue->last->next = process;
        queue->last = process;
    }
}

pcb_t* DequeueProcess(mutex_t* mutex){
    // Remove the first process from the queue
    process_queue_t* queue = &mutex->waiting;
    if(queue->first == NULL){
        return NULL;
    }
    pcb_t* newOwner = queue->first;
    pcb_t* next = queue->first->next;
    queue->first = next;
    if(next == NULL){
        queue->last = NULL;
    }
    return newOwner;
}

// Locks a mutex to a process. If the mutex is already locked, the process is stopped and added to the queue.
void MutexLock(mutex_t* mutex){
    asm volatile("" ::: "memory");                          // Prevent the compiler from reordering instructions
    if(mutex->locked){
        // Add the process to the queue
        currentProcess->state = WAITING;
        EnqueueProcess(mutex, currentProcess);
    }else{
        asm volatile("inc %0" : "=m" (mutex->locked));       // Atomically set the lock
        mutex->owner = currentProcess;
    }
    // Do something else? Switch tasks?
}

// Unlocks a mutex. Uses a simple queue method. The process that has been waiting the longest is given the mutex.
void MutexUnlock(mutex_t* mutex){
    asm volatile("" ::: "memory");                          // Prevent the compiler from reordering instructions
    if(mutex->owner != currentProcess){
        // Only the owner can unlock the mutex
        return;
    }
    asm volatile("dec %0" : "=m" (mutex->locked));           // Atomically unlock the mutex
    pcb_t* newOwner = DequeueProcess(mutex);
    if(newOwner != NULL){
        newOwner->state = RUNNING;
    }
}

// Locks a spinlock. If the spinlock is already locked, inform the calling process. The process must check if the lock is free before attempting to lock it.
bool SpinlockLock(spinlock_t* lock){
    asm volatile("" ::: "memory");                          // Prevent the compiler from reordering instructions
    if(lock->locked){
        return false;
    }else{
        asm volatile("inc %0" : "=m" (lock->locked));       // Atomically set the lock
        lock->owner = currentProcess;
        return true;
    }
}

// Checks if a spinlock is locked
bool CheckSpinlock(spinlock_t* lock){
    return lock->locked;
}

// Unlocks a spinlock.
void SpinlockUnlock(spinlock_t* lock){
    asm volatile("" ::: "memory");                          // Prevent the compiler from reordering instructions
    if(lock->owner != currentProcess){
        // Only the owner can unlock the spinlock
        return;
    }
    asm volatile("dec %0" : "=m" (lock->locked));           // Atomically unlock the spinlock
    lock->owner = NULL;
}