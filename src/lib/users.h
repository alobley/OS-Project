#ifndef USERS_H
#define USERS_H

#include <stdint.h>
#include <stddef.h>

#include <system.h>

// For priveliged or unpriveliged users for the kernel to determine what system calls they can make

#define MAX_USERNAME_LENGTH 32
#define ROOT_UID 0                      // Root user ID

typedef struct User {
    uid_t id;
    gid_t group;
    char username[MAX_USERNAME_LENGTH];
    char* password;                     // Password hash
} user_t;

user_t* CreateUser(char* username, char* password, uid_t id);

#endif // USERS_H