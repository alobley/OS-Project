#ifndef USERS_H
#define USERS_H

#include <stdint.h>
#include <stddef.h>

// For priveliged or unpriveliged users for the kernel to determine what system calls they can make

#define MAX_USERNAME_LENGTH 32
#define ROOT_UID 0                      // Root user ID

typedef uint32_t uid;
typedef uint32_t gid;

typedef struct User {
    uid id;
    gid group;
    char username[MAX_USERNAME_LENGTH];
    char* password;                     // Password hash
} user_t;

user_t* CreateUser(char* username, char* password, uid id);

#endif // USERS_H