#include <users.h>

user_t* CreateUser(char* username, char* password, uid id){
    user_t* user = (user_t*)halloc(sizeof(user_t) + strlen(password) + 1);
    memset(user, 0, sizeof(user_t) + strlen(password) + 1);
    user->id = id;
    if(strlen(username) > MAX_USERNAME_LENGTH){
        return NULL;            // Do not create user if username is too long
    }
    strcpy(user->username, username);
    strcpy(user->password, password);
    return user;
}