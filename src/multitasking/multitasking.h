#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <stdint.h>
#include <stdbool.h>
#include <interrupts.h>
#include <util.h>
#include <alloc.h>
#include <string.h>
#include <users.h>
#include <paging.h>
#include <system.h>

typedef struct VFS_Node vfs_node_t;
typedef struct FileContext file_context_t;
typedef struct FileListNode file_list_node_t;
typedef struct FileList file_list_t;

#define PROCESS_DEFAULT_TIME_SLICE 10           // Default time slice in milliseconds (0.01 seconds should be enough, right? We're talking 100 MHz or greater for i686-compatible...)
#define DEFAULT_DRIVER_TIME_SLICE 100           // Give drivers a little extra time

typedef enum {
    KERNEL = 0,                                 // Kernel processes
    REALTIME = 1,                               // Real-time processes
    HIGH = 2,                                   // High priority processes
    NORMAL = 3,                                 // Normal priority processes
    LOW = 4,                                    // Low priority processes
} priority_t;

typedef enum {
    RUNNING,                                    // Process is currently running (including non-current processes)
    DRIVER,                                     // Process is a driver and will not be scheduled
    WAITING,                                    // Process is waiting for an event (e.g. a mutex or timer)
    STOPPED                                     // Process is stopped (it is not running, e.g. closing)
} process_state_t;

typedef struct {
    bool priveliged : 1;                        // Running as root (if not, running as standard user)
    bool kernel : 1;                            // Running in kernel mode (drivers, etc. If not, running in userland)
    bool foreground : 1;                        // Running in the foreground
    bool disableSwap : 1;                       // Disable swapping (unimplemented)
} process_flags_t;

typedef struct Process_Control_Block {
    // Process information
    pid_t pid;                                  // Process ID
    uid owner;                                  // User ID
    process_flags_t flags;                      // Process flags
    process_state_t state;                      // Process state
    char* name;                                 // Process name
    void (*EntryPoint)(void);                   // Entry point

    // Environment/Args
    char* env;                                  // Environment variables
    char* workingDirectory;                     // Working directory

    // Are these needed with env?
    int argc;                                   // Argument count
    char** argv;                                // Argument vector

    // Memory information
    physaddr_t pageDirectory;                   // Page directory location
    uintptr_t stackBase;                        // Stack base
    //uintptr_t esp;                              // Stack pointer (ESP)
    //uintptr_t ebp;                              // Base pointer (EBP)
    uintptr_t stackTop;                         // Stack top
    uintptr_t heapBase;                         // Heap base
    uintptr_t heapEnd;                          // Heap end
    //uintptr_t eip;                              // Instruction pointer (EIP)

    size_t memSize;                             // Memory size (in bytes)
    struct Registers* regs;                     // CPU registers

    // Scheduler information
    uint64_t timeSlice;                         // Time slice (for the scheduler)
    priority_t priority;                        // Process priority

    // Other process information
    volatile struct Process_Control_Block* next;         // Next process in the list
    volatile struct Process_Control_Block* previous;     // Previous process in the list
    volatile struct Process_Control_Block* firstChild;   // First child process
    volatile struct Process_Control_Block* parent;       // Parent process

    file_list_t* fileList;                      // File list (for file descriptors)
} pcb_t;

typedef struct QueueNode {
    volatile pcb_t* process;                    // Process in the queue
    struct QueueNode* next;                     // Next process in the queue
} qnode;

typedef struct {
    qnode* first;                               // First process in the queue
    qnode* last;                                // Last process in the queue
} process_queue_t;

typedef struct {
    volatile uint8_t locked;                    // Mutex lock state (byte so that atomic instructions are more likely to function properly)
    volatile pcb_t* owner;                      // Mutex owner
    process_queue_t waitQueue;                  // Mutex wait queue
} mutex_t;

#define MUTEX_INIT (mutex_t){ false, NULL, { NULL, NULL } }

volatile pcb_t* GetCurrentProcess(void);
void SwitchProcess(bool kill, struct Registers* regs);
void SwitchToSpecificProcess(volatile pcb_t* process, struct Registers* regs);
void DestroyProcess(volatile pcb_t* process);
volatile pcb_t* CreateProcess(void (*entryPoint)(void), char* name, char* directory, uid owner, bool priveliged, bool kernel, bool foreground, priority_t priority, uint64_t timeSlice, volatile pcb_t* parent);
void Scheduler(void);
void SetCurrentProcess(volatile pcb_t* process);

MUTEXSTATUS PeekMutex(mutex_t* mutex);
MUTEXSTATUS MutexLock(mutex_t* mutex);
MUTEXSTATUS MutexUnlock(mutex_t* mutex);

#endif // MULTITASKING_H