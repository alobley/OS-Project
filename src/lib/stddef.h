#ifndef STDDEF_H
#define STDDEF_H 1

typedef signed long long ptrdiff_t;                             // Signed difference between two pointers
typedef unsigned long long size_t;                              // Size of an object
typedef signed long long ssize_t;                               // Size of an object (signed)

typedef union {
    long long ll;
    long double ld;
} max_align_t;                                                  // Maximum alignment

typedef char wchar_t;                                           // Wide character type (just ASCII for now)

#define NULL ((void*)0)                                         // Null pointer constant

#define offsetof(type, member) ((size_t) &((type*)0)->member)   // Offset of a structure member

#endif