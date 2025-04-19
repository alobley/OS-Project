#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <stdint.h>
#include <stdbool.h>
#include <util.h>
#include <users.h>
#include <paging.h>
//#include <vfs.h>

typedef struct VFS_Node vfs_node_t;
typedef struct Driver driver_t;
typedef struct File_Table file_table_t;
typedef struct IPC_Pipe pipe_t;
typedef struct Pipe_Queue_Node pqnode;
typedef struct Pipe_Queue pqueue_t;

extern uint16_t numProcesses;

typedef struct Process_Control_Block pcb_t;

// A single node within a process queue
struct Queue_Node {
    pcb_t* process;
    struct Queue_Node* next;
    struct Queue_Node* prev;
};

// A queue of processes, most likely waiting for something
typedef struct Process_Queue {
    struct Queue_Node* first;
    struct Queue_Node* last;
} queue_t;

// A simple semaphore for thread safety
typedef struct Mutex {
    volatile unsigned int locked;
    pcb_t* owner;
    queue_t waitQueue;
} mutex_t;

// Time slice in milliseconds (is this enough?)
#define DEFAULT_TIMESLICE 5

typedef enum {
    KERNEL,                         // The process is the kernel
    RUNNING,                        // Process is actively running
    WAITING,                        // Process is waiting for a resource owned by another process or for another process to finish
    SLEEPING,                       // Process is sleeping for a specified amount of time
    STOPPED,                        // Process has halted execution
    DYING,                          // Process is currently being killed
    ZOMBIE,                         // Process has been killed but its status has not been collected by its parent
    DANGEROUS,                      // Process is detected as malicious or in an undefined state and should be destroyed immediately.
} Process_State;

// Signals are a bitmap of a uint32_t
#define NUM_SIGNALS 32
#define SIGTERM (1 << 0)            // Terminate immediately (cannot be overridden by the process)
#define SIGINT (1 << 1)             // Interrupt signal (Ctrl+C)
#define SIGSEV (1 << 2)             // Segmentation fault occurred
#define SIGCHLD (1 << 3)            // Child process has exited
#define SIGTIME (1 << 4)            // There was a timer interrupt (masked by default)
#define SIGPOLL (1 << 5)            // There was an update in a fd from the SYS_POLL system call
// More?

// For validating that PCBs are still valid (can be used to detect if the process holding a mutex is still valid, for example)
#define PCB_MAGIC 0xDEDEDEDE

#define DEFAULT_MASK (SIGTIME)

typedef void (*signal_handler_t)(int);

// Signals to be sent to a process upon a system call, interrupt return, or context switch. There should be default kernel-mode handlers.
// If the kernel-mode handlers are replaced by user-mode ones, the EIP of the iret should be pushed onto the user stack and EIP should be set to the handler
typedef struct SignalInfo {
    uint32_t pending;                           // Bitmap of pending signals to process
    signal_handler_t handlers[NUM_SIGNALS];     // The signal handlers for each signal
    uint32_t masked;                            // Whether each signal is masked (blocked). SIGTERM can't be masked.
} signal_info_t;

// Linked list of processes waiting for another process
struct Process_List {
    pcb_t* this;
    struct Process_List* next;
    struct Process_List* previous;
};

struct Resource_Limits {
    size_t maxHeapSize;                         // The maximum size of the heap for this process
    size_t maxStackSize;                        // The maximum size of the stack for this process
    size_t maxFileSize;                         // The maximum file size for this process
    size_t maxOpenFiles;                        // The maximum number of open files for this process
    size_t maxChildren;                         // The maximum number of children this process can have
    size_t maxMemUsage;                         // The maximum amount of memory this process can use
    uint64_t maxQuantum;                        // The maximum time slice this process can have
    size_t maxQueuedSignals;                    // The maximum number of queued signals this process can have
    size_t maxIPCQueueSize;                     // The maximum size of the IPC queue of this process
};
#define DEFAULT_MAX_HEAP USER_MEM_SIZE
#define DEFAULT_MAX_STACK 0x10000
#define DEFAULT_MAX_FILESIZE USER_MEM_SIZE
#define DEFAULT_MAX_OPENFILES 256
#define DEFAULT_MAX_CHILDREN 256
#define DEFAULT_MAX_MEM_USAGE USER_MEM_SIZE
#define DEFAULT_MAX_QUANTUM 10
#define DEFAULT_MAX_QUEUED_SIGNALS 256
#define DEFAULT_MAX_IPC_QUEUE_SIZE 256

