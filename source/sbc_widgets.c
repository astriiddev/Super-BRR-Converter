#include "sbc_utils.h"
#include "sbc_widgets.h"
#include "sbc_screen.h"
#include "sbc_window.h"

/*
*======================================================================================

	Button functions

*======================================================================================
*/

Button_t *createButton(const Rect_t rect, const ButtonType_t type, const char *text, const int color, const int scale)
{
    Button_t *new_button = calloc(1, sizeof(Button_t));

    if(new_button == NULL) return NULL;

    new_button->rect = rect;
    new_button->type = type;
    new_button->text = text;

    new_button->txt_color = color;
    new_button->txt_scale = scale;
    new_button->clicked = false;

    return new_button;
}

void destroyButton(Button_t **b)
{
    SBC_FREE((*b));
}

static void paint_box_button(const Button_t *b)
{	
	const Rect_t *r = &b->rect;
	const uint32_t c1 	= r->c + 0x202020, c2  	= r->c - 0x707070;

	fill_rect((Rect_t) { r->x - 1, r->y - 1, r->w + 2, r->h + 2, 0 });
	fill_rect(*r);

	if(b->clicked) paint_bevel(*r, 0, c2);
	else paint_bevel(*r, c2, c1);
}

static void paint_cartridge_button(const Button_t *btn)
{
	const uint32_t color 		= btn->rect.c;
	const uint32_t color_brite 	= color + 0x202020;
	const uint32_t color_dark  	= color - 0x707070;

	const Rect_t m = btn->rect;

	Rect_t ct = { m.x + (m.w / 8), m.y, (m.w * 3) / 4, m.h / 4, color };
	Rect_t cb = { m.x, m.y + m.h / 4, m.w, (m.h * 3) / 4, color };

	fill_rect((Rect_t) {ct.x - 1, ct.y - 1, ct.w + 2, ct.h, 0});
	fill_rect((Rect_t) {cb.x - 1, cb.y - 1, cb.w + 2, cb.h + 2, 0});

	fill_rect(ct);
	fill_rect(cb);

	if(btn->clicked)
	{
		fill_rect((Rect_t) {ct.x, ct.y, ct.w, 1, color_dark});
		fill_rect((Rect_t) {ct.x, ct.y, 1, ct.h, color_dark});

		fill_rect((Rect_t) {cb.x, cb.y, 1, cb.h, color_dark});
		fill_rect((Rect_t) {cb.x + 1, cb.y, cb.w / 8, 1, color_dark});

		fill_rect((Rect_t) {cb.x + 1 + cb.w / 4, ct.y, cb.w  / 2, ct.h + 1, 0});
	}
	else
	{
		fill_rect((Rect_t) {ct.x, ct.y, ct.w, 1, color_brite});
		fill_rect((Rect_t) {ct.x, ct.y, 1, ct.h, color_brite});

		fill_rect((Rect_t) {cb.x, cb.y, 1, cb.h, color_brite});
		fill_rect((Rect_t) {cb.x + 1, cb.y, cb.w / 8, 1, color_brite});

		fill_rect((Rect_t) {ct.x + ct.w - 1, ct.y, 1, ct.h, color_dark});
		fill_rect((Rect_t) {cb.x + cb.w - 1, cb.y, 1, cb.h, color_dark});
		fill_rect((Rect_t) {cb.x, cb.y + cb.h - 1, cb.w, 1, color_dark});

		fill_rect((Rect_t) {cb.x + cb.w / 4, ct.y, cb.w  / 2, ct.h, 0});
	}
}

static void paint_button_text(const Button_t *btn)
{
	int textlen = 0, size = btn->txt_scale, x, y;

	Rect_t m = btn->type == CRT_BUTTON ? 
                (Rect_t) {btn->rect.x, btn->rect.y + btn->rect.h / 4, 
                btn->rect.w, (btn->rect.h * 3) / 4, btn->rect.c } : 
                btn->rect;

	textlen = (int) strlen(btn->text) * (size << 3);

	if(m.w < textlen + 4) m.w = textlen + 4;

    x = m.x + ((m.w - textlen) >> 1);
    y = m.y + ((m.h - (size << 3)) >> 1);

    if (btn->type == CRT_BUTTON) x--;

	if(btn->clicked) print_string(btn->text, x, y + 1, btn->txt_color, size);
	else print_string(btn->text, x - 1, y, btn->txt_color, size);
}

