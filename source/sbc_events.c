
#include "sbc_utils.h"

#include "sbc_events.h"

#include "sbc_gui.h"
#include "sbc_buttons.h"
#include "sbc_window.h"

#include "sbc_keys.h"
#include "sbc_textbox.h"
#include "sbc_mouse.h"

void handleEvents(const void *event)
{
        const SDL_Event *e = (const SDL_Event*) event;
        const uint8_t* keyState	= SDL_GetKeyboardState(NULL);
        
        switch(e->type)
        {
        case SDL_QUIT:
                quitProgram();

                break;

        case SDL_DROPFILE:

                loadSample(e->drop.file);        
                break;

        case SDL_TEXTINPUT:

                if (textboxInputText(e->text.text[0]))
                        repaintGUI();
                break;

        case SDL_KEYDOWN:
                
                handle_keydown(keyState);

                break;

        case SDL_KEYUP:

                handle_keyup(keyState);

                break;

        default:
                handleMouse(e);

                break;
        }

}
