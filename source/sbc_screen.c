#include <string.h>

#include "sbc_utils.h"
#include "sbc_font.h"

#define SETPIX(x, y, c)  *(screen_buffer + (x) + (y) * PIXEL_WIDTH) = c
#define GETXPIX(x)       *(screen_buffer + (x))
#define GETPIX(x, y)     *(screen_buffer + (x) + (y) * PIXEL_WIDTH)

#define ABS(a)	 ((a) >  0 ? (a) : -(a))
#define SGN(a,b) ((a) < (b) ? 1 : -1)

static uint32_t* screen_buffer = NULL;
static int PIXEL_WIDTH = 0, PIXEL_HEIGHT = 0;

bool allocateScreen(Pixel_Buffer_t *pixel_buffer)
{
	SBC_FREE(pixel_buffer->buffer);

	pixel_buffer->buffer = malloc(pixel_buffer->width * pixel_buffer->height * sizeof *pixel_buffer->buffer);

	if (pixel_buffer->buffer == NULL) return false;

	return true;
}

void clearScreenArea(const int x, const int color, const int area)
{
	assert(screen_buffer != NULL);

	for(int i = x; i < area; i++)
	{
		if(i >= SCREEN_WIDTH * SCREEN_HEIGHT) return;
		*(screen_buffer + i) = color;
	}
}

void draw_line(const int x1, const int y1, const int x2, const int y2)
{
	assert (screen_buffer != NULL);

	if (x1 < 0 || y1 < 0)   return;
	if (x2 < 0 || y2 < 0)   return;
	if (x1 > SCREEN_WIDTH ) return;
	if (x2 > SCREEN_WIDTH ) return;
	if (y1 > SCREEN_HEIGHT) return;
	if (y2 > SCREEN_HEIGHT) return;

    const int get_pix = GETXPIX(x1) ;
	int x, y, dx, dy, sx, sy, error, e2, c;

    if (get_pix == SCROLLBACK) c = SBCLPURPLE;
    else c = SBCMPURPLE;

    x = x1;
    y = y1;

	dx = ABS(x2 - x1);
	sx = SGN(x1, x2);
	dy = ABS(y2 - y1);
	sy = SGN(y1, y2);

	error = dx - dy;

	while (1)
	{
		SETPIX(x, y, c);
		if (x == x2 && y == y2) break;
		e2 = error << 1;
		if (e2 >= -dy)
		{
			if (x == x2) break;
			error -= dy;
			x += sx;
		}
		if (e2 <= dx)
		{
			if (y == y2) break;
			error += dx;
			y += sy;
		}
	}
}

void draw_Hline(const int x, const int y, const int w, const int c)
{
	int draw_x = x, draw_w = x + w;

	assert (screen_buffer != NULL);
	if (y < 0 || y > SCREEN_HEIGHT) return;

	if (w < 0)
	{
		draw_x = draw_w;
		draw_w = draw_x - w;
	}

	if (draw_x < 0) { draw_x = 0; }

	if (draw_w > SCREEN_WIDTH) draw_w = SCREEN_WIDTH;

	if (draw_x > SCREEN_WIDTH) return;

	for (int i = draw_x; i < draw_w; i++)
		SETPIX(i, y, c);
}

void draw_Vline(const int x, const int y, const int h, const int c)
{
	assert (screen_buffer != NULL);
	if (x < 0 || x > SCREEN_WIDTH || y > SCREEN_HEIGHT) return;

	int draw_y = y, draw_h = y + h;

	if (h < 0)
	{
		draw_y = draw_h;
		draw_h = draw_y - h;
	}

	if (draw_y < 0) { draw_y = 0; }

	if (draw_h > SCREEN_HEIGHT) draw_h = SCREEN_HEIGHT;

	if (draw_y > SCREEN_HEIGHT) return;

	for (int i = draw_y; i < draw_h; i++)
		SETPIX(x, i, c);
}

