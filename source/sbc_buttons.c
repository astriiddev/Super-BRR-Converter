#include "sbc_utils.h"
#include "sbc_buttons.h"

#include "sbc_audio.h"
#include "sbc_gui.h"
#include "sbc_optmenu.h"
#include "sbc_samp_edit.h"
#include "sbc_pitch.h"
#include "sbc_waveform.h"

#define NUM_BUTTONS  13
#define STD_BUTTON_W 63
#define STD_BUTTON_H 15

enum Gui_Buttons
{
    PLAY = 0, 
    LOOP = 1, 
    OPTIONS = 2, 
    CLEAR = 3, 
    RESAMPLE = 4, 
    PITCH = 5,
    LOAD = 6, 
    SAVE = 7, 
    CUT = 8, 
    COPY = 9,
    PASTE = 10, 
    CROP = 11, 
    DELETE = 12,
};

static Button_t *gui_buttons[NUM_BUTTONS];

void initButtons(void)
{
    const int left = 5, right = 69;
    Rect_t button_rect = { left, SAMPLE_HEIGHT +  107, STD_BUTTON_W, STD_BUTTON_H, TRACKGREY };

    gui_buttons[CUT] = createButton(button_rect, TXT_BUTTON, "CUT", 0xFF000000, 1);

    button_rect.x = right;
    gui_buttons[COPY] = createButton(button_rect, TXT_BUTTON, "COPY", 0xFF000000, 1);

    button_rect.y = SAMPLE_HEIGHT + 123;
    gui_buttons[PASTE] = createButton(button_rect, TXT_BUTTON, "PASTE", 0xFF000000, 1);

    button_rect.x = left;
    gui_buttons[CROP] = createButton(button_rect, TXT_BUTTON, "CROP", 0xFF000000, 1);

    button_rect.y = SAMPLE_HEIGHT + 139;
    gui_buttons[DELETE] = createButton(button_rect, TXT_BUTTON, "DELETE", 0xFF000000, 1);

    button_rect.x = right;
    gui_buttons[CLEAR] = createButton(button_rect, TXT_BUTTON, "CLEAR", 0xFF000000, 1);

    button_rect.x = 289;
    button_rect.y = SCREEN_HEIGHT - 20;
    gui_buttons[OPTIONS] = createButton(button_rect, TXT_BUTTON, "OPTIONS", 0xFF000000, 1);

    button_rect.x = 225;
    gui_buttons[LOOP] = createButton(button_rect, TXT_BUTTON, "LOOP", 0xFF000000, 1);

    button_rect.x = 161;
    gui_buttons[PLAY] = createButton(button_rect, TXT_BUTTON, "PLAY", 0xFF000000, 1);
    
    button_rect.w = 55;
    button_rect.y = SAMPLE_HEIGHT + 41;
    gui_buttons[PITCH] = createButton(button_rect, TXT_BUTTON, "PITCH", 0xFF000000, 1);

    button_rect.x = 217;
    button_rect.w = 70;
    gui_buttons[RESAMPLE] = createButton(button_rect, TXT_BUTTON, "RESAMPLE", 0xFF000000, 1);

    button_rect = (Rect_t) { SCREEN_WIDTH - 150, SCREEN_HEIGHT - 50, STD_BUTTON_W, 45, TRACKGREY };
    gui_buttons[LOAD] = createButton(button_rect, CRT_BUTTON, "LOAD", SBCDPURPLE, 2);

    button_rect.x = SCREEN_WIDTH -  75;
    gui_buttons[SAVE] = createButton(button_rect, CRT_BUTTON, "SAVE", SBCDPURPLE, 2);
}

void freeButtons(void)
{
    for(int i = 0; i < NUM_BUTTONS; i++)
        destroyButton(&gui_buttons[i]);
}

void paintButtons(void)
{
    for(int i = 0; i < NUM_BUTTONS; i++)
        paint_button(gui_buttons[i]);
}

static bool buttonHitbox(const Button_t *btn, const int x, const int y)
{
    return hitbox(&btn->rect, x, y);
}

