#ifndef VESA_H
#define VESA_H

#include <devices.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel.h>
#include <multiboot.h>
#include <util.h>

// VESA driver

typedef struct {
    bool enabled;
    uint16_t fbwidth;
    uint16_t fbheight;
    uint16_t fbbpp;
    void* linearfbaddr;
} fb_t;

struct vbe_info_block {
    uint8_t signature[4];
    uint16_t version;
    uint32_t oem_ptr;
    uint8_t capabilities[4];
    uint32_t video_mode_ptr;
    uint16_t total_memory;
    uint16_t oem_software_rev;
    uint32_t oem_vendor_name_ptr;
    uint32_t oem_product_name_ptr;
    uint32_t oem_product_rev_ptr;
    uint8_t reserved[222];
    uint8_t oem_data[256];
} PACKED;

struct vbe_mode_info {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;
    void* framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved[206];
} PACKED;

extern fb_t framebuffer;

#endif // VESA_H