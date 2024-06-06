#include <SDL2/SDL.h>
#include <ctype.h>

#include "sbc_utils.h"
#include "sbc_screen.h"

#include "sbc_optmenu.h"
#include "sbc_samp_edit.h"

#include "sbc_widgets.h"
#include "sbc_textbox.h"

enum
{
    SAMPLE_NAME,
    SAMPLE_RATE,
    RESMPL_RATE,
    DEFAULT_DIR
} Textbox_Types;

static uint64_t cursor_timer = 0;
static Textbox_t *editbox, *textboxes[4];

bool isTextboxEditing(void) { return editbox == NULL ? false : editbox->editing; }

void paintTextBoxes(void)
{
    print_string_shadow("NAME:", 322, 186, (int[]) {1, 1}, (int[]) {SBCDPURPLE, SBCLGREY}, 1);
    print_string_shadow("PLAYBACK RATE:", 140, 186, (int[]) {1, 1}, (int[]) {SBCDPURPLE, SBCLGREY}, 1);

    for(int i = 0; i < 4; i++)
    {
        if(i > 2 && !optionsIsShowing()) break;
        paintTextbox(textboxes[i], true);
    }
}

bool updateTextboxCursor(void)
{
    if(!isTextboxEditing() && *getCursorBlink()) return false;
    if(SDL_GetTicks64() - cursor_timer < 530) return false;

    cursor_timer = SDL_GetTicks64();
    updateCursor();

    SBC_LOG(CURSOR, %s, "BLINK");

    return true;
}

void initTextboxes(void) 
{ 
    char *work_dir = getWorkingDir();

    textboxes[SAMPLE_NAME] = createTextBox("", (const int[]) {SBCMPURPLE, 0xFFDDDDDD, SCROLLBACK }, 1, 
                                            (Rect_t) { 364, SAMPLE_HEIGHT + 21, 168, 16, SCROLLPURP });

    textboxes[SAMPLE_RATE] = createTextBox("016744", (const int[]) {SBCMPURPLE, 0xFFDDDDDD, SCROLLBACK }, 1, 
                                            (Rect_t) { 254, SAMPLE_HEIGHT + 21, 64, 16, SCROLLPURP });

    textboxes[RESMPL_RATE] = createTextBox("016744", (const int[]) {SBCMPURPLE, 0xFFDDDDDD, SCROLLBACK }, 1, 
                                            (Rect_t) { 289, SAMPLE_HEIGHT + 40, 64, 16, SCROLLPURP });

    textboxes[DEFAULT_DIR] = createTextBox(work_dir, (const int[]) {SBCMPURPLE, 0xFFDDDDDD, SCROLLBACK }, 1, 
                                            (Rect_t) { 390, SAMPLE_HEIGHT - 1, 148, 16, SCROLLPURP });
}

void setSampleNameBoxText(const char* text) 
{ 
    setTextboxText(textboxes[SAMPLE_NAME], text); 
    setSampEditName(text);
}

static void set_rate_box(Textbox_t *t, const double rate)
{
    char  *rate_text = NULL;
    
    if(t == NULL) return;
    
    rate_text = calloc(7, sizeof *rate_text);

    snprintf(rate_text, 7, "%06d", (int) rate);
    setTextboxText(t, rate_text);
    SBC_FREE(rate_text);
}

void setSampleRateBoxText(const double rate)   { set_rate_box(textboxes[SAMPLE_RATE], rate); }
void setResampleRateBoxText(const double rate) { set_rate_box(textboxes[RESMPL_RATE], rate); }

void freeTextboxes(void)
{
    for(int i = 0; i < 4; i++)
        destroyTextBox(&textboxes[i]);
}

static int textbox_hitbox(const Textbox_t *t, const int x, const int y)
{
    return t == NULL ? 0 : hitbox(&t->rect, x, y);
}

