#ifndef __SBC_WIDGETS_H
#define __SBC_WIDGETS_H

#include "sbc_defs.h"

/*
*======================================================================================

	Button functions

*======================================================================================
*/

typedef enum 
{
	STD_BUTTON = 1,
	TXT_BUTTON = 2,
	CRT_BUTTON = 3,
	RAD_BUTTON = 4
} ButtonType_t;

typedef struct Button_s
{
	Rect_t rect;
	ButtonType_t type;
	const char* text;

	int txt_color;
	int txt_scale;

	bool clicked;

} Button_t;

Button_t *createButton(const Rect_t rect, const ButtonType_t type, const char *text, const int color, const int scale);
void destroyButton(Button_t **b);

void paint_button(Button_t*btn);

void click_button(Button_t*, const bool);
bool wasButtonClicked(const Button_t* b);

void initRadButtons(Button_t **b, Rect_t rect, const int num_btns, const char** text, const int color, const int scale, const int dflt);
void destroyRadButtons(Button_t **b, const int num_btns);

int radButtonHitbox(Button_t **b, const int num_buttons, const int x, const int y);
void radButtonClick(Button_t **b, const int num_buttons, const int selection);

/*
*======================================================================================

	Textbox functions

*======================================================================================
*/

typedef struct Textbox_s
{
    char* text;
	int text_color, select_color;

    Pixel_Buffer_t pixels;
    Rect_t rect, cursor;

    int text_length, text_start, select_start, select_end;

	bool editing;
} Textbox_t;

Textbox_t *createTextBox(const char* text, const int colors[3], const int scale, const Rect_t rect);
void destroyTextBox(Textbox_t **t);

void updateCursor(void);
bool *getCursorBlink(void);
void paintTextbox(Textbox_t *t, const bool box);

void setTextboxText(Textbox_t *t, const char* text);

void typeText(Textbox_t *t, const char c);
void handleDelete(Textbox_t *t, const bool backspace);

void incCursorPos(Textbox_t *t);
void decCursorPos(Textbox_t *t);
void cursorToStart(Textbox_t *t);
void cursorToEnd(Textbox_t *t);

void selectAll(Textbox_t *t);
void incSelectEnd(Textbox_t *t);
void decSelectEnd(Textbox_t *t);
void selectToStart(Textbox_t *t);
void selectToEnd(Textbox_t *t);

void setCursorPos(Textbox_t *t, const int mouse_x);
void setSelectEndPos(Textbox_t *t, const int mouse_x);

void setTextboxEditing(Textbox_t *t, const bool editing);

/*
*======================================================================================

	Select Menu functions

*======================================================================================
*/

typedef struct Select_Menu_s
{
	const char *title;
	char **item;
	int max, current;
	Rect_t rect;
	int select_color;
} Select_Menu_t;

Select_Menu_t *createSelectMenu(const char* title, const int max, const Rect_t rect, const int color);

void initSelectMenuMax(Select_Menu_t *s, const int max);
void initSelectMenuItem(Select_Menu_t *s, const int i, const char* item);

void clearSelectMenu(Select_Menu_t **s);
void destroySelectMenu(Select_Menu_t **s);

void setSelectMenuSelection(Select_Menu_t *s, const int i);
bool isMenuInitialized(const Select_Menu_t *s);
void paintSelectMenu(const Select_Menu_t *s);
int selectMenuHitbox(const Select_Menu_t *s, const int x, const int y);

/*
*======================================================================================

	Slider functions

*======================================================================================
*/

typedef struct Slider_s
{
        int sample;
        int x_pos;

		SliderType_t type;

        bool clicked;

} Slider_t;

void paintSamplePos(const int pos);
void paint_slider(const Slider_t *s, const uint8_t color[4]);
void paintLoopAreaOverlay(const Slider_t *start, const Slider_t *end);
void paintIgnoredAreaOverlay(const Slider_t *slider, const int pos);

#endif /* __SBC_WIDGETS_H */
