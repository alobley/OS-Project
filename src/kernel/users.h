#ifndef USERS_H
#define USERS_H

#include <stdint.h>
#include <stddef.h>

// For priveliged or unpriveliged users for the kernel to determine what system calls they can make

#define MAX_USERNAME_LENGTH 32
#define KERNEL_UID 0                    // Maximum possible privelige
#define ROOT_UID 1                      // Root user ID

typedef uint32_t uid;

typedef struct User {
    uid id;
    char username[MAX_USERNAME_LENGTH];
    char password[];
} user_t;

user_t* CreateUser(char* username, char* password, uid id);

#endif // USERS_H