void draw_rect(const Rect_t r)
{
	int draw_x = r.x, draw_w = r.x + r.w,
		draw_y = r.y, draw_h = r.y + r.h,
		c = r.c;

	assert (screen_buffer != NULL);

	if (r.w < 0)
	{
		draw_x = draw_w;
		draw_w = draw_x - r.w;
	}

	if (r.h < 0)
	{
		draw_y = draw_h;
		draw_h = draw_y - r.h;
	}

	draw_w--;
	draw_h--;

	if (draw_x < 0) { draw_x = 0; }
	if (draw_y < 0) { draw_y = 0; }

	if (draw_w > SCREEN_WIDTH) draw_w = SCREEN_WIDTH;
	if (draw_h > SCREEN_HEIGHT) draw_h = SCREEN_HEIGHT;

	if (draw_x > SCREEN_WIDTH) return;
	if (draw_y > SCREEN_HEIGHT) return;

	for (int x = draw_x; x <= draw_w; x++)
	{
		SETPIX(x, draw_y, c);
		SETPIX(x, draw_h, c);
	}

	for (int y = draw_y; y <= draw_h; y++)
	{
		SETPIX(draw_x, y, c);
		SETPIX(draw_w, y, c);
	}
}

void fill_rect(const Rect_t r)
{
	int draw_x = r.x, draw_w = r.x + r.w, 
		draw_y = r.y, draw_h = r.y + r.h,
		c = r.c;

	assert (screen_buffer != NULL);

	if (r.w < 0)
	{
		draw_x = draw_w;
		draw_w = draw_x - r.w;
	}

	if (r.h < 0)
	{
		draw_y = draw_h;
		draw_h = draw_y - r.h;
	}

	if (draw_x < 0) { draw_x = 0; }
	if (draw_y < 0) { draw_y = 0; }

	if (draw_w > SCREEN_WIDTH) draw_w = SCREEN_WIDTH;
	if (draw_h > SCREEN_HEIGHT) draw_h = SCREEN_HEIGHT;

	if (draw_x > SCREEN_WIDTH) return;
	if (draw_y > SCREEN_HEIGHT) return;

	for (int n = draw_y; n < draw_h; n++)
	{
		for (int i = draw_x; i < draw_w; i++)
			SETPIX(i, n, c);
	}
}

void paint_bevel(Rect_t r, const int c1, const int c2)
{
	const int color_1 = c1 != 0 ? c1 : (int) GETPIX(r.x, r.y),
			  color_2 = c2 != 0 ? c2 : (int) GETPIX(r.x, r.y);

    draw_Hline(r.x, r.y + r.h - 1, r.w, color_1);
    draw_Vline(r.x + r.w - 1, r.y, r.h, color_1);
    draw_Hline(r.x,  r.y, r.w, color_2);
    draw_Vline(r.x,  r.y, r.h, color_2);
}

void print_font(char text, int x, int y, int color, int size)
{
	int i, n, t, draw_x, draw_y;
	int snh = size << 3;

	t = text - '!';

	for(n = 0; n < snh; n++)
	{
		int shift = 7;
		int l = n / size;

		for (i = 0; i < snh; i++)
		{
			draw_x = x + i;
			draw_y = y + n;

			if(draw_x >= PIXEL_WIDTH || draw_y >= PIXEL_HEIGHT)  continue;

			if(*getFont(t, l) & (1 << shift))
				SETPIX(draw_x, draw_y, color);

			if((i + 1) % size == 0) shift--;
		}
	}
}

void print_string(const char* text, const int x, const int y, const int c, const int size)
{
	const int spacing = size << 3, bg = (int) GETXPIX(x);
	int text_x = x - spacing, text_y = y;
	char letter;

	while((letter = *text++) != '\0')
	{
		const int color = bg == (int) GETXPIX((text_x += spacing) + size) ? c : 
								(int) (GETXPIX(text_x) > 0xFF808080 ? 0xFF000000 : 0xFFFFFFFF);

		if(letter == ' ') continue;

		if(letter == '\n' || text_x >= PIXEL_WIDTH - 7)
		{
			text_x = x;
			text_y += spacing;
			continue;
		}

		print_font(letter, text_x, text_y, color, size);
	}
}

void print_string_shadow(const char* text, const int x, const int y, const int off[2], const int c[2], const int size)
{
	print_string(text, x + off[0], y + off[1], c[1], size);
	print_string(text, x, y, c[0], size);
}

uint32_t** getScreenBuffer(void)
{
	if (screen_buffer) return &screen_buffer;
	else return NULL;
}

void setScreenBuffer(Pixel_Buffer_t *pixels) 
{ 
	assert(pixels != NULL);
	
	if(pixels->buffer == NULL) return;

	screen_buffer = pixels->buffer;
	PIXEL_WIDTH   = pixels->width; 
	PIXEL_HEIGHT  = pixels->height; 
}