#define RESOURCE_LIMITS(maxHeapSize, maxStackSize, maxFileSize, maxOpenFiles, maxChildren, maxMemUsage, maxQuantum, maxQueuedSignals, maxIPCQueueSize) \
    (struct Resource_Limits){maxHeapSize, maxStackSize, maxFileSize, maxOpenFiles, maxChildren, maxMemUsage, maxQuantum, maxQueuedSignals, maxIPCQueueSize}

#define DEFAULT_LIMITS \
    (struct Resource_Limits){DEFAULT_MAX_HEAP, USER_MEM_SIZE, DEFAULT_MAX_STACK, DEFAULT_MAX_FILESIZE, DEFAULT_MAX_OPENFILES, DEFAULT_MAX_CHILDREN, \
    DEFAULT_MAX_MEM_USAGE, DEFAULT_MAX_QUANTUM, DEFAULT_MAX_QUEUED_SIGNALS, DEFAULT_MAX_IPC_QUEUE_SIZE}

typedef struct {
    void* start;
    size_t length;
    uint32_t flags;
    void* end;
} memregion_t;

// Standard memory region flags
#define MEMREGION_SHARED (1 << 0)
#define MEMREGION_EXECUTABLE (1 << 1)
#define MEMREGION_READ (1 << 2)
#define MEMREGION_WRITE (1 << 3)
#define MEMREGION_MMIO (1 << 4)
#define MEMREGION_RESIZEABLE (1 << 5)
#define MEMREGION_FILE_MAP (1 << 6)
#define MEMREGION_KERNELSHARE ((1 << 7) | MEMREGION_READ | MEMREGION_WRITE)          // The memory region contains a buffer the kernel is sharing with the program

// Memory region flags available for programs to set for themselves
#define MEMREGION_AVAIL1 (1 << 8)
#define MEMREGION_AVAIL2 (1 << 9)
#define MEMREGION_AVAIL3 (1 << 10)
#define MEMREGION_AVAIL4 (1 << 11)
#define MEMREGION_AVAIL5 (1 << 12)

// Other memory region flags?

// The current priority of a running process
typedef char priority_t;

// These are just basic assignments and process priorities can be anywhere between them.
// Defines how likely the process is to be scheduled next (TBI)
#define MAX_PRIORITY 100
#define HIGH_PRIORITY 10
#define DEFAULT_PRIORITY 0
#define LOW_PRIORITY -10
#define DONT_SCHEDULE -100

#define KERNEL_PID 0
#define INIT_SID 1

#define REGION_KERNEL_STACK_INDEX 0
#define REGION_USER_STACK_INDEX 1
#define REGION_MMAP_INDEX 2
#define REGION_SHM_INDEX 3
#define REGION_HEAP_INDEX 4

typedef struct Process_Control_Block {
    unsigned int magic;                     // Magic number of the process
    Process_State state;                    // The current state of the process
    priority_t priority;                    // The priority of the process

    struct Process_List* waiting;           // The processes waiting for this one to exit

    struct Process_List* schedulerNode;     // The node in the scheduled process list that this process owns

    // If the process is sleeping, this contains the time it will stop sleeping.
    uint64_t sleepUntil;
    
    uint64_t timeSlice;                     // The time slice of this process

    struct Resource_Limits limits;          // The resource limits of this process

    // Memory info
    void (*EntryPoint)(void);               // The entry point of the program
    // 1 is kernel stack, 2 is user stack, 3 is mmap region, 4 is shared memory region, 5 is heap, and 6 and beyond are defined by the program's ELF data.
    memregion_t** regions;                  // A dynamic array of this process's memory regions
    size_t numRegions;                      // The length of the memory regions array

    pqueue_t pipes;                         // A queue of pipes into and out of the process

    size_t usedMemory;                      // The amount of memory this process is using

    // This will be allocated dynamically and is a virtual pointer to the page tables of the process
    pde_t* pageTables;

    // The context the CPU was in upon the latest switch away from this process
    struct Registers context;

    // This process is currently using a system call
    bool inSyscall;

    // The user and group that owns this process
    uid_t user;
    gid_t group;

    // I need to learn how these work, but it's probably a good idea to have them.
    uid_t effectiveUser;
    gid_t effectiveGroup;

    pid_t pid;                              // The process ID
    int exitStatus;                         // The status this process has exited with
    pid_t sessionId;                        // The session ID of this process
    pid_t processGroupId;                   // Process group for job control

    // The working directory of this process
    vfs_node_t* workingDirectory;

    // The name of this process
    char* processName;

    // Process hierarchy
    pcb_t* next;                            // Next process in the list of processes with the same parent as this one
    pcb_t* previous;                        // Previous process in the list of processes with the same parent as this one
    pcb_t* firstChild;                      // The first child process of this process
    pcb_t* parent;                          // The parent process of this process

    size_t numChildren;                     // The number of child processes this process has

    // The mutex the process is waiting on (if it is waiting on one)
    mutex_t* waitingFor;

    device_t* using;                        // The device this process is currently using

    file_table_t* fileTable;                // The file table for this process
    signal_info_t signalInfo;               // Signal information for this process
} pcb_t;

