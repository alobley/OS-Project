#include <system.h>
#include <kernel.h>

typedef unsigned int _ADDRESS;
#define getresult(x) asm volatile("mov %%eax, %0" : "=r" (x));

int sys_debug(void){
    int result = 0;
    do_syscall(SYS_DBG, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int install_keyboard_handler(KeyboardCallback callback){
    int result = 0;
    do_syscall(SYS_INSTALL_KBD_HANDLE, (_ADDRESS)callback, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int remove_keyboard_handler(KeyboardCallback callback){
    int result = 0;
    do_syscall(SYS_REMOVE_KBD_HANDLE, (_ADDRESS)callback, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int install_timer_handler(timer_callback_t callback, unsigned int interval){
    int result = 0;
    do_syscall(SYS_INSTALL_TIMER_HANDLE, (_ADDRESS)callback, (unsigned int)interval, 0, 0, 0);
    getresult(result);
    return result;
}

int remove_timer_handler(timer_callback_t callback){
    int result = 0;
    do_syscall(SYS_REMOVE_TIMER_HANDLE, (_ADDRESS)callback, 0, 0, 0, 0);
    getresult(result);
    return result;
}

FILESTATUS write(int fd, const void* buf, unsigned int count){
    int result = 0;
    do_syscall(SYS_WRITE, fd, (_ADDRESS)buf, count, 0, 0);
    getresult(result);
    return result;
}

FILESTATUS read(int fd, void* buf, unsigned int count){
    int result = 0;
    do_syscall(SYS_READ, fd, (_ADDRESS)buf, count, 0, 0);
    getresult(result);
    return result;
}

void exit(int status){
    do_syscall(SYS_EXIT, status, 0, 0, 0, 0);
}

int fork(void){
    int result = 0;
    do_syscall(SYS_FORK, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int exec(const char* path, const char* argv[], const char* envp[], int argc){
    int result = 0;
    do_syscall(SYS_EXEC, (_ADDRESS)path, (_ADDRESS)argv, (_ADDRESS)envp, argc, 0);
    getresult(result);
    return result;
}

int waitpid(pid_t pid){
    int result = 0;
    do_syscall(SYS_WAIT_PID, pid, 0, 0, 0, 0);
    getresult(result);
    return result;
}

pid_t getpid(void){
    int result = 0;
    do_syscall(SYS_GET_PID, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

pid_t getppid(void){
    pid_t result = 0;
    do_syscall(SYS_GET_PPID, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int getcwd(char* buf, unsigned int size){
    int result = 0;
    do_syscall(SYS_GETCWD, (_ADDRESS)buf, size, 0, 0, 0);
    getresult(result);
    return result;
}

int chdir(const char* path){
    int result = 0;
    do_syscall(SYS_CHDIR, (_ADDRESS)path, 0, 0, 0, 0);
    getresult(result);
    return result;
}

file_result open(const char* path){
    file_result result = {0, 0};
    do_syscall(SYS_OPEN, (_ADDRESS)path, 0, 0, 0, 0);
    getresult(result.status);
    asm volatile("mov %%ebx, %0" : "=r" (result.fd));
    return result;
}

FILESTATUS close(int fd){
    FILESTATUS result = 0;
    do_syscall(SYS_CLOSE, fd, 0, 0, 0, 0);
    getresult(result);
    return result;
}

unsigned int seek(int fd, unsigned int* offset, int whence){
    int result = 0;
    do_syscall(SYS_SEEK, fd, offset, whence, 0, 0);
    getresult(result);
    return result;
}

int sleep(unsigned long long milliseconds){
    int result = 0;
    do_syscall(SYS_SLEEP, (unsigned int)(milliseconds & 0xFFFFFFFF), (unsigned int)((milliseconds >> 32) & 0xFFFFFFFF), 0, 0, 0);
    getresult(result);
    return result;
}

int gettime(datetime_t* datetime){
    int result = 0;
    do_syscall(SYS_GET_TIME, datetime, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int kill(pid_t pid){
    int result = 0;
    do_syscall(SYS_KILL, pid, 0, 0, 0, 0);
    getresult(result);
    return result;
}

void yield(void){
    do_syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

int mmap(void* addr, unsigned int length, unsigned int flags){
    int result = 0;
    do_syscall(SYS_MMAP, (unsigned int)addr, length, flags, 0, 0);
    getresult(result);
    return result;
}

int munmap(void* addr, unsigned int length){
    int result = 0;
    do_syscall(SYS_MUNMAP, (unsigned int)addr, length, 0, 0, 0);
    getresult(result);
    return result;
}

int brk(unsigned int size){
    int result = 0;
    do_syscall(SYS_BRK, size, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int mprotect(void* addr, unsigned int length, unsigned int flags){
    int result = 0;
    do_syscall(SYS_MPROTECT, (_ADDRESS)addr, length, flags, 0, 0);
    getresult(result);
    return result;
}

int sys_dumpregs(){
    int result = 0;
    do_syscall(SYS_REGDUMP, 0, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int sysinfo(struct sysinfo* info){
    int result = 0;
    do_syscall(SYS_SYSINFO, info, 0, 0, 0, 0);
    getresult(result);
    return result;
}

int open_device(char* path, user_device_t* device){
    int result = 0;
    do_syscall(SYS_OPEN_DEVICE, (uintptr_t)path, (uintptr_t)device, 0, 0, 0);
    getresult(result);
    return result;
}

int device_read(int deviceID, void* buffer, unsigned int size){
    int result = 0;
    do_syscall(SYS_DEVICE_READ, deviceID, (_ADDRESS)buffer, size, 0, 0);
    getresult(result);
    return result;
}

int device_write(int deviceID, const void* buffer, unsigned int size){
    int result = 0;
    do_syscall(SYS_DEVICE_WRITE, deviceID, (_ADDRESS)buffer, size, 0, 0);
    getresult(result);
    return result;
}

int shutdown(void){
    int result = 0;
    do_syscall(SYS_SHUTDOWN, 0, 0, 0, 0, 0);
    getresult(result);
    return result;              // In case shutdown fails
}

int reboot(void){
    int result = 0;
    do_syscall(SYS_REBOOT, 0, 0, 0, 0, 0);
    getresult(result);
    return result;              // In case reboot fails
}