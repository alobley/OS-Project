#ifndef STDDEF_H
#define STDDEF_H

typedef signed int ptrdiff_t;              // The difference between two pointers
typedef unsigned int size_t;               // The size of an object
typedef signed int ssize_t;                // The size of an object (signed - not an official part of this header)

typedef union {
    long long ll;
    long double ld;
} max_align_t;

typedef short wchar_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define offsetof(type, member) ((size_t) &((type*)0)->member)

#endif