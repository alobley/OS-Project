#include <tty.h>
#include <console.h>
#include <multitasking.h>
#include <kernel.h>
#include <drivers.h>
#include <keyboard.h>

#define DEFAULT_TTY_WIDTH 80
#define DEFAULT_TTY_HEIGHT 25
#define DEFAULT_TTY_SIZE (DEFAULT_TTY_WIDTH * DEFAULT_TTY_HEIGHT)

tty_t* activeTTY = NULL;
vfs_node_t* ttyNode = NULL;


// TODO: move these into the tty struct so each TTY has its own data
volatile bool readComplete = false;
volatile bool keyPressed = false;
volatile char lastKey = 0;
size_t readChars = 0;

void ttykeypress(KeyboardEvent_t event){
    if(event.keyUp || event.ascii == 0){
        return;
    }
    if(event.scanCode == ENTER){
        readComplete = true;
    }else{
        readComplete = false;
    }
    if(event.ascii != '\b'){
        readChars++;
    }
    if(readChars > 0){
        TTYWrite(activeTTY, &event.ascii, 1);
    }
    //ttyNode->offset += 1;
    keyPressed = true;
    lastKey = event.ascii;
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
    MoveCursor(tty->cursorX, tty->cursorY);
    return 0;
}

// Scroll the TTY up or down
void TTYScroll(tty_t* tty){
    for(uint16_t i = 0; i < tty->width * (tty->height - 1); i++){
        tty->buffer[i] = tty->buffer[i + tty->width];
    }
    for(uint16_t i = tty->width * (tty->height - 1); i < tty->width * tty->height; i++){
        tty->buffer[i] = 0;
    }
}

int TTYWrite(tty_t* tty, const char* text, size_t size){
    if(tty == NULL || text == NULL || size == 0){
        return -1;
    }

    if(strcmp(text, ANSI_ESCAPE) == 0){
        TTYClear(tty);
        return 0;
    }

    ttyNode->offset++;
    if(ttyNode->offset >= ttyNode->size){
        ttyNode->offset = 0;
        return -1;
    }

    //printf("Writing key: %c\n", *text);
    size_t actuallIterations = 0;
    for(size_t i = 0; i < size; i++){
        if(text[i] == '\b'){
            if(tty->cursorX > 0){
                tty->cursorX--;
            }else{
                if(tty->cursorY > 0){
                    tty->cursorY--;
                    tty->cursorX = tty->width - 1;
                }
            }
            if(readChars > 0){
                readChars--;
            }
            //printf("ReadChars: %d\n", readChars);
            //printf(tty->buffer + ttyNode->offset);
            //printf("Offset: %u\n", ttyNode->offset);
            if(ttyNode->offset > 0){
                ttyNode->offset--;
            }
            tty->buffer[ttyNode->offset] = 0;
            if(ttyNode->offset > 0){
                ttyNode->offset--;
            }
        }else if(text[i]){
            tty->buffer[ttyNode->offset] = text[i];
        }
        tty->cursorX++;
        while(tty->cursorX >= tty->width){
            tty->cursorX -= tty->width;
            tty->cursorY++;
        }
        if(tty->cursorY >= tty->height){
            // Scroll the TTY
            TTYScroll(tty);
            ttyNode->offset -= tty->width;
            tty->cursorY = tty->height - 1;
        }
        if(tty->active){
            WriteChar(text[i]);
        }
        actuallIterations++;
        if(actuallIterations >= size){
            break;
        }
        ttyNode->offset++;
        if(ttyNode->offset >= ttyNode->size){
            ttyNode->offset = 0;
            return -1;
        }
    }

    //MoveCursor(tty->cursorX, tty->cursorY);

    //ttyNode->offset += size;
    return 0;
}

int TTYRead(tty_t* tty, char* buffer, size_t size){
    // Not implemented yet (should I just use keyboard handlers?)
    tty->buffer[ttyNode->offset] = 0;
    size_t originalOffset = ttyNode->offset + 1;
    lastKey = 0;
    readChars = 0;
    MutexLock(&tty->lock);
    while(!readComplete){
        if(readChars >= size){
            // The buffer is full during the read, end it prematurely
            readComplete = false;
            MutexUnlock(&tty->lock);
            return readChars;
        }
    }
    readComplete = false;
    //ttyNode->offset++;

    //printf(&tty->buffer[originalOffset]);

    //printf("Buffer will return: %s\n", buffer);
    MutexUnlock(&tty->lock);
    memcpy(buffer, &tty->buffer[originalOffset], size);
    //printf("%s\n", &tty->buffer[originalOffset]);
    //printf("%s\n", buffer);
    size_t result = readChars;
    keyPressed = false;
    lastKey = 0;
    readChars = 0;
    return result;
}

