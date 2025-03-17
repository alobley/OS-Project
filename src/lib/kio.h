#ifndef KIO_H
#define KIO_H

#include <stddef.h>
#include <stdbool.h>
#include <vfs.h>
#include <kernel.h>

// Contains the bridge between the kernel's I/O system and the userland STDIO system

#define FILE_FLAG_READ 0x01
#define FILE_FLAG_WRITE 0x02
#define FILE_FLAG_APPEND 0x04

#define FILE_MAGIC 0xDADCAFE

// Define file types and file descriptors
typedef struct STDIO_FILE {
    int fd;
    char* buffer;
    size_t bufSize;
    size_t pos;
    int flags;

    vfs_node_t* node;
} FILE;


extern FILE _stdin;
extern FILE _stdout;
extern FILE _stderr;

#define stdin (&_stdin)
#define stdout (&_stdout)
#define stderr (&_stderr)

// Stub for creating the STDIO entries in the VFS and other initialization (WIP)
#define InitStdio() 

#endif // KIO_H