static void paint_radio_button(const Button_t *btn)
{
	const int size = btn->txt_scale;
	const Rect_t m = btn->rect, box = { m.x, m.y + ((m.h - (size << 3)) >> 1), size << 3, size << 3, m.c };

	if(btn->clicked)
	{
		fill_rect(box);
	}
	else
	{
		fill_rect((Rect_t) { box.x, box.y, box.w, box.h, SBCDGREY });
		draw_rect(box);
	}

	if(btn->text == NULL) return;

	print_string(btn->text, box.x + box.w + 2, m.y + ((m.h - (size << 3)) >> 1), btn->txt_color, size);
}

void paint_button(Button_t *btn)
{	
    if(btn == NULL) return;
	if(btn->txt_scale < 1) btn->txt_scale = 1;

	switch (btn->type)
	{
	case STD_BUTTON:
	case TXT_BUTTON:
		paint_box_button(btn);

		if(btn->text == NULL) break;
	    paint_button_text(btn);

		break;
	case CRT_BUTTON:
		paint_cartridge_button(btn);

		if(btn->text == NULL) break;
	    paint_button_text(btn);

		break;

	case RAD_BUTTON:
		paint_radio_button(btn);
		
		break;
	}
}

void click_button(Button_t *b, bool click)
{
    if(b == NULL) return;

	b->clicked = click;
	paint_button(b);
}

bool wasButtonClicked(const Button_t *b)  { return b == NULL ? false : b->clicked; }

void initRadButtons(Button_t **b, Rect_t rect, const int num_btns, const char** text, const int color, const int scale, const int dflt)
{
    if(b == NULL) return;

    for(int i = 0; i < num_btns; i++)
    {
        rect.w = ((int) strlen(text[i]) + 1) * (scale << 3);

        b[i] = createButton(rect, RAD_BUTTON, text[i], color, 1);

        rect.y += rect.h;

        if(b[i] == NULL) continue;

        b[i]->clicked = (i == dflt);

        SBC_LOG(RAD BUTTON LABEL, %s, text[i]);
        SBC_LOG(RAD BUTTON CLICKED, %s, i == dflt ? "TRUE" : "FALSE");
    }
}

void destroyRadButtons(Button_t **b, const int num_btns)
{
    if(b == NULL) return;

    assert(num_btns > 0);
    
    if(num_btns == 1)
    {
        SBC_FREE(*b);
        return;
    }

    for(int i = 0; i < num_btns; i++)
    {
        if(b[i] == NULL) continue;

        SBC_FREE(b[i]);
    }
}

void radButtonClick(Button_t **b, const int num_buttons, const int selection)
{
    if(b == NULL || selection >= num_buttons) return;

    for(int i = 0; i < num_buttons; i++)
    {
        if(b[i] == NULL) continue;

        if(i == selection) click_button(b[i], true);
        else click_button(b[i], false);
    }
}

int radButtonHitbox(Button_t **b, const int num_buttons, const int x, const int y)
{
    int selection = -1;

    if(b == NULL) return false;

    for(int i = 0; i < num_buttons; i++)
    {
        if(b[i] == NULL || !hitbox(&b[i]->rect, x, y)) continue;
        
        selection = i;
    }

    return selection;
}

/*
*======================================================================================

	Textbox functions

*======================================================================================
*/

static uint64_t double_click = 0;
static bool cursor_blink = false;

Textbox_t *createTextBox(const char* text, const int colors[3], const int scale, const Rect_t rect)
{
    Textbox_t *new_textbox = calloc(1, sizeof(Textbox_t));

    if(new_textbox == NULL) return NULL;

    new_textbox->text_color = colors[0];
    new_textbox->select_color = colors[1];

    new_textbox->rect = rect;
    new_textbox->cursor = (Rect_t) { 0, 0, scale, (scale << 3) + 2, colors[2] };

    new_textbox->pixels.scale = scale;
    setTextboxText(new_textbox, text);

    return new_textbox;
}

void destroyTextBox(Textbox_t **t)
{
    if(t == NULL) return;

    SBC_FREE((*t)->text);
    SBC_FREE((*t)->pixels.buffer);

    SBC_FREE((*t));
}

static int getLetterPosX(const int pos, const int scale) { return pos * (scale << 3) + (scale << 1); }
static int getTextStartX(const Textbox_t* t) { return getLetterPosX(t->text_start, t->pixels.scale); }

