#include <tty.h>
#include <console.h>
#include <multitasking.h>
#include <kernel.h>
#include <devices.h>
#include <keyboard.h>
#include <vfs.h>
#include <stdio.h>

#define DEFAULT_TTY_WIDTH 80
#define DEFAULT_TTY_HEIGHT 25
#define DEFAULT_TTY_SIZE (DEFAULT_TTY_WIDTH * DEFAULT_TTY_HEIGHT)

typedef struct TTY {
    char* name;                     // Name of the TTY
    framebuffer_t* fb;              // Framebuffer for the TTY
    char* stdBuffer;                // Buffer for standard input/output
    int cursor_x;                   // X position of the cursor
    int cursor_y;                   // Y position of the cursor
    int num;                        // The TTY's number
    bool active;                    // Whether this TTY is active or not
    bool reading;                   // Whether this TTY is currently reading input from STDIN
    device_t* this;                 // Pointer to the device this TTY is associated with
    uint16_t* buffer;               // Buffer for the TTY
} tty_t;

#define MAX_TTYS 10

static driver_t* this = NULL;                   // Pointer to the current driver
tty_t* ttys[MAX_TTYS] = {NULL};                 // Array of TTYs
static framebuffer_t* vgaFramebuffer = NULL;    // Pointer to the VGA framebuffer
static size_t numTTYs = 0;                      // Number of TTYs

static tty_t* activeTTY = NULL;                  // Pointer to the active TTY

char* ttyNames[] = {
    "tty0",
    "tty1",
    "tty2",
    "tty3",
    "tty4",
    "tty5",
    "tty6",
    "tty7",
    "tty8",
    "tty9"
};

volatile bool readComplete = false;
volatile char lastKey = 0;
size_t readChars = 0;

dresult_t ioctl(int cmd, void* arg, device_id_t device){
    // Handle IOCTL commands for the TTY
    switch(cmd){
        case 1: // Set the TTY active
            if(arg == NULL){
                return DRIVER_FAILURE;
            }
            activeTTY->active = false;
            ttys[*(uint16_t*)arg]->active = true;
            activeTTY = ttys[*(uint16_t*)arg];
            break;
        case 2: // Create a new TTY
            if(arg == NULL || numTTYs >= MAX_TTYS){
                return DRIVER_FAILURE;
            }
            break;
        default:
            return DRIVER_FAILURE;
    }
    return DRIVER_SUCCESS;
}

dresult_t TTYRead(device_id_t device, void* buf, size_t len, size_t offset){
    // Read from the TTY
    
    return DRIVER_SUCCESS;
}

dresult_t TTYClear(tty_t* tty){
    // Clear the TTY's framebuffer
    if(tty == NULL){
        return DRIVER_FAILURE;
    }
    
    tty->cursor_x = 0;
    tty->cursor_y = 0;
    
    // Clear the framebuffer
    for(size_t i = 0; i < DEFAULT_TTY_SIZE; i++){
        tty->buffer[i] = 0x0F00;
    }

    if(tty->active){
        // Update the framebuffer
        for(size_t i = 0; i < DEFAULT_TTY_SIZE; i++){
            ((uint16_t*)(vgaFramebuffer->address))[i] = tty->buffer[i];
        }
        MoveCursor(tty->cursor_x, tty->cursor_y);
    }
    
    return DRIVER_SUCCESS;
}

void TTYScroll(tty_t* tty){
    for(uint16_t i = 0; i < tty->fb->width * (tty->fb->height - 1); i++){
        tty->buffer[i] = tty->buffer[i + tty->fb->width];
    }
    for(uint16_t i = tty->fb->width * (tty->fb->height - 1); i < tty->fb->width * tty->fb->height; i++){
        tty->buffer[i] = 0;
    }
    tty->cursor_y--;
    tty->cursor_x = 0;

    for(int i = 0; i < tty->fb->width * tty->fb->height; i++){
        ((uint16_t*)(tty->fb->address))[i] = tty->buffer[i];
    }
}

