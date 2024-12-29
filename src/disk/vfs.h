#ifndef VFS_H
#define VFS_H

#include <types.h>
#include <util.h>

typedef struct PACKED File {
    char* name;
    void* data;
} file_t;

#endif