static void blitText(Pixel_Buffer_t* dest, const Textbox_t* src, const int dest_x, const int dest_y)
{
    assert(src  != NULL);
    assert(dest != NULL);

    for(int i = getTextStartX(src); i < src->pixels.width; i++)
    {
        for(int n = 0; n < src->pixels.height; n++)
        {
            const int pixel = src->pixels.buffer[i + (n * src->pixels.width)];
            
            if(!pixel || i - getTextStartX(src) > src->rect.w - 4 || n > src->rect.h - 2) continue;
            dest->buffer[(dest_x + i - getTextStartX(src)) + ((dest_y + n) * dest->width)] = pixel;
        }
    }
}

static int get_text_color(const Textbox_t* t, const int i)
{
    const int start = t->select_start < t->select_end ? t->select_start : t->select_end,
              end   = t->select_start < t->select_end ? t->select_end : t->select_start;

    if(t->select_start == t->select_end) return t->text_color;

    if(i < start) return t->text_color;
    if(i >= end)  return t->text_color;

    return t->select_color;
}

static void print_tb_text(const Textbox_t* t)
{
	const int scale = t->pixels.scale, spacing = scale << 3;
	int text_x = (scale << 1) - spacing + 1, text_y = scale;

    char* text = t->text;

    if(text == NULL || strlen(text) == 0) return;

	for(int i = 0; i < t->text_length; i++)
	{
        const char letter = text[i];
        text_x += spacing;

		if(letter == ' ') continue;

		print_font(letter, text_x, text_y, get_text_color(t, i), scale);
	}
}

static void paintCursor(const Textbox_t* t)
{
    Rect_t cursor = t->cursor;

    if(t == NULL || !t->editing) return;
    
    if(t->select_start != t->select_end)
    {
        fill_rect(cursor);
        return;
    }

    cursor.c = cursor_blink ? t->rect.c : t->cursor.c;

    fill_rect(cursor);
}

static void set_cursor(Textbox_t* t)
{
    const int cursor_len = t->select_end - t->select_start,
              scale      = t->pixels.scale, 
              letter_w   = scale << 3;

    t->cursor = (Rect_t)
    {
        (letter_w * (cursor_len >= 0 ? t->select_start : t->select_end)) + scale + 1,
        0,
        cursor_len == 0 ? scale : (abs(cursor_len) * letter_w) + scale,
        t->pixels.height,
        SCROLLBACK
    };

    cursor_blink = false;
}

void setTextboxText(Textbox_t *t, const char* text)
{
    const int scale = t->pixels.scale;

    SBC_FREE(t->text);

    t->text = _strndup((char*) text, strlen(text));

    SBC_FREE(t->pixels.buffer);

    t->text_length   = (int) strlen(t->text);
    t->pixels.width  = t->text_length * (scale << 3) + (scale << 2);
    t->pixels.height = (t->pixels.scale << 3) + (scale << 1);

    t->pixels.buffer = calloc(t->pixels.width * t->pixels.height, sizeof *t->pixels.buffer);

    set_cursor(t);
}

static int getXtoLetterPos(const Textbox_t* t, const int x) { return (x + t->pixels.scale) / (t->pixels.scale << 3); }

static void setTextStart(Textbox_t* t) 
{ 
    const int start_x = t->cursor.x - getTextStartX(t);

    if(start_x > t->rect.w - 4) 
        t->text_start = getXtoLetterPos(t, t->cursor.x - t->rect.w + (t->pixels.scale << 3)); 

    else if(start_x < 0)
        t->text_start = getXtoLetterPos(t, t->cursor.x);
}

static char* typeOverSelection(Textbox_t *t, const char c)
{
    const int start = t->select_end > t->select_start ? t->select_start : t->select_end,
              end   = t->select_end > t->select_start ? t->select_end : t->select_start,
              len   = t->text_length - (end - start) + 2;

    char *temp = NULL;

    temp = malloc(len);
    snprintf(temp, len, "%.*s%c%s" , start, t->text, c, end > t->text_length ? "\0" : t->text + end);

    t->select_start = t->select_end = start + 1;
    return temp;
}

static char* typeAtCursor(Textbox_t* t, const char c)
{
    const int pos = t->select_start, len = t->text_length + 2;
    char *temp = NULL;

    temp = malloc(len);
    snprintf(temp, len, "%.*s%c%s" , pos, t->text, c, pos > t->text_length ? "\0" : t->text + pos);

    t->select_start = ++t->select_end;

    return temp;
}

void updateCursor(void) { cursor_blink ^= 1; }
bool *getCursorBlink(void) { return &cursor_blink; }

