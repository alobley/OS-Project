#ifndef TYPES_H
#define TYPES_H

// Boolean type
#ifndef bool
typedef unsigned char bool;
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

// Integers
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

// Size/pointers
typedef uint32 size_t;
typedef int32 ssize_t;
typedef uint32 uintptr_t;       // Convert pointers to this when they need to be used
typedef int32 intptr_t;         // For when you feel like using signed integers for memory addressing

// Floating-point
typedef float float32;
typedef double float64;

// Strings
typedef char* string;
typedef const char* cstring;

// Volatile types, typically for hardware access (i.e. MMIO)
typedef volatile uint32 reg32_t;
typedef volatile uint16 reg16_t;
typedef volatile uint8 reg8_t;

// NULL type
#ifndef NULL
#define NULL ((void*)0)
#endif

// low-level memory definitions
typedef uint8 byte;
typedef uint16 word;
typedef uint32 dword;


// Limits
#define UINT8_MAX 255
#define INT8_MAX 127
#define INT8_MIN -128
#define UINT16_MAX 65535
#define INT16_MAX 32767
#define INT16_MIN -32768
#define UINT32_MAX 4294967295
#define INT32_MAX 2147483647
#define INT32_MIN -2147483648
#define UINT64_MAX 18446744073709551615
#define INT64_MAX 9223372036854775807
#define INT64_MIN -9223372036854775808

#endif  // TYPES_H