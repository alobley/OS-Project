#ifndef STRING_H
#define STRING_H

#include <stddef.h>

// Copy a block of memory to another location
static inline void* memcpy(void* dest, const void* src, unsigned long size){
    unsigned char* d = (unsigned char*) dest;
    unsigned char* s = (unsigned char*) src;
    for(unsigned long i = 0; i < size; i++){
        d[i] = s[i];
    }
    return dest;
}

// Move a block of memory
static inline void* memmove(void* dest, const void* src, unsigned long size){
    unsigned char* d = (unsigned char*) dest;
    unsigned char* s = (unsigned char*) src;
    if(d < s){
        for(unsigned long i = 0; i < size; i++){
            d[i] = s[i];
        }
    } else {
        for(unsigned long i = size; i > 0; i--){
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

// Copy one string to another
static inline char* strcpy(char* dest, const char* src){
    char* d = dest;
    while(*src){
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

// Copy a string, up to a certain number of characters
static inline char* strncpy(char* dest, const char* src, unsigned long size){
    char* d = dest;
    while(size-- && *src){
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

// Concatenate two strings
static inline char* strcat(char* dest, const char* src){
    char* d = dest;
    while(*d){
        d++;
    }
    while(*src){
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

// Append a string to another string, up to a certain number of characters
static inline char* strncat(char* dest, const char* src, unsigned long size){
    char* d = dest;
    while(*d){
        d++;
    }
    while(size-- && *src){
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

// Compare two memory blocks
static inline int memcmp(const void* ptr1, const void* ptr2, unsigned long size){
    const unsigned char* p1 = (const unsigned char*) ptr1;
    const unsigned char* p2 = (const unsigned char*) ptr2;
    for(unsigned long i = 0; i < size; i++){
        if(p1[i] != p2[i]){
            return p1[i] - p2[i];
        }
    }
    return 0;
}

// Compare two strings
static inline int strcmp(const char* str1, const char* str2){
    while(*str1 && *str2 && *str1 == *str2){
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

// Compare two strings, up to a certain number of characters
static inline int strncmp(const char* str1, const char* str2, unsigned long size){
    size--;                 // Bug fix
    while(size-- && *str1 && *str2 && *str1 == *str2){
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

// Compare two strings, ignoring case
static inline size_t strxfrm(char* dest, const char* src, size_t size){
    size_t i = 0;
    while(i < size && *src){
        dest[i++] = *src++;
    }
    dest[i] = '\0';
    return i;
}

// Locate a character in a string (or locate a given value in a memory block)
static inline void* memchr(const void* ptr, int value, unsigned long size){
    const unsigned char* p = (const unsigned char*) ptr;
    for(unsigned long i = 0; i < size; i++){
        if(p[i] == value){
            return (void*) &p[i];
        }
    }
    return NULL;
}

// Locate the first occurrence of a character in a string
static inline char* strchr(const char* str, int value){
    while(*str){
        if(*str == value){
            return (char*) str;
        }
        str++;
    }
    return (char*)NULL;
}

// Locate the last occurrence of a character in a string
static inline char* strrchr(const char* str, int value){
    const char* last = (char*)NULL;
    while(*str){
        if(*str == value){
            last = str;
        }
        str++;
    }
    return (char*) last;
}

// Locate the last occurrence of a character in a string
static inline size_t strcspn(const char* str1, const char* str2){
    size_t i = 0;
    while(str1[i]){
        if(strchr(str2, str1[i])){
            return i;
        }
        i++;
    }
    return i;
}

// Locate the first occurrence of any character in a string
static inline char* strpbrk(const char* str1, const char* str2){
    while(*str1){
        if(strchr(str2, *str1)){
            return (char*) str1;
        }
        str1++;
    }
    return (char*) NULL;
}

// Get the length of a string
static inline size_t strspn(const char* str1, const char* str2){
    size_t i = 0;
    while(str1[i] && strchr(str2, str1[i])){
        i++;
    }
    return i;
}

// Locate the first occurrence of the substring needle in the string haystack
static inline char* strstr(const char* haystack, const char* needle){
    while(*haystack){
        if(*haystack == *needle){
            const char* h = haystack;
            const char* n = needle;
            while(*h && *n && *h == *n){
                h++;
                n++;
            }
            if(!*n){
                return (char*) haystack;
            }
        }
        haystack++;
    }
    return (char*) NULL;
}

// Tokenize a string
static inline char* strtok(char* str, const char* delim){
    static char* s = (char*)NULL;
    if(str){
        s = str;
    }
    if(!s || !*s){
        return (char*)NULL;
    }
    char* start = s;
    while(*s && strchr(delim, *s)){
        s++;
    }
    if(!*s){
        return (char*)NULL;
    }
    start = s;
    while(*s && !strchr(delim, *s)){
        s++;
    }
    if(*s){
        *s++ = '\0';
    }
    return start;
}

// Skipping strerror

// Set a block of memory to a specific value
static inline void memset(void* ptr, unsigned char value, unsigned long size){
    unsigned char* p = (unsigned char*) ptr;
    for(unsigned long i = 0; i < size; i++){
        p[i] = value;
    }
}

// Get the length of a string
static inline size_t strlen(const char* str){
    size_t len = 0;
    while(*str++){
        len++;
    }
    return len;
}

static inline char toupper(char chr){
    if(chr >= 'a' && chr <= 'z'){
        chr = chr - 32;
    }

    return chr;
}

char* strdup(const char* str);

// Skipping strndup

#endif