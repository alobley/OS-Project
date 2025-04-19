#include <tty.h>
#include <console.h>
#include <multitasking.h>
#include <kernel.h>
#include <drivers.h>
#include <keyboard.h>

#define DEFAULT_TTY_WIDTH 80
#define DEFAULT_TTY_HEIGHT 25
#define DEFAULT_TTY_SIZE (DEFAULT_TTY_WIDTH * DEFAULT_TTY_HEIGHT)

// TODO: reimplement