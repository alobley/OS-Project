#ifndef STDIO_H
#define STDIO_H

void printf(const char* format, ...);
int read(int fd, void* buf, unsigned int count);
int write(int fd, const void* buf, unsigned int count);

#endif