void paintTextbox(Textbox_t *t, const bool box)
{
    Pixel_Buffer_t* gui_buffer = getSbcPixelBuffer();
	int scale, text_y;

	if(t == NULL || t->pixels.buffer == NULL) return;
	
	scale = t->pixels.scale;
	text_y = (t->rect.y + t->rect.h) - (scale * 10) - 1;

    fill_rect((Rect_t) { t->rect.x, t->rect.y, t->rect.w, t->rect.h, SBCDGREY });
    if(box) fill_rect(t->rect);

	memset(t->pixels.buffer, 0, t->pixels.width  * t->pixels.height * sizeof *t->pixels.buffer);
	setScreenBuffer(&t->pixels);

	paintCursor(t);
	print_tb_text(t);

	setScreenBuffer(gui_buffer);
	blitText(gui_buffer, t, t->rect.x + 2, (scale << 3) < t->rect.h ? text_y : t->rect.y);

    if(box) paint_bevel(t->rect, addColors(t->rect.c, 0x00454545), subColors(t->rect.c, 0x00505050));
}

void typeText(Textbox_t *t, const char c)
{
    char *temp = NULL, input = c < ' ' ||  c > '~' ? '?' : c;

    if(t == NULL || !t->editing) return;

    if(t->select_start == t->select_end)
        temp = typeAtCursor(t, input);
    else 
        temp = typeOverSelection(t, input);

    if(temp == NULL) return;

    SBC_FREE(t->text);

    setTextboxText(t, temp);
    setTextStart(t);
    
    SBC_FREE(temp);

    if(t->text_start == t->select_start && t->text_start > 0) t->text_start--;
}

static char* deleteAtCursor(Textbox_t *t, const bool backspace)
{
    const int pos = t->select_start;
    char *temp = NULL;

    if(backspace && pos <= 0) return temp;
    if(!backspace && pos >= t->text_length) return temp;

    temp = malloc(t->text_length);
    snprintf(temp, t->text_length, "%.*s%s", backspace ? pos - 1 : pos, t->text, t->text + (backspace ? pos : pos + 1));

    if(backspace) t->select_start = --t->select_end;

    return temp;
}

static char* deleteRangeText(Textbox_t *t)
{
    const int start = t->select_end > t->select_start ? t->select_start : t->select_end,
              end   = t->select_end > t->select_start ? t->select_end : t->select_start,
              len   = t->text_length - (end - start) + 1;

    char *temp = NULL;

    temp = malloc(len);
    snprintf(temp, len, "%.*s%s" , start, t->text, end > t->text_length ? "\0" : t->text + end);

    t->select_start = t->select_end = start;

    return temp;
}

void handleDelete(Textbox_t *t, const bool backspace)
{
    char *temp = NULL;

    if(t == NULL || !t->editing) return;

    if(t->select_start == t->select_end)
        temp = deleteAtCursor(t, backspace);
    else
        temp = deleteRangeText(t);

    if(temp == NULL) return;

    SBC_FREE(t->text);

    setTextboxText(t, temp);
    setTextStart(t);

    SBC_FREE(temp);
    
    if(t->text_start == t->select_start && t->text_start > 0) t->text_start--;
}

void incCursorPos(Textbox_t *t)
{
    t->select_start = t->select_end = t->select_end >= t->text_length ? t->text_length : t->select_end  + 1;

    set_cursor(t);
    setTextStart(t);
}

void incSelectEnd(Textbox_t *t)
{
    t->select_end = t->select_end >= t->text_length ? t->text_length : t->select_end  + 1;
    if(t->select_end > getXtoLetterPos(t, t->rect.w) + t->text_start) t->text_start++;

    set_cursor(t);
}

void decCursorPos(Textbox_t *t)
{
    t->select_start = t->select_end = t->select_start <= 0 ? 0 : t->select_start - 1;

    set_cursor(t);
    setTextStart(t);
}

void decSelectEnd(Textbox_t *t)
{
    t->select_end = t->select_end <= 0 ? 0 : t->select_end - 1;
    if(t->select_end < t->text_start) t->text_start--;

    set_cursor(t);
}

void cursorToStart(Textbox_t *t)
{
    t->select_start = t->select_end = 0;

    set_cursor(t);
    setTextStart(t);
}

void selectToStart(Textbox_t *t)
{
    t->select_end = 0;

    set_cursor(t);
    t->text_start = 0;
}

