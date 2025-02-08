#ifndef STDDEF_H
<<<<<<< HEAD
#define STDDEF_H 1

typedef signed long long ptrdiff_t;                             // Signed difference between two pointers
typedef unsigned long long size_t;                              // Size of an object
typedef signed long long ssize_t;                               // Size of an object (signed)
=======
#define STDDEF_H

typedef signed long ptrdiff_t;              // The difference between two pointers
typedef unsigned long size_t;               // The size of an object
typedef signed long ssize_t;                // The size of an object (signed - not an official part of this header)
>>>>>>> a9a2e67 (Rewrote again, back at 32-bit and implemented a fully functional paging setup that is totally flawless and has no flaws at all)

typedef union {
    long long ll;
    long double ld;
<<<<<<< HEAD
} max_align_t;                                                  // Maximum alignment

typedef char wchar_t;                                           // Wide character type (just ASCII for now)

#define NULL ((void*)0)                                         // Null pointer constant

#define offsetof(type, member) ((size_t) &((type*)0)->member)   // Offset of a structure member
=======
} max_align_t;

typedef short wchar_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define offsetof(type, member) ((size_t) &((type*)0)->member)
>>>>>>> a9a2e67 (Rewrote again, back at 32-bit and implemented a fully functional paging setup that is totally flawless and has no flaws at all)

#endif