dresult_t TTYWrite(device_id_t device, void* buf, size_t len, size_t offset){
    // Write to the TTY
    if(buf == NULL || len == 0){
        return DRIVER_FAILURE;
    }
    
    tty_t* tty = (tty_t*)GetDeviceByID(device)->driverData;
    if(tty == NULL){
        return DRIVER_FAILURE;
    }

    // Set the TTY cursor position to the offset
    if(offset >= DEFAULT_TTY_SIZE){
        return DRIVER_FAILURE;
    }

    uint16_t* buffer = (uint16_t*)buf;
    for(size_t i = 0; i < len; i++){
        if((buffer[i] & 0xFF) == '\n'){
            // Just update cursor position for newline, don't write to buffer
            tty->cursor_x = 0;
            tty->cursor_y++;
        }else if((buffer[i] & 0xFF) == '\b'){
            if(tty->cursor_x > 0){
                tty->cursor_x--;
            }else{
                tty->cursor_x = 0;
            }
            // Clear the character at current position
            tty->buffer[tty->cursor_y * DEFAULT_TTY_WIDTH + tty->cursor_x] = 0x0F00;

            if(tty->active){
                ((uint16_t*)(tty->fb->address))[tty->cursor_y * DEFAULT_TTY_WIDTH + tty->cursor_x] = 0x0F00;
            }
        }else{
            // Write the character to buffer and advance cursor
            // Check if we need to scroll
            tty->buffer[tty->cursor_y * DEFAULT_TTY_WIDTH + tty->cursor_x] = buffer[i];

            if(tty->active){
                ((uint16_t*)(tty->fb->address))[tty->cursor_y * DEFAULT_TTY_WIDTH + tty->cursor_x] = tty->buffer[tty->cursor_y * DEFAULT_TTY_WIDTH + tty->cursor_x];
            }

            tty->cursor_x++;
            if(tty->cursor_x >= DEFAULT_TTY_WIDTH){
                tty->cursor_x = 0;
                tty->cursor_y++;
            }
        }
    }

    // Check if we need to scroll
    while(tty->cursor_y >= DEFAULT_TTY_HEIGHT){
        TTYScroll(tty);
    }
    
    if(tty->active){
        // Update the cursor position
        MoveCursor(tty->cursor_x, tty->cursor_y);
    }
    
    return DRIVER_SUCCESS;
}

int startOffset = 0;
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
    }else{
        if(readChars > 0){
            readChars--;
        }
    }
    
    if(activeTTY->cursor_x > startOffset || readChars > 0){
        uint8_t buf[2];
        buf[0] = event.ascii;
        buf[1] = 0x0F;
        TTYWrite(activeTTY->this->id, (void*)&buf[0], 1, activeTTY->cursor_x + (activeTTY->cursor_y * DEFAULT_TTY_WIDTH));
        if(activeTTY->reading){
            activeTTY->stdBuffer[readChars - 1] = event.ascii;
        }
    }
    lastKey = event.ascii;
}

dresult_t StdinRead(device_id_t device, void* buf, size_t len, size_t offset){
    // Read from the standard input

    readComplete = false;
    readChars = 0;
    startOffset = activeTTY->cursor_x;
    activeTTY->reading = true;

    sti;  // Enable interrupts

    size_t actualReads = 0;
    while(!readComplete){
        // Wait for a key press
        if(lastKey != 0) {
            if(lastKey == '\b') {
                // Handle backspace
                if(actualReads > 0) {
                    actualReads--;
                    ((char*)buf)[actualReads] = 0;
                }
            } 
            else if(lastKey != '\n' && actualReads < len - 1) {
                // Store the character
                ((char*)buf)[actualReads++] = lastKey;
            }
            // Reset lastKey after processing
            lastKey = 0;
        }
    }
    
    // Add null terminator to make it a proper C string
    ((char*)buf)[actualReads] = '\0';
    
    return actualReads;
}

dresult_t StderrWrite(device_id_t device, void* buf, size_t len, size_t offset){
    // Write to the standard error
    if(buf == NULL || len == 0){
        return DRIVER_FAILURE;
    }
    
    return DRIVER_SUCCESS;
}

dresult_t StdoutWrite(device_id_t device, void* buf, size_t len, size_t offset){
    // Write to the standard output
    if(buf == NULL || len == 0){
        return DRIVER_FAILURE;
    }

    tty_t* tty = (tty_t*)GetDeviceByID(device)->driverData;

    if(strcmp(((char*)buf), ANSI_ESCAPE) == 0){
        TTYClear(tty);
        return DRIVER_SUCCESS;
    }

    char* buffer = (char*)buf;
    for(size_t i = 0; i < len; i++){
        char c[2] = {0};
        c[0] = buffer[i];
        c[1] = 0x0F;
        if(tty == NULL){
            return DRIVER_FAILURE;
        }
        
        TTYWrite(tty->this->id, (void*)&c[0], 1, offset + i);
    }
    
    return DRIVER_SUCCESS;
}


extern device_t* stdin;
extern device_t* stdout;
extern device_t* stderr;


