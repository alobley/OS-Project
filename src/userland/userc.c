#include <system.h>

void _start(){
    write(STDOUT_FILENO, "Hello, World!\n", 14);
    exit(0);
}