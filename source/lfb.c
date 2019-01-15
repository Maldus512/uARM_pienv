#include "uart.h"
#include "utils.h"
#include "mailbox.h"
#include "lfb.h"

/* PC Screen Font as used by Linux Console */
typedef struct {
    unsigned int  magic;
    unsigned int  version;
    unsigned int  headersize;
    unsigned int  flags;
    unsigned int  numglyph;
    unsigned int  bytesperglyph;
    unsigned int  height;
    unsigned int  width;
    unsigned char glyphs;
} __attribute__((packed)) psf_t;
extern volatile unsigned char _binary_font_psf_start;

unsigned int   width, height, pitch;
unsigned char *lfb;

void horizontal_line(int y) {
    int offs = (y * pitch);
    int i;
    for (i = 0; i < width; i++) {
        *((unsigned int *)(lfb + offs)) = 0xFFFFFF;
        offs += 4;
    }
}

void vertical_line(int x) {
    int offs = x * 4;
    int i;

    for (i = 0; i < height; i++) {
        *((unsigned int *)(lfb + offs)) = 0xFFFFFF;
        offs += pitch;
    }
}

void terminal_grid() {
    horizontal_line(height / 2);
    vertical_line(width / 2);
}

void screen_resolution(int *w, int *h, int *p) {
    *w = width;
    *h = height;
    *p = pitch;
}


/**
 * Set screen resolution to 1024x768
 */
void lfb_init() {
    char string[64];
    mbox[0] = 35 * 4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = MBOX_TAG_SET_PHY_SIZE;     // set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = REQUESTED_WIDTH;      // FrameBufferInfo.width
    mbox[6] = REQUESTED_HEIGHT;     // FrameBufferInfo.height

    mbox[7]  = MBOX_TAG_SET_VIRT_SIZE;     // set virt wh
    mbox[8]  = 8;
    mbox[9]  = 8;
    mbox[10] = REQUESTED_WIDTH;      // FrameBufferInfo.virtual_width
    mbox[11] = REQUESTED_HEIGHT;     // FrameBufferInfo.virtual_height

    mbox[12] = MBOX_TAG_SET_VIRT_OFFSET;     // set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;     // FrameBufferInfo.x_offset
    mbox[16] = 0;     // FrameBufferInfo.y.offset

    mbox[17] = MBOX_TAG_SET_DEPTH;     // set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32;     // FrameBufferInfo.depth

    mbox[21] = MBOX_TAG_SET_PIXEL_ORDER;     // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1;     // RGB, not BGR preferably

    mbox[25] = MBOX_TAG_GET_FB;     // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096;     // FrameBufferInfo.pointer
    mbox[29] = 0;        // FrameBufferInfo.size

    mbox[30] = MBOX_TAG_GET_PITCH;     // get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;     // FrameBufferInfo.pitch

    mbox[34] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF;
        width  = mbox[5];
        height = mbox[6];
        pitch  = mbox[33];     // 5120
        lfb    = (void *)((unsigned long)mbox[28]);
        strcpy(string, "Framebuffer address: ");
        itoa((long unsigned) lfb, &string[strlen(string)], 16);
        LOG(INFO, string);
        terminal_grid();
    } else {
        LOG(ERROR, "Unable to set screen resolution to 1024x768x32");
    }
}

/**
 * Display a string
 */
void lfb_print(int x, int y, char *s) {
    // get our font
    psf_t *font = (psf_t *)&_binary_font_psf_start;
    // draw next character if it's not zero
    while (*s) {
        // get the offset of the glyph. Need to adjust this to support unicode table
        unsigned char *glyph = (unsigned char *)&_binary_font_psf_start + font->headersize +
                               (*((unsigned char *)s) < font->numglyph ? *s : 0) * font->bytesperglyph;
        // calculate the offset on screen
        int offs = (y * font->height * pitch) + (x * (font->width + 1) * 4);
        // variables
        int i, j, line, mask, bytesperline = (font->width + 7) / 8;
        // handle carrige return
        if (*s == '\r') {
            x = 0;
        } else
            // new line
            if (*s == '\n') {
            x = 0;
            y++;
        } else {
            // display a character
            for (j = 0; j < font->height; j++) {
                // display one row
                line = offs;
                mask = 1 << (font->width - 1);
                for (i = 0; i < font->width; i++) {
                    // if bit set, we use white color, otherwise black
                    *((unsigned int *)(lfb + line)) = ((int)*glyph) & mask ? 0xFFFFFF : 0;
                    mask >>= 1;
                    line += 4;
                }
                // adjust to next line
                glyph += bytesperline;
                offs += pitch;
            }
            x++;
        }
        // next character
        s++;
    }
}


/**
 * Display a char
 */
void lfb_send(int x, int y, char c) {
    // get our font
    psf_t *font = (psf_t *)&_binary_font_psf_start;
    // draw next character if it's not zero
    // get the offset of the glyph. Need to adjust this to support unicode table
    unsigned char *glyph = (unsigned char *)&_binary_font_psf_start + font->headersize +
                           (c < font->numglyph ? c : 0) * font->bytesperglyph;
    // calculate the offset on screen
    int offs = (y * font->height * pitch) + (x * (font->width + 1) * 4);
    // variables
    int i, j, line, mask, bytesperline = (font->width + 7) / 8;
    // handle carrige return
    if (c != '\r' && c != '\n') {
        // display a character
        for (j = 0; j < font->height; j++) {
            // display one row
            line = offs;
            mask = 1 << (font->width - 1);
            for (i = 0; i < font->width; i++) {
                // if bit set, we use white color, otherwise black
                *((unsigned int *)(lfb + line)) = ((int)*glyph) & mask ? 0xFFFFFF : 0;
                mask >>= 1;
                line += 4;
            }
            // adjust to next line
            glyph += bytesperline;
            offs += pitch;
        }
    }
}