static void set_rate_text(Textbox_t *t)
{
    char *rate_text = t->text, *err;
    double rate = strtod(rate_text, &err);

    if(*err != '\0' || rate < 1000 || rate > 48000)
        rate = t == textboxes[SAMPLE_RATE] ? *getSampleEditSampleRate() : 16744.0;

    t == textboxes[SAMPLE_RATE] ? setSampleEditSampleRate(rate) : setResampleRate(rate);
    set_rate_box(t, rate);
}

static void handleTextChange(void)
{
    if(editbox == NULL) return;

    if(editbox == textboxes[SAMPLE_NAME]) setSampEditName(textboxes[SAMPLE_NAME]->text);
    else if(editbox == textboxes[SAMPLE_RATE]) set_rate_text(textboxes[SAMPLE_RATE]);
    else if(editbox == textboxes[RESMPL_RATE]) set_rate_text(textboxes[RESMPL_RATE]);
    else if(editbox == textboxes[DEFAULT_DIR]) setWorkingDir(textboxes[DEFAULT_DIR]->text);
}

static void setEditBox(Textbox_t* t)
{
    if(editbox == t) return;
    
    setTextboxEditing(editbox, false);
    handleTextChange();

    if((editbox = t) == NULL) return;
    setTextboxEditing(editbox, true);
}

bool mouseDownTextbox(const int x, const int y)
{
    int change_box = -1;

    for(int i = 0; i < 4; i++)
    {
        if(i > 2 && !optionsIsShowing()) break;

        if(textbox_hitbox(textboxes[i], x, y))
        {
            change_box = i;
            break;
        }
    }

    if(change_box < 0)
    {
        if(editbox != NULL) setEditBox(NULL);
        return false;
    }

    setEditBox(textboxes[change_box]);

    setCursorPos(editbox, x);
    cursor_timer = SDL_GetTicks64();

    return true;
}

int mouseDragTextbox(const int x, const int y, const bool mouse_down)
{
    int action = 0;

    for(int i = 0; i < 4; i++)
    {
        if(i > 2 && !optionsIsShowing()) break;
        
        if(textbox_hitbox(textboxes[i], x, y))
        {
            action |= 1;
            break;
        }
    }

    if(!isTextboxEditing() || !mouse_down) return action;

    setSelectEndPos(editbox, x);

    action |= 2;

    return action;
}

bool textboxInputText(const char c)
{
    if(!isTextboxEditing()) return false;

    if(editbox == textboxes[SAMPLE_RATE] || editbox == textboxes[RESMPL_RATE])
        if(!isdigit(c)) return false;

    typeText(editbox, c);
    return true;
}

bool textboxKeyInput(const uint8_t* keystate)
{
    const bool shift = keystate[SDL_SCANCODE_LSHIFT] | keystate[SDL_SCANCODE_RSHIFT],
               ctrl  = keystate[SDL_SCANCODE_LCTRL]  | keystate[SDL_SCANCODE_RCTRL];

    if(!isTextboxEditing()) return false;

    if(keystate[SDL_SCANCODE_LEFT])
        shift ? decSelectEnd(editbox) : decCursorPos(editbox);
    else if(keystate[SDL_SCANCODE_RIGHT])
        shift ? incSelectEnd(editbox) : incCursorPos(editbox);
    else  if(keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_HOME])
        shift ? selectToStart(editbox) : cursorToStart(editbox);
    else if(keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_END])
        shift ? selectToEnd(editbox) : cursorToEnd(editbox);
    else if(ctrl && keystate[SDL_SCANCODE_A])
        selectAll(editbox);
    else if(keystate[SDL_SCANCODE_BACKSPACE])
        handleDelete(editbox, true);
    else if(keystate[SDL_SCANCODE_DELETE])
        handleDelete(editbox, false);
    else if(keystate[SDL_SCANCODE_RETURN] | keystate[SDL_SCANCODE_KP_ENTER] | keystate[SDL_SCANCODE_ESCAPE])
        setEditBox(NULL);

    cursor_timer = SDL_GetTicks64();

    return true;
}
