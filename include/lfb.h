#ifndef __LFB_H__
#define __LFB_H__

#define REQUESTED_WIDTH  1280
#define REQUESTED_HEIGHT 960

#define FONT_WIDTH       8
#define FONT_HEIGHT      16

void lfb_send(int x, int y, char c);
void screen_resolution(int *w, int *h, int *p);

#endif