#include <SDL2/SDL.h>

#include "sbc_utils.h"
#include "sbc_mouse.h"
#include "sbc_gui.h"
#include "sbc_optmenu.h"
#include "sbc_waveform.h"

static bool mouse_is_down = false;
static uint64_t double_click = 0;
static int last_x = 0;

static void scale_mouse(int* x, int *y, const int scale)
{
    int abs_x, abs_y;
    (void) SDL_GetMouseState(&abs_x,  &abs_y);

    *x = abs_x >> scale;
    *y = abs_y >> scale;
}

void handleMouse(const void *event)
{
    const SDL_Event *e = (const SDL_Event*) event;
    int mouse_x, mouse_y, wave_focus;
    
    if(e->window.event == SDL_WINDOWEVENT_LEAVE)
    {
        scale_mouse(&mouse_x, &mouse_y, *getWindowScale());

        if(mouse_x >= SCREEN_WIDTH - 1) mouse_x = SCREEN_WIDTH;

        // scroll waveform when mouse drags selection area beyond window
        if(mouse_is_down)
        {
            wave_focus = scr2samp(mouse_x);
            setMouseFocus(wave_focus);
            if(!isMouseOnSlider()) setSelEnd(wave_focus);
            repaintWaveform();
        }
    }

    if(e->window.event == SDL_WINDOWEVENT_ENTER)
        mouse_is_down = SDL_GetGlobalMouseState(NULL,NULL) & 1;

    switch(e->type)
    {
    case SDL_MOUSEBUTTONDOWN:

        scale_mouse(&mouse_x, &mouse_y, *getWindowScale());
        if(mousedn(mouse_x, mouse_y)) repaintGUI();

        if(optionsIsShowing() || mouse_is_down) return;

        if(mouse_y < SAMPLE_HEIGHT)
        {
            setWaveSelecting(true);

            wave_focus = scr2samp(mouse_x);
            setCursor(wave_focus);

            repaintWaveform();

            if(double_click + 500 > SDL_GetTicks64())
            {
                handleSelectAll();
                double_click = 0;
            }
            else
            {
                mouse_is_down = true;
                double_click = SDL_GetTicks64();
            }
        }
        else
        {
            setWaveSelecting(false);
        }

    break;

    case SDL_MOUSEBUTTONUP:

        mouse_is_down = false;
        
        scale_mouse(&mouse_x, &mouse_y, *getWindowScale());
        if(mouseup(mouse_x, mouse_y)) repaintGUI();
        
        break;

    case SDL_MOUSEMOTION:

        scale_mouse(&mouse_x, &mouse_y, *getWindowScale());
        if(mousemotion(mouse_x, mouse_y) && !optionsIsShowing()) repaintWaveform();

        if(optionsIsShowing()) return;        

        if(mouse_x > SCREEN_WIDTH && isZoomed())
        {
            if(mouse_is_down && last_x < mouse_x)
            {
                handleScroll(false);
                wave_focus = scr2samp(SCREEN_WIDTH);
            }
        }
        else if (mouse_x < 0 && isZoomed())
        {
            if(mouse_is_down && last_x > mouse_x)
            {
                handleScroll(true);
                wave_focus = scr2samp(0);
            }
        }
        else
        {
            if(mouse_y < SAMPLE_HEIGHT || mouse_is_down)
            {
                wave_focus = scr2samp(mouse_x);
                setMouseFocus(wave_focus);
            }
        }

        if (mouse_is_down && *waveIsSelecting())
        {
            double_click = 0;

            if(!isMouseOnSlider())
            {
                wave_focus = scr2samp(mouse_x);
                setSelEnd(wave_focus);

                repaintWaveform();
            }
        }

        last_x = mouse_x;
    
        break;

    case SDL_MOUSEWHEEL:

        if(optionsIsShowing()) return;        

        scale_mouse(&mouse_x, &mouse_y, *getWindowScale());
            
        if (mouse_y < SAMPLE_HEIGHT)
        {
            if (e->wheel.y > 0) handleZoom(true);

            else if (e->wheel.y < 0) handleZoom(false);

            else if (e->wheel.x < 0)
            {
                wave_focus = scr2samp(mouse_x);
                setMouseFocus(wave_focus);

                handleScroll(true);
            }

            else if (e->wheel.x > 0)
            {
                wave_focus = scr2samp(mouse_x);
                setMouseFocus(wave_focus);

                handleScroll(false);
            }

            repaintWaveform();
            repaintGUI();
        }
        
        break;
    }
}