int InitializeTTY(){
    // Initialize the TTY subsystem
    this = halloc(sizeof(driver_t));
    if(this == NULL){
        return DRIVER_FAILURE;
    }
    memset(this, 0, sizeof(driver_t));

    this->name = "TTY";
    this->class = DEVICE_CLASS_CHAR | DEVICE_CLASS_VIRTUAL;
    this->type = DEVICE_TYPE_TTY;
    this->init = InitializeTTY;
    this->deinit = NULL;
    this->probe = NULL;

    RegisterDriver(this, true);

    // Create the first TTY device
    device_t* ttyDevice = halloc(sizeof(device_t));
    if(ttyDevice == NULL){
        return DRIVER_FAILURE;
    }
    memset(ttyDevice, 0, sizeof(device_t));

    ttyDevice->name = ttyNames[0];
    ttyDevice->class = DEVICE_CLASS_CHAR | DEVICE_CLASS_VIRTUAL;
    ttyDevice->type = DEVICE_TYPE_TTY;
    ttyDevice->driver = this;
    ttyDevice->ops.read = TTYRead;
    ttyDevice->ops.write = TTYWrite;
    ttyDevice->ops.ioctl = ioctl;

    ttyDevice->driverData = halloc(sizeof(tty_t));
    if(ttyDevice->driverData == NULL){
        hfree(ttyDevice);
        return DRIVER_FAILURE;
    }
    memset(ttyDevice->driverData, 0, sizeof(tty_t));
    tty_t* tty = (tty_t*)ttyDevice->driverData;
    tty->name = ttyNames[0];
    tty->buffer = halloc(DEFAULT_TTY_SIZE * sizeof(uint16_t));
    if(tty->buffer == NULL){
        hfree(ttyDevice->driverData);
        hfree(ttyDevice);
        return DRIVER_FAILURE;
    }
    memset(tty->buffer, 0, DEFAULT_TTY_SIZE * sizeof(uint16_t));

    tty->this = ttyDevice;
    tty->num = 0;
    tty->active = true;
    tty->cursor_x = 0;
    tty->cursor_y = 0;
    ttys[tty->num] = tty;
    numTTYs++;
    tty->stdBuffer = halloc(DEFAULT_TTY_SIZE);                      // This is just a buffer of characters
    if(tty->stdBuffer == NULL){
        hfree(tty->buffer);
        hfree(ttyDevice->driverData);
        hfree(ttyDevice);
        return DRIVER_FAILURE;
    }
    memset(tty->stdBuffer, 0, DEFAULT_TTY_SIZE);

    vgaFramebuffer = halloc(sizeof(framebuffer_t));
    if(vgaFramebuffer == NULL){
        hfree(tty->buffer);
        hfree(ttyDevice->driverData);
        hfree(ttyDevice);
        return DRIVER_FAILURE;
    }
    memset(vgaFramebuffer, 0, sizeof(framebuffer_t));

    tty->fb = vgaFramebuffer;

    tty->fb->address = (void*)GetVGAAddress();
    tty->fb->size = DEFAULT_TTY_SIZE * sizeof(uint16_t);
    tty->fb->width = DEFAULT_TTY_WIDTH;
    tty->fb->height = DEFAULT_TTY_HEIGHT;
    tty->fb->pitch = DEFAULT_TTY_WIDTH * sizeof(uint16_t);
    tty->fb->bpp = 16;
    tty->fb->text = true;

    // These probably don't need to be set, but there's no harm in it
    tty->fb->red_mask_size = 5;
    tty->fb->green_mask_size = 6;
    tty->fb->blue_mask_size = 5;
    tty->fb->reserved_mask_size = 0;

    RegisterDevice(ttyDevice, "/dev/tty0", S_IROTH | S_IWOTH);
    ttyDevice->node->size = DEFAULT_TTY_SIZE * sizeof(uint16_t);

    TTYClear(tty);

    activeTTY = tty;

    stdin->driver = this;
    stdout->driver = this;
    stderr->driver = this;

    stdin->driverData = tty;
    stdout->driverData = tty;
    stderr->driverData = tty;

    stdin->node->size = DEFAULT_TTY_SIZE;
    stdout->node->size = DEFAULT_TTY_SIZE;
    stderr->node->size = DEFAULT_TTY_SIZE;

    stdin->ops.read = StdinRead;
    stdout->ops.write = StdoutWrite;
    stderr->ops.write = StderrWrite;

    InstallKeyboardCallback(ttykeypress);
    
    return DRIVER_SUCCESS;
}