void cursorToEnd(Textbox_t *t)
{
    t->select_start = t->select_end = t->text_length;

    set_cursor(t);
    setTextStart(t);
}

void selectToEnd(Textbox_t *t)
{
    t->select_end = t->text_length;

    set_cursor(t);
    t->text_start = t->text_length - getXtoLetterPos(t, t->rect.w);
}

void selectAll(Textbox_t *t)
{
    t->select_start = 0;
    t->select_end = t->text_length;

    set_cursor(t);
    double_click = SDL_GetTicks64();
}

void setCursorPos( Textbox_t *t, const int mouse_x)
{
	if(t == NULL) return;

    if(SDL_GetTicks64() - double_click < 500)
    {
        selectAll(t);
        return;
    }
    else
    {
        t->select_start = t->select_end = getXtoLetterPos(t, mouse_x - t->rect.x) + t->text_start;

        if(t->select_start > t->text_length) t->select_start = t->select_end = t->text_length;
    }

    set_cursor(t);
    double_click = SDL_GetTicks64();
}

void setSelectEndPos(Textbox_t *t, const int mouse_x)
{
    const int mouse_in_box = mouse_x - t->rect.x, 
              mouse_letter_pos = mouse_in_box < t->rect.w ? mouse_in_box > 0 ? mouse_in_box : 0 : t->rect.w;
              
    t->select_end = getXtoLetterPos(t, mouse_letter_pos) + t->text_start;

    if(t->select_end > t->text_length) t->select_end = t->text_length;
    else if (t->select_end < 0) t->select_end = 0;

    if(mouse_in_box > t->rect.w && t->select_end < t->text_length) 
    {
        if(mouse_in_box % (t->pixels.scale << 1) == 0) t->text_start++;
    }
    else if(mouse_in_box < 0 && t->select_end > 0)
    {
        if(abs(mouse_in_box) % (t->pixels.scale << 1) == 0) t->text_start--;
    }

    set_cursor(t);
}

void setTextboxEditing(Textbox_t *t, const bool editing) 
{ 
	if(t == NULL) return; 

	t->editing = cursor_blink = editing; 
	t->select_start = t->select_end = 0;
}

/*
*======================================================================================

	Select Menu functions

*======================================================================================
*/

Select_Menu_t *createSelectMenu(const char *title, const int max, const Rect_t rect, const int color)
{
    Select_Menu_t *new_menu = calloc(1, sizeof(Select_Menu_t));

    if(new_menu == NULL) return NULL;

    new_menu->title = title;
    new_menu->max = max;
    new_menu->current = -1;
    new_menu->rect = rect;
    new_menu->select_color = color;

    initSelectMenuMax(new_menu, max);

    return new_menu;
}

void initSelectMenuMax(Select_Menu_t *s, const int max)
{
    if(s == NULL || max <= 0) return;

    clearSelectMenu(&s);

    SBC_LOG(SELECT MENU MAX ITEMS, %d, max);

    s->max = max;
    s->item = malloc(s->max * sizeof(char*));
}

void initSelectMenuItem(Select_Menu_t *s, const int i, const char *item)
{
    if(!isMenuInitialized(s) || item == NULL) return;
    if(i >= s->max) return;

    SBC_LOG(SELECT MENU INDEX, %d, i);
    SBC_LOG(SELECT MENU ITEM , %s, item);

    s->item[i] = _strndup((char*) item, strlen(item));
}

void clearSelectMenu(Select_Menu_t **s)
{
    if(!isMenuInitialized((*s))) return;

    for(int i = 0; i < (*s)->max; i++)
    {
        if((*s)->item[i] == NULL) continue;
        SBC_FREE((*s)->item[i]);
    }

    SBC_FREE((*s)->item);

    (*s)->max = 0;
    (*s)->current = -1;
}

void destroySelectMenu(Select_Menu_t **s)
{
    clearSelectMenu(s);
    SBC_FREE((*s));
}

void setSelectMenuSelection(Select_Menu_t *s, const int i)
{
    if(!isMenuInitialized(s) || i >= s->max || s->current == i) return;
    s->current = i;
}

bool isMenuInitialized(const Select_Menu_t *s)
{
    if(s == NULL) return false;
    if(s->item == NULL) return false;
    if(s->max <= 0) return false;

    return true;
}

