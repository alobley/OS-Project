#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <stdint.h>
#include <stdbool.h>
#include <interrupts.h>
#include <util.h>
#include <alloc.h>
#include <string.h>
#include <users.h>

#define process_switch_stub()

#define PROCESS_DEFAULT_TIME_SLICE 100          // Default time slice in milliseconds

typedef enum Process_Priority {
    KERNEL = 0,                                 // Kernel processes
    REALTIME = 1,                               // Real-time processes
    HIGH = 2,                                   // High priority processes
    NORMAL = 3,                                 // Normal priority processes
    LOW = 4,                                    // Low priority processes
} priority_t;

typedef enum Process_State {
    RUNNING,                                    // Process is currently running (including non-current processes)
    WAITING,                                    // Process is waiting for an event (e.g. a mutex or timer)
    STOPPED                                     // Process is stopped (it is not running, e.g. closing)
} process_state_t;

typedef struct Process_Flags {
    bool priveliged : 1;                        // Running as root (if not, running as standard user)
    bool kernel : 1;                            // Running in kernel mode (drivers, etc. If not, running in userland)
    bool foreground : 1;                        // Running in the foreground
    bool disableSwap : 1;                       // Disable swapping (unimplemented)
} process_flags_t;

typedef struct Process_Control_Block {
    // Process information
    uint32_t pid;                               // Process ID
    uid owner;                                  // User ID
    process_flags_t flags;                      // Process flags
    process_state_t state;                      // Process state
    char* name;                                 // Process name
    int (*entryPoint)(void);                    // Entry point

    // Environment/Args
    char* env;                                  // Environment variables

    // Are these needed with env?
    int argc;                                   // Argument count
    char** argv;                                // Argument vector

    // Memory information
    uintptr_t stack;                            // Stack pointer
    uintptr_t stackBase;                        // Stack base
    uintptr_t stackTop;                         // Stack top
    uintptr_t heapBase;                         // Heap base
    uintptr_t heapEnd;                          // Heap end

    // Scheduler information
    uint64_t timeSlice;                         // Time slice (for the scheduler)
    priority_t priority;                        // Process priority

    // Other process information
    struct Process_Control_Block* next;         // Next process in the list
    struct Process_Control_Block* firstChild;   // First child process
    struct Process_Control_Block* parent;       // Parent process

    // Registers
    struct Registers* registers;                // Registers
} pcb_t;

typedef struct QueueNode {
    pcb_t* process;                             // Process in the queue
    struct QueueNode* next;                     // Next process in the queue
} qnode;

typedef struct Process_Queue {
    qnode* first;                               // First process in the queue
    qnode* last;                                // Last process in the queue
} process_queue_t;

typedef struct Mutex {
    volatile bool locked;                       // Mutex lock state
    pcb_t* owner;                               // Mutex owner
    process_queue_t waitQueue;                  // Mutex wait queue
} mutex_t;

#define MUTEX_INIT { false, NULL, { NULL, NULL } }

// First come first serve semaphore (for very short wait times)
typedef struct Spinlock {
    volatile bool locked;                       // Spinlock lock state
    pcb_t* owner;                               // Spinlock owner
} spinlock_t;

#define SPINLOCK_INIT { false, NULL }

pcb_t* GetCurrentProcess(void);
void SwitchProcess(pcb_t* process);
void DestroyProcess(pcb_t* process);
pcb_t* CreateProcess(int (*entryPoint)(void), char* name, uid owner, bool priveliged, bool kernel, bool foreground, priority_t priority, uint64_t timeSlice);

#endif // MULTITASKING_H