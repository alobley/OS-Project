#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <types.h>
#include <idt.h>
#include <memmanage.h>

// Experiments and placeholders for multitasking. Not the current primary focus.

#define MAX_PROCESS_NAME 32

enum ProcessState {
    RUNNING,
    WAITING,
    STOPPED
};

typedef struct Process_Flags {
    bool priveliged : 1;        // Process will have direct access to other processes (maybe), I/O ports, and priveliged system calls. Must be run as root user.
    bool kernel : 1;            // Process is a kernel process and has access to all system resources
    bool foreground : 1;        // Process is in the foreground and can receive input
    bool disableSwap : 1;       // Process cannot be swapped out (no swapping implemented atm)
} process_flags_t;

// The process control block. (Note: programs can obtain their own PCB in user mode, but kernel-level processes can access all PCBs. Specific values can be obtained by any program.)
// TODO:
// - Add a task scheduler
// - Add process creation and destruction functions
// - Add support for this in the ABI
// - Add priveliged ABI system calls
typedef struct Process_Control_Block {
    uint32 pid;                                 // The process ID
    process_flags_t flags;                      // The flags for the process
    uint8 user;                                 // If launched by root (0) then this can be a driver or a system process and has greater access to system resources
    char name[MAX_PROCESS_NAME + 1];            // The name of the process and a null terminator
    int (*entryPoint)(void);                    // The entry point of the process
    struct Registers* currentRegs;              // Make sure to update these every time the process is switched (How do I keep the kernel's stack pointer consistent?)
    PageDirectory* processPageDir;              // The page directory for the process
    uint32 size;                                // The size of the whole program in bytes (this might be able to be omitted with paging)
    void* stackTop;                             // The top of the stack (lowest address)
    void* stackBottom;                          // The bottom of the stack (highest address)
    void* heapStart;                            // The start address of the heap
    void* heapEnd;                              // The end address of the heap
    uint32 timeSlice;                           // The time slice for the process (how much time the task scheduler gives it)
    uint64 executionTime;                       // The total time the process has been running (instead of this, a start date and time should probably be used)
    enum ProcessState state;                    // The state of the process
    // int priority;                            // Starting with round-robin, so this is irrelevant. Disabled to reduce RAM use.

    int32 exitStatus;                           // The exit status of the process

    struct Process_Control_Block* parent;       // The parent process
    struct Process_Control_Block* firstChild;   // The first child process (linked list. firstChild->next is the next child) (disable parent while child is running?)
    struct Process_Control_Block* next;         // The next process in the list
} pcb_t;

typedef struct ProcessQueue {
    pcb_t* first;                               // The first process in the queue (waiting the shortest amount of time)
    pcb_t* next;                                // The next process in the queue
    pcb_t* last;                                // The last process in the queue (waiting the longest amount of time)
} process_queue_t;

typedef struct mutex {
    volatile bool locked;                       // If the mutex is locked
    pcb_t* owner;                               // The process that owns the mutex
    process_queue_t waiting;                    // The processes waiting for the mutex
} mutex_t;

// First come first serve spinlock. The first process that tries to lock the spinlock gets it.
typedef struct spinlock {
    volatile bool locked;                       // If the spinlock is locked
    pcb_t* owner;                               // The process that owns the spinlock
} spinlock_t;

pcb_t* GetCurrentProcess();

#endif