static int paint_menu_item(const Select_Menu_t *s, const int i, const int y)
{
    char* item, list_name[40];
    Rect_t select = { s->rect.x + 2, -13, s->rect.w - 5, 11, s->select_color };
    
    if(i >= s->max || s->item[i] == NULL) return 0;

    item = s->item[i];
    sprintf(list_name, "%02d %.25s", i + 1, item);

    if(i == s->current)
    { 
            select.y = (s->rect.y + 1) + (11 * i);
            fill_rect(select);
    }

    print_string_shadow(list_name, s->rect.x + 5, y, (int[]) { 1, 0}, 
                        (int[]) { SBCLGREY, 0xFF2E2E2E}, 1);

    return y;
}

void paintSelectMenu(const Select_Menu_t *s)
{
    int i = 0, y = 3;

    if(!isMenuInitialized(s)) return;

    y += s->rect.y;

    fill_rect(s->rect);
    paint_bevel(s->rect, 0xFFBEBEBE, 0xFF2E2E2E);

    print_string_shadow(s->title, s->rect.x + 2, s->rect.y - 11, (int[]) {1, 0}, 
                        (int[]) {s->select_color, 0xFF121212}, 1);

    while(paint_menu_item(s, i, y) > 0)
    {
        i++;
        y +=11;
    }
}

int selectMenuHitbox(const Select_Menu_t *s, const int x, const int y)
{
    int selection = -1;

    if(!isMenuInitialized(s)) return selection;
    if(!hitbox(&s->rect, x, y) || y > ((s->max - 1) * 11 )+ (s->rect.y + 12)) return selection;

    for(int i = 0; i < s->max; i++)
    {
        if(y < (s->rect.y +  1) + (11 * i)) continue;
        if(y > (s->rect.y + 12) + (11 * i)) continue;

        selection = i;
    }

    return selection;
}

/*
*======================================================================================

	Slider functions

*======================================================================================
*/

static const int SLIDE_Y = 1, SLIDE_H = SAMPLE_HEIGHT - 1;

void paintSamplePos(const int pos)
{
	SDL_SetRenderDrawColor(*getSbcRenderer(),0xFF,0xFF,0xFF,0xFF);
	SDL_RenderDrawLine(*getSbcRenderer(), pos, 1, pos, SAMPLE_HEIGHT - 1);
}

void paint_slider(const Slider_t *s, const uint8_t color[4])
{
	const SDL_Rect flag = { s->type != END_LOOP_SLIDER ? s->x_pos : s->x_pos - 8, 
							s->type != END_LOOP_SLIDER ? 1 : SAMPLE_HEIGHT - 8, 8, 8 };
	
	const SDL_Color c = { color[0], color[1],  color[2], color[3] };
	
	SDL_SetRenderDrawBlendMode(*getSbcRenderer(), SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(*getSbcRenderer(), c.r, c.g, c.b, c.a);

	SDL_RenderFillRect(*getSbcRenderer(), &flag);
	SDL_RenderDrawLine(*getSbcRenderer(), s->x_pos, SLIDE_Y, s->x_pos, SLIDE_H);
}

void paintLoopAreaOverlay(const Slider_t *start, const Slider_t *end)
{
	const int loop_xstart = start->x_pos;
	const int loop_xend   = end->x_pos;
	const int loop_range  = loop_xend - loop_xstart;

	const SDL_Rect loop_area = { loop_xstart, 1, loop_range, SAMPLE_HEIGHT - 1};
	
	const SDL_Color c = { 0xAC, 0x63, 0xD1, 0x30 };

	SDL_SetRenderDrawBlendMode(*getSbcRenderer(), SDL_BLENDMODE_BLEND);        

	SDL_SetRenderDrawColor(*getSbcRenderer(), c.r, c.g, c.b, c.a);
	SDL_RenderFillRect(*getSbcRenderer(), &loop_area);
}

void paintIgnoredAreaOverlay(const Slider_t *slider, const int pos)
{
	const int x = slider->x_pos < pos ? slider->x_pos : pos;
	const int w = slider->x_pos < pos ? pos - x : slider->x_pos - x;

	const SDL_Rect ignored_area = { x, 1, w, SAMPLE_HEIGHT - 1 };

	const SDL_Color c = { 0x12, 0x12, 0x12, 0xB0 };

	SDL_SetRenderDrawBlendMode(*getSbcRenderer(), SDL_BLENDMODE_BLEND);        

	SDL_SetRenderDrawColor(*getSbcRenderer(), c.r, c.g, c.b, c.a);
	SDL_RenderFillRect(*getSbcRenderer(), &ignored_area);
}
