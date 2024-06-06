#include "sbc_utils.h"
#include "sbc_keys.h"
#include "sbc_events.h"

#include "sbc_buttons.h"

#include "sbc_gui.h"
#include "sbc_optmenu.h"
#include "sbc_window.h"

#include "sbc_audio.h"
#include "sbc_samp_edit.h"
#include "sbc_waveform.h"

#include "sbc_textbox.h"

void handle_keydown(const uint8_t* keyState)
{
    if(isTextboxEditing())
    {
        if(textboxKeyInput(keyState)) 
                repaintGUI();
    }
#ifdef __APPLE__
    else if (keyState[SDL_SCANCODE_LGUI] || keyState[SDL_SCANCODE_RGUI])
#else
    else if (keyState[SDL_SCANCODE_LCTRL] || keyState[SDL_SCANCODE_RCTRL])
#endif
    {

        if(keyState[SDL_SCANCODE_Q])
            quitProgram();

        else if(keyState[SDL_SCANCODE_S])
            saveFileDialog();

        else if(keyState[SDL_SCANCODE_O])
            openFileDialog();      

        else if(keyState[SDL_SCANCODE_TAB])
        {
            click_button(getOptButton(), true);
            repaintGUI();
        }
        else if(optionsIsShowing())
            return;
        else if(keyState[SDL_SCANCODE_A])
        {
            handleSelectAll();
            repaintWaveform();
            repaintWaveform();
        }

        else if(keyState[SDL_SCANCODE_X])
        {
            audioPaused();

            if(keyState[SDL_SCANCODE_LSHIFT] || keyState[SDL_SCANCODE_RSHIFT])
            {
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
            else if(handleSampleCut())
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

        else if(keyState[SDL_SCANCODE_C])
            handleSampleCopy();

        else if(keyState[SDL_SCANCODE_V])
        {
            audioPaused();

            if(handleSamplePaste())
            {
                drawNewWave();
                repaintWaveform();
                repaintGUI();
            }
        }

        else if(keyState[SDL_SCANCODE_Z])
        {
            audioPaused();
            
            if(handleUndo())
            {
                drawNewWave();
                repaintWaveform();
                repaintGUI();
            }
        }
    }
    else if (keyState[SDL_SCANCODE_LALT] || keyState[SDL_SCANCODE_RALT]) {}
    else
    {
        if(keyState[SDL_SCANCODE_SPACE])
        {
            click_button(getPlayButton(), true);

            repaintScreen();
        }

        if(optionsIsShowing()) return;

        if(keyState[SDL_SCANCODE_LEFT])
        {
            if(keyState[SDL_SCANCODE_RIGHT]) return;
            handleScroll(true);

            repaintWaveform();
        }

        else if(keyState[SDL_SCANCODE_RIGHT])
        {
            if(keyState[SDL_SCANCODE_LEFT]) return;
            handleScroll(false);

            repaintWaveform();
        }

        else if(keyState[SDL_SCANCODE_KP_PLUS])
        {
            if(keyState[SDL_SCANCODE_KP_MINUS]) return;

            handleZoom(true);
        }                

        else if(keyState[SDL_SCANCODE_KP_MINUS])
        {
            if(keyState[SDL_SCANCODE_KP_PLUS]) return;

            handleZoom(false);
        }

        else if(keyState[SDL_SCANCODE_DELETE])
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
    }
}

void handle_keyup(const uint8_t* keyState)
{
    if(wasButtonClicked(getOptButton()))
    {
        click_button(getOptButton(), false);

        if(optionsIsShowing())
        {
            hideOptions();
            repaintWaveform();
        }
        else showOptions();

        repaintGUI();
    }
    
    if (keyState[SDL_SCANCODE_LCTRL] || keyState[SDL_SCANCODE_RCTRL])
    {
    }
    // else if (keyState[SDL_SCANCODE_LALT] || keyState[SDL_SCANCODE_RALT]) {}
    else if (!isTextboxEditing())
    {
        if(wasButtonClicked(getPlayButton()))
        {
                click_button(getPlayButton(), false);

                if(*audioQueued())pauseAudio();
                else queueAudio();

                repaintScreen();
        }
    }
}