// Define the memory regions and their default sizes for user programs. Ensure there is padding between them all.

// NOTE: will need functions to change the sizes of these as they are resizeable.
#define KERNEL_STACK_SIZE 0x1000                                     // 4KiB should be enough for nearly all circumstances.
#define KERNEL_STACK_START (USER_MEM_END - KERNEL_STACK_SIZE)        // USER_MEM_END and PAGE_SIZE defined in paging.h

#define USER_STACK_SIZE 0x2000                                       // 8 KiB. Should be enough, but need to test with advanced programs.
#define USER_STACK_START (KERNEL_STACK_START - USER_STACK_SIZE - PAGE_SIZE)
#define USER_STACK_END (KERNEL_STACK_START - PAGE_SIZE)              // Add a page of padding so regions don't overlap

#define MMAP_END (USER_STACK_START - PAGE_SIZE)
#define MMAP_REGION_SIZE 0x200000                                    // 2 MiB for memory maps, such as files. Should be far and away beyond enough space for most cases.
#define MMAP_START (MMAP_END - MMAP_REGION_SIZE)    

#define SHM_END (MMAP_START - PAGE_SIZE)
#define SHM_REGION_SIZE 0x100000                                     // 1MiB for shared memory mapping. Should be gigantic for most cases.
#define SHM_START (MMAP_START - SHM_REGION_SIZE)

#define USER_HEAP_END (MMAP_START - PAGE_SIZE)                       // Last available address for the user heap (Don't use for setting initial limits)

// The rest is the program and its heap - this should give it ~2.7 GiB of virtual address space for everything else. Surely that's enough, right?

extern pcb_t* kernelPCB;

// An even simpler semaphore that is CPU-intensive but doesn't shut processes down
typedef struct Spinlock {
    volatile unsigned int locked;
    pcb_t* owner;
} spinlock_t;

#define SPINLOCK_INIT (spinlock_t){.locked = 0, .owner = NULL}

#define MUTEX_INIT (mutex_t){.locked = 0, .owner = NULL, .waitQueue.first = NULL, .waitQueue.last = NULL}

// Atomically test-and-set a lock (returns old value)
#define Atomic_Test_And_Set(lock) ({ \
    unsigned int _old; \
    asm volatile("lock xchgl %0, %1" \
        : "=r" (_old), "+m" (lock) \
        : "0"(1) \
        : "memory"); \
    _old; \
})

// Atomically reset a lock to 0 (returns old value)
#define Atomic_Test_And_Reset(lock) ({ \
    unsigned int _old; \
    asm volatile("lock xchgl %0, %1" \
        : "=r" (_old), "+m" (lock) \
        : "0"(0) \
        : "memory"); \
    _old; \
})

// Atomically check the value of a DWORD (int) sized lock
#define Atomic_Test(lock) ({ \
    unsigned int _val; \
    asm volatile("lock xaddl %0, %1" : "=r" (_val), "+m" (lock) : "0"(0) : "memory"); \
    _val; \
})

// WARNING: THIS WILL ALMOST NEVER GENERATE ERRORS. ENSURE YOU KNOW WHAT YOU'RE DOING.
#define UserPush(user_esp, value) *--((uint32_t*)(user_esp)) = (uint32_t)(value)

#define UserPop(user_esp) ({\
    unsigned int _val = *((uint32_t*)(user_esp))++; \
    _val; \
})