void UpdateTTY(){
    for(size_t i = 0; i < activeTTY->size; i++){
        WriteChar(activeTTY->buffer[i]);
    }
}

tty_t* CreateTTY(char* name, vfs_node_t* node){
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
    tty->node = node;

    return tty;
}

void InitializeTTY(void){
    // Driver for the TTY (can just be built into the kernel)
    driver_t* ttyDriver = CreateDriver("TTY Driver", "A simple TTY driver", 1, DEVICE_TYPE_CHAR, NULL, NULL, NULL);

    // Register the driver (this will cause an error since we aren't using the actual device_t struct) (should I use it?)
    module_load(ttyDriver, NULL);

    // Add the TTY to the VFS
    ttyNode = VfsMakeNode("tty0", false, false, false, true, DEFAULT_TTY_SIZE, 0755, ROOT_UID, NULL);
    VfsAddChild(VfsFindNode("/dev"), ttyNode);

    tty_t* tty = CreateTTY("tty0", ttyNode);
    ttyNode->data = tty;

    activeTTY = tty;
    ttyNode = tty->node;
    tty->active = true;
    if(install_keyboard_handler(ttykeypress) != STANDARD_SUCCESS){
        printf("Failed to install keyboard handler!\n");
        STOP
    }

    // Assign STDOUT's buffer to the TTY buffer
    vfs_node_t* stdout = VfsFindNode("/dev/stdout");
    stdout->size = activeTTY->size;
    stdout->data = activeTTY;

    // Assign STDIN's buffer to the TTY buffer
    vfs_node_t* stdin = VfsFindNode("/dev/stdin");
    stdin->size = activeTTY->size;
    stdin->data = activeTTY;

    // Assign STDERR to the TTY buffer
    vfs_node_t* stderr = VfsFindNode("/dev/stderr");
    stderr->size = activeTTY->size;
    stderr->data = activeTTY;
}

tty_t* GetActiveTTY(void){
    return activeTTY;
}

bool IsTTY(tty_t* tty){
    // Search if a given pointer is a TTY
    if(tty == activeTTY){
        return true;
    }
    tty_t* saved = activeTTY;
    SetActiveTTYByNum(0);
    tty_t* current = activeTTY;
    SetActiveTTYByNum(saved->ttyNum);
    while(current != NULL){
        if(current == tty){
            return true;
        }
        current = current->next;
    }
    return false;
}

void GeneralSetActiveTTY(tty_t* tty){
    // Set the active TTY to the one passed in
    if(activeTTY == tty){
        return;
    }
    activeTTY->active = false;
    activeTTY = tty;
    activeTTY->active = true;
    ttyNode = activeTTY->node;
    MoveCursor(0, 0);
    UpdateTTY();
    MoveCursor(tty->cursorX, tty->cursorY);

    // Remap STDIN, STDOUT, and STDERR to the new TTY
    // NOTE: Will need some kind of ownership system for the TTYs later

    // Assign STDOUT's buffer to the TTY buffer
    vfs_node_t* stdout = VfsFindNode("/dev/stdout");
    stdout->size = activeTTY->size;
    stdout->data = activeTTY;

    // Assign STDIN's buffer to the TTY buffer
    vfs_node_t* stdin = VfsFindNode("/dev/stdin");
    stdin->size = activeTTY->size;
    stdin->data = activeTTY;

    // Assign STDERR to the TTY buffer
    vfs_node_t* stderr = VfsFindNode("/dev/stderr");
    stderr->size = activeTTY->size;
    stderr->data = activeTTY;
}

int SetActiveTTYByNum(short ttyNum){
    if(activeTTY->ttyNum == ttyNum){
        return 0;
    }
    activeTTY->active = false;

    // Select the first TTY
    SetActiveTTYByName("/dev/tty0");
    tty_t* current = activeTTY;
    current->active = false;
    while(current != NULL){
        if(current->ttyNum == ttyNum){
            GeneralSetActiveTTY(current);
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
    GeneralSetActiveTTY(tty);
    return 0;
}

int SetActiveTTYByName(const char* path){
    vfs_node_t* node = VfsFindNode((char*)path);
    if(node == NULL){
        return -1;
    }
    GeneralSetActiveTTY((tty_t*)node->data);
    return 0;
}