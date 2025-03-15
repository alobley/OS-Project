#include <tty.h>
#include <console.h>
#include <multitasking.h>
#include <kernel.h>

#define DEFAULT_TTY_WIDTH 80
#define DEFAULT_TTY_HEIGHT 25
#define DEFAULT_TTY_SIZE (DEFAULT_TTY_WIDTH * DEFAULT_TTY_HEIGHT)

tty_t* activeTTY = NULL;

int TTYWrite(tty_t* tty, const char* text, size_t size){
    if(tty == NULL || text == NULL || size == 0){
        return -1;
    }

    MutexLock(&tty->lock);
    for(size_t i = 0; i < size; i++){
        tty->buffer[tty->cursorY * tty->width + tty->cursorX + i] = *text;
        tty->cursorX++;
        while(tty->cursorX >= tty->width){
            tty->cursorX -= tty->width;
            tty->cursorY++;
        }
        if(tty->cursorY >= tty->height){
            // Scroll the TTY
            Scroll();
            tty->cursorY = tty->height - 1;
        }
    }
    MutexUnlock(&tty->lock);
    for(size_t i = 0; i < size; i++){
        WriteChar(text[i]);
    }
    return 0;
}

int TTYRead(tty_t* tty, char* buffer, size_t size){
    // Not implemented yet (should I just use keyboard handlers?)
    return -1;
}

int TTYClear(tty_t* tty){
    if(tty == NULL){
        return -1;
    }
    MutexLock(&tty->lock);
    memset(tty->buffer, 0, tty->size);
    tty->cursorX = 0;
    tty->cursorY = 0;
    ClearScreen();
    MutexUnlock(&tty->lock);
    return 0;
}

void UpdateTTY(){
    for(size_t i = 0; i < strlen(activeTTY->buffer); i++){
        WriteChar(activeTTY->buffer[i]);
    }
}

tty_t* CreateTTY(char* name){
    tty_t* tty = (tty_t*)halloc(sizeof(tty_t));
    memset(tty, 0, sizeof(tty_t));
    activeTTY = tty;
    tty->buffer = (char*)halloc(DEFAULT_TTY_SIZE);
    memset(tty->buffer, 0, DEFAULT_TTY_SIZE);
    tty->lock = MUTEX_INIT;
    tty->size = DEFAULT_TTY_SIZE;
    tty->width = DEFAULT_TTY_WIDTH;
    tty->height = DEFAULT_TTY_HEIGHT;
    tty->devName = name;
    tty->write = TTYWrite;
    tty->clear = TTYClear;

    return tty;
}

void InitializeTTY(void){
    tty_t* tty = CreateTTY("tty0");

    // Driver for the TTY (can just be built into the kernel)
    driver_t* ttyDriver = CreateDriver("TTY Driver", "A simple TTY driver", 0, 1, NULL, NULL, NULL);

    // Register the driver (this will cause an error since we aren't using the actual device_t struct) (should I use it?)
    do_syscall(SYS_MODULE_LOAD, (uint32_t)ttyDriver, 0, 0, 0, 0);

    do_syscall(SYS_ADD_VFS_DEV, (uint32_t)tty, (uint32_t)"tty0", (uint32_t)"/dev", 0, 0);
    
    //vfs_node_t* tty0 = VfsMakeNode("tty0", false, 0, 0755, ROOT_UID, tty);
    //VfsAddChild(VfsFindNode("/dev"), tty0);
}

tty_t* GetActiveTTY(void){
    return activeTTY;
}

int SetActiveTTYByNum(short ttyNum){
    if(activeTTY->ttyNum == ttyNum){
        return 0;
    }
    tty_t* current = activeTTY;
    while(current != NULL){
        if(current->ttyNum == ttyNum){
            activeTTY = activeTTY->next;
            for(size_t i = 0; i < strlen(activeTTY->buffer); i++){
                WriteChar(activeTTY->buffer[i]);
            }
            return 0;
        }
        current = current->next;
    }
    return -1;
}

int SetActiveTTYByPtr(tty_t* tty){
    if(activeTTY == tty){
        return 0;
    }
    activeTTY = tty;
    for(size_t i = 0; i < strlen(activeTTY->buffer); i++){
        WriteChar(activeTTY->buffer[i]);
    }
    return 0;
}

int SetActiveTTYByName(const char* path){
    vfs_node_t* node = VfsFindNode((char*)path);
    if(node == NULL){
        return -1;
    }
    activeTTY = (tty_t*)node->data;
    for(size_t i = 0; i < strlen(activeTTY->buffer); i++){
        WriteChar(activeTTY->buffer[i]);
    }
    return 0;
}