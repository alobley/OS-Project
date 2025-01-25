#include <bootloader.h>
#include <util.h>

NORETURN void kernel_main(bootutil_t* bu){
    uint32_t e = bu->screenBpp;
    for(int i = 0; i < 10; i++){
        e = i;
        i = e;
    }

    for(;;);
}