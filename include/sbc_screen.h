#ifndef __SBC_SCREEN_H
#define __SBC_SCREEN_H

#include "sbc_defs.h"

bool allocateScreen(Pixel_Buffer_t *buffer);

void clearScreenArea(const int x, const int color, const int area);

void draw_line(const int x1, const int y1, const int x2, const int y2);

void draw_Hline(const int x, const int y, const int w, const int c);
void draw_Vline(const int x, const int y, const int h, const int c);

void draw_rect(const Rect_t r);
void fill_rect(const Rect_t r);

void paint_bevel(Rect_t r, const int c1, const int c2);

void print_font(char text, int x, int y, int color, int size);
void print_string(const char* text, const int x, const int y, const int c, const int size);
void print_string_shadow(const char* text, const int x, const int y, const int off[2], const int c[2], const int size);

void setScreenBuffer(Pixel_Buffer_t *buffer);
uint32_t** getScreenBuffer(void);
	
#endif /* __SBC_SCREEN_H */
