#include <vesa.h>

void DetectVesa(multiboot_info_t* mbootInfo){
    if(mbootInfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO){
        fb_t* fb = (fb_t*)halloc(sizeof(fb_t));
        fb->enabled = true;
        fb->fbwidth = mbootInfo->framebuffer_width;
        fb->fbheight = mbootInfo->framebuffer_height;
        fb->fbbpp = mbootInfo->framebuffer_bpp;
        fb->linearfbaddr = (void*)mbootInfo->framebuffer_addr;
        printf("VESA framebuffer detected: %dx%d, %d bpp\n", fb->fbwidth, fb->fbheight, fb->fbbpp);
    }else{
        printf("No VESA framebuffer detected\n");
    }

}