/// @brief Create a new process for scheduling
/// @param processName The name of the process
/// @param timeSlice The time slice of the process
/// @param limits The resource limits of the process
/// @param user The owner of the process (set to 0 to use the parent)
/// @param group The group owner of the process (must match the owner's group)
/// @param processGroup The process group this process is part of. If none, it will be set to INIT or the parent.
/// @param sid The session the process is part of. If 0, it will be set to INIT or the parent.
/// @param workingDirectory The working directory of the process
/// @param parent The parent process
/// @return Pointer to the new PCB  if successful, NULL otherwise.
pcb_t* CreateProcess(const char* processName, uint64_t timeSlice, struct Resource_Limits limits, uid_t user, gid_t group, pid_t processGroup, pid_t sid, vfs_node_t* workingDirectory, pcb_t* parent);

/// @brief Replace a process with a new program (keeps most of the PCB's data)
/// @param newName The name of the new process
/// @param data The data containing the new ELF executable
/// @param pcb The PCB to modify
/// @return Success or error code
int ReplaceProcess(char* newName, void* data, pcb_t* pcb, struct Registers* regs);

/// @brief Create an exact copy of a given process
/// @param this The process to duplicate
/// @return Pointer to the new PCB
pcb_t* DuplicateProcess(pcb_t* this);

// Destroy a process
int DestroyProcess(pcb_t* process);

// Emergencies only. Force-quit a process by any means necessary. Do not allow any more of its code to execute.
// WARNING: Could cause undefined behavior, memory leaks, or other issues.
int ObliterateProcess(pcb_t* maliciousProcess);

// Context switch to a new process
void ContextSwitch(struct Registers* regs, pcb_t* next);

/// @brief Add a new memory region to a process
/// @param process The process to map a region to
/// @param length The length of the memory region
/// @param flags The flags of the memory region
/// @param start Pointer to the start of the region. To allocate an arbitrary region, both start and end must be NULL.
/// @param end Pointer to the end of the region
/// @param removeMatches The function will remove all memory regions with the same flags as those provided
/// @return Success or error code
int AddMemoryRegion(pcb_t* process, size_t length, uint32_t flags, void* start, void* end, bool removeMatches);

/// @brief Remove a process's memory region based on a given index
/// @param process The process to modify
/// @param index The index of the memory region
/// @return Success or error code
int RemoveMemoryRegion(pcb_t* process, index_t index);

memregion_t* GetRegionWithFlags(pcb_t* process, uint32_t flags);

// Get the next scheduleable process (does not check state)
pcb_t* GetNextProcess(pcb_t* process);

// Schedule a process for execution
int ScheduleProcess(pcb_t* process);

// Stop a program's execution
void UnscheduleProcess(pcb_t* process);

// Load and execute a new program (kernel only, user processes should use exec/execfd)
int LoadExecute(pcb_t* process, vfs_node_t* data);

// Insert the signal handlers into the given process's call stack (EIP = first one, ESP = second, ESP + 4 = original EIP, for example)
int InsertSignals(pcb_t* process, unsigned int signal);

// This is actually a timer handler(?) that preemptively schedules processes
void Scheduler(struct Registers* state);

void DefaultSignalHandler();

message_queue_t* CreateMessageQueue(size_t capacity);

int SendMessage(message_queue_t* queue, void* data, size_t size);

int SetProcessGroup(pcb_t* process, pid_t pgid);
pcb_t* GetSessionLeader(pid_t sid);

// Bitmap functions that work well with any bitmap
void SetBitmapBit(uint32_t* bitmap, index_t bit);
void ClearBitmapBit(uint32_t* bitmap, index_t bit);

/// @brief Retrieve the currently active process
/// @return The current process
pcb_t* GetCurrentProcess();

void SetCurrentProcess(pcb_t* process);

int EnqueueProcess(mutex_t* lock, pcb_t* requester);

pcb_t* DequeueProcess(mutex_t* lock);

pcb_t* GetProcessByPID(pid_t pid);

int RemoveProcessFromQueue(pcb_t* process, queue_t* queue);

int MPeek(mutex_t* lock);

int MLock(mutex_t* lock, pcb_t* requester);

void MUnlock(mutex_t* lock);

int SpinPeek(spinlock_t* lock);

int SpinLock(spinlock_t* lock, pcb_t* requester);

void SpinUnlock(spinlock_t* lock);

void UpdateSignals(uint32_t signal);

void PrepareSignals(pcb_t* process);

#endif // MULTITASKING_H