bool mouseDownOnButton(const int x, const int y)
{
    bool update_gui = false;

    for(int i = 0; i < NUM_BUTTONS; i++)
    {
        if(buttonHitbox(gui_buttons[i], x, y))
        {
            update_gui = true;

            if(i == LOOP) 
            {
                click_button(gui_buttons[i], gui_buttons[i]->clicked ^ 1);
                setLoopEnable(gui_buttons[i]->clicked);

                SBC_LOG(SHOWING LOOPS, %s, gui_buttons[LOOP]->clicked ? "TRUE" : "FALSE");
                break;
            }

            if(optionsIsShowing() && i > 7) break;
                
            click_button(gui_buttons[i], true);
            break;
        }
    }

    return update_gui;
}

static void do_button_action(const enum Gui_Buttons b)
{
    assert((int) b > -1);

    switch(b)
    {
        case PLAY:
        {
            if(*audioQueued()) pauseAudio();
            else queueAudio();
        }
        break;

        case LOOP: 
        {
            click_button(gui_buttons[b], *isSampEditLoopEnabled());
        }
        return;

        case OPTIONS:
        {
            if(optionsIsShowing()) 
            {
                hideOptions();
                repaintWaveform();
            }
            else showOptions();
        } 
        break;

        case CLEAR:
        {
            audioPaused();
            setUndoBuffer();
            clearSampleEdit();
            drawNewWave();
            if(!optionsIsShowing()) repaintWaveform();
        } 
        break;

        case RESAMPLE:
        {
            audioPaused();

            if(handleResample())
            {
                drawNewWave();
                
                if(!optionsIsShowing()) repaintWaveform();

                handleSampleRateText(*getSampleEditSampleRate());
            }
        } 
        break;

        case PITCH:
        {
            detectCenterPitch(1);
        }
        break;

        case LOAD:
        {
            audioPaused();
            openFileDialog();
        } 
        break;

        case SAVE:
        {
            audioPaused();
            saveFileDialog();
        } 
        break;

        case CUT:
        {
            audioPaused();
            
            if(handleSampleCut())
            {
                drawNewWave();

                if(*getSampleEditLength() <= 1)
                {
                    setLoopEnable(false);
                    click_button(getLoopButton(), false);
                }
             
                repaintWaveform();
                repaintGUI();
            }
        } 
        break;

        case COPY:
        {
            handleSampleCopy();
        }
        break;

        case PASTE:
        {
            audioPaused();

            if(handleSamplePaste())
            {
                drawNewWave();
                repaintWaveform();
                repaintGUI();
            }
        } 
        break;

        case CROP:
        {
            audioPaused();
            
            if(handleSampleCrop())
            {
                drawNewWave();

                if(*getSampleEditLength() <= 1)
                {
                    setLoopEnable(false);
                    click_button(getLoopButton(), false);
                }

                repaintWaveform();
                repaintGUI();
            }
        } 
        break;

        case DELETE:
        {
            audioPaused();
            
            if(handleSampleDelete())
            {
                drawNewWave();

                if(*getSampleEditLength() <= 1)
                {
                    setLoopEnable(false);
                    click_button(getLoopButton(), false);
                }

                repaintWaveform();
                repaintGUI();
            }
        }
        break;
    }
}

bool mouseUpOnButton(const int x, const int y)
{
    int button_clicked = -2;

    for(int i = 0; i < NUM_BUTTONS; i++)
    {
        if(!wasButtonClicked(gui_buttons[i])) continue;
            
        if(buttonHitbox(gui_buttons[i], x, y)) button_clicked = i;
        else button_clicked = -1;

        if(button_clicked == LOOP) break;
        else if(i == LOOP) continue;

        click_button(gui_buttons[i], false);
        break;
    }
    
    if(button_clicked > -1) do_button_action(button_clicked);

    return (button_clicked > -2);
}

bool mouseOverButton(const int x, const int y)
{
    for(int i = 0; i < NUM_BUTTONS; i++)
        if(buttonHitbox(gui_buttons[i], x, y)) return true;

    return false;
}

Button_t* getPlayButton(void) { return gui_buttons[PLAY]; }
Button_t* getLoopButton(void) { return gui_buttons[LOOP]; }

Button_t* getOptButton(void)  { return gui_buttons[OPTIONS]; }
