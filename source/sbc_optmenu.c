#include <SDL2/SDL.h>

#include "sbc_utils.h"

#include "sbc_optmenu.h"
#include "sbc_audio.h"
#include "sbc_screen.h"
#include "sbc_gui.h"

static bool show_optmenu = false, update_optmenu = false;
static const Rect_t astriid_rect = { 5, 155, 160, 16, 0 };

static Button_t *interpolationButtons[4], *bufferSizeButtons[4], 
                *deviceSampRateButtons[4], *wavExport[2], *brr_button;

static Select_Menu_t *audioDrvMenu, *outputDevMenu, *inputDevMenu;

static void initAudioDrvMenu(void)
{
    audioDrvMenu = createSelectMenu("Audio Devices:", SDL_GetNumAudioDrivers(), 
                                    (Rect_t) { 0,  16, 154, 74, 0  }, SBCMPURPLE);

    for(int i = 0; i < audioDrvMenu->max; i++)
        initSelectMenuItem(audioDrvMenu, i, SDL_GetAudioDriver(i));

    setSelectMenuSelection(audioDrvMenu, getCurrentAudioDriver());
}

static void initOutputDevMenu(void)
{
    outputDevMenu = createSelectMenu("Output Devices:", SDL_GetNumAudioDevices(0), 
                                    (Rect_t) { 154, 16, 233, 94, 0 }, SBCMPURPLE);

    for(int i = 0; i < outputDevMenu->max; i++)
    {
        const char* dev_name = SDL_GetAudioDeviceName(i, 0);
        
        initSelectMenuItem(outputDevMenu, i, dev_name);
    }

    setSelectMenuSelection(outputDevMenu, getCurrentOutDev());
}

static void initInputDevMenu(void)
{
    inputDevMenu = createSelectMenu("Input Devices:", SDL_GetNumAudioDevices(1), 
                                    (Rect_t) { 154, 123, 233, 54, 0 }, SBCMPURPLE);

    for(int i = 0; i < inputDevMenu->max; i++)
        initSelectMenuItem(inputDevMenu, i, SDL_GetAudioDeviceName(i, 1));

    setSelectMenuSelection(inputDevMenu, getCurrentInDev());
}

void initOptMenu(void)
{
    initRadButtons(interpolationButtons, (Rect_t) { 392, 94, 68, 13, SBCDPURPLE }, 4,
                    (const char*[]) {"NEAREST", "LINEAR", "CUBIC", "GAUSSIAN"}, SBCDPURPLE, 1, 
                    getCurrentInterpolationType());

    initRadButtons(bufferSizeButtons, (Rect_t) { 390, 31, 64, 11, SBCDPURPLE }, 4,
                    (const char*[]) {"256", "512", "1024", "2048"}, 0xFF000000, 1, 
                    getAudioBufferSize());

    initRadButtons(deviceSampRateButtons, (Rect_t) { 460, 31, 64, 11, SBCDPURPLE }, 4,
                    (const char*[]) {"32000Hz", "44100Hz", "48000Hz", "96000Hz"}, 0xFF000000, 1, 
                    getDeviceSampleRate());
    
    initRadButtons(wavExport, (Rect_t) { 4, 105, 68, 11, SBCDPURPLE }, 2,
                    (const char*[]) {"8-bit", "16-bit"}, SBCDPURPLE, 1, (int) exporting16bit());

    brr_button = createButton((Rect_t) { 4, SAMPLE_HEIGHT - 26, 146, 11, SBCDPURPLE }, 
                    RAD_BUTTON, "BRR SAMPLE SELECT", 0xFF000000, 1);

    initAudioDrvMenu();
    initOutputDevMenu();
    initInputDevMenu();
}

void freeOptMenu(void)
{
    destroyRadButtons(interpolationButtons, 4);
    destroyRadButtons(bufferSizeButtons, 4);
    destroyRadButtons(deviceSampRateButtons, 4);
    destroyRadButtons(wavExport, 2);

    destroySelectMenu(&audioDrvMenu);
    destroySelectMenu(&outputDevMenu);
    destroySelectMenu(&inputDevMenu);
    
    destroyButton(&brr_button);
}

bool optMenuMouseDown(const int x, const int y)
{
    int selection = -1;

    if(!show_optmenu) return false;

    if((selection = selectMenuHitbox(outputDevMenu, x, y)) > -1)
    {
        if(selection >= outputDevMenu->max || selection == outputDevMenu->current) return false;
        
        setOutputDevice(selection);
        setSelectMenuSelection(outputDevMenu, getCurrentOutDev());
    }
    else if((selection = selectMenuHitbox(inputDevMenu, x, y)) > -1)
    {
        if(selection >= inputDevMenu->max || selection == inputDevMenu->current) return false;
        
        setInputDevice(selection);
        setSelectMenuSelection(inputDevMenu, getCurrentInDev());
    }
    else if((selection = selectMenuHitbox(audioDrvMenu, x, y)) > -1)
    {
        if(selection >= audioDrvMenu->max || selection == audioDrvMenu->current) return false;
        
        setAudioDriver(selection);

        if(audioDrvMenu->current == getCurrentAudioDriver()) return false;

        setSelectMenuSelection(audioDrvMenu, selection);

        destroySelectMenu(&outputDevMenu);
        destroySelectMenu(&inputDevMenu);

        initOutputDevMenu();
        initInputDevMenu();
    }
    else if((selection = radButtonHitbox(interpolationButtons, 4, x, y)) > -1)
    {
        if(selection > GAUSS) return false;

        SBC_LOG(INTERPOLATION, %s, interpolationButtons[selection]->text);

        setInterpolationType(selection);
        radButtonClick(interpolationButtons, 4, getCurrentInterpolationType());
    }
    else if((selection = radButtonHitbox(bufferSizeButtons, 4, x, y)) > -1)
    {
        if(selection > 3) return false;
        if(!setAudioBufferSize(selection)) return false;

        radButtonClick(bufferSizeButtons, 4, getAudioBufferSize());
    }
    else if((selection = radButtonHitbox(deviceSampRateButtons, 4, x, y)) > -1)
    {
        if(selection > 3) return false;
        if(!setDeviceSampleRate(selection)) return false;
        
        radButtonClick(deviceSampRateButtons, 4, getDeviceSampleRate());
    }
    else if((selection = radButtonHitbox(wavExport, 2, x, y)) > -1)
    {
        if(selection > 1) return false;

        set16bitExport(selection);
        radButtonClick(wavExport, 2, exporting16bit());
    }
    else if(hitbox(&brr_button->rect, x, y))
    {
        selection = 1;
        click_button(brr_button, brr_button->clicked ^ 1);
        SBC_LOG(BRR SELECT, %s, brr_button->clicked ? "TRUE" : "FALSE");
    }
    else if(hitbox(&astriid_rect, x, y))
    {
#if defined (_WIN32)
        selection = system("link https://www.youtube.com/watch?v=dQw4w9WgXcQ");
#elif defined (__linux__)
        selection = system("xdg-open https://www.youtube.com/watch?v=dQw4w9WgXcQ");
#elif defined (__APPLE__)
        selection = system("open https://www.youtube.com/watch?v=dQw4w9WgXcQ");
#endif
    }

    return (update_optmenu = (selection > -1));
}

bool optMenuMouseMove(const int x, const int y, const int mouse_is_down)
{
    bool update = false;

    if(!show_optmenu) return update;

    (void) mouse_is_down;

    if(selectMenuHitbox(outputDevMenu, x, y) > -1) update = true;
    else if(selectMenuHitbox(inputDevMenu, x, y) > -1) update = true;
    else if(selectMenuHitbox(audioDrvMenu, x, y) > -1) update = true;
    else if(radButtonHitbox(interpolationButtons, 4, x, y) > -1) update = true;
    else if(radButtonHitbox(bufferSizeButtons, 4, x, y) > -1) update = true;
    else if(radButtonHitbox(deviceSampRateButtons, 4, x, y) > -1) update = true;
    else if(radButtonHitbox(wavExport, 2, x, y) > -1) update = true;
    else if(hitbox(&brr_button->rect, x, y)) update = true;
    else if(hitbox(&astriid_rect, x, y)) update = true;

    return update;
}

static void paintSelectMenus(void)
{
	paint_bevel((Rect_t) {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0}, 0xFF2E2E2E, 0xFFBEBEBE);

    draw_Vline(153, 0, 17, 0xFF2E2E2E);
    draw_Vline(386, 0, 17, 0xFF2E2E2E);
    draw_Vline(154, 0, 17, 0xFFBEBEBE);

    paintSelectMenu(outputDevMenu);
    paintSelectMenu(inputDevMenu);
    paintSelectMenu(audioDrvMenu);
}

static void paintInterpolationSelect(void)
{
    for(int i = 0; i < 4; i++)
    {
        paint_button(interpolationButtons[i]);
        paint_button(bufferSizeButtons[i]);
        paint_button(deviceSampRateButtons[i]);

        if(i < 2) paint_button(wavExport[i]);
    }

    print_string_shadow("Interpolation:", interpolationButtons[0]->rect.x - 1, 
                        interpolationButtons[0]->rect.y - interpolationButtons[0]->rect.h + 2, 
                        (int[]) {1, 1}, (int[]) {0xFF121212, SBCLGREY}, 1 );

    print_string_shadow("Device Settings:", bufferSizeButtons[0]->rect.x - 1, 
                        4, (int[]) {1, 0}, (int[]) {SBCMPURPLE, 0xFF121212}, 1 );

    print_string_shadow("Buffer:", bufferSizeButtons[0]->rect.x - 1, 
                        bufferSizeButtons[0]->rect.y - bufferSizeButtons[0]->rect.h, 
                        (int[]) {1, 1}, (int[]) {0xFF121212, SBCLGREY}, 1 );

    print_string_shadow("Rate:", deviceSampRateButtons[0]->rect.x - 1, 
                        deviceSampRateButtons[0]->rect.y - deviceSampRateButtons[0]->rect.h, 
                        (int[]) {1, 1}, (int[]) {0xFF121212, SBCLGREY}, 1 );

    print_string_shadow("Export Bit Depth:", wavExport[0]->rect.x - 1,  wavExport[0]->rect.y - wavExport[0]->rect.h, 
                        (int[]) {1, 1}, (int[]) {0xFF121212, SBCLGREY}, 1 );
}

void paintOptions(void)
{
    if(!show_optmenu || !update_optmenu) return;

    SBC_LOG(PAINTING OPTIONS, %s, "TRUE");

    paint_bevel((Rect_t) { 0, 0, SCREEN_WIDTH, SAMPLE_HEIGHT + 18, 0}, 0xFF2E2E2E, 0xFFBEBEBE);
    paint_bevel((Rect_t) { 0, 148, 155, 30, 0}, 0xFF2E2E2E, 0xFFBEBEBE);

    paint_bevel((Rect_t) { 0, 90, outputDevMenu->rect.x + 1, 40, 0}, 0xFF2E2E2E, 0xFFBEBEBE);
    paint_bevel((Rect_t) { 0, 130, outputDevMenu->rect.x + 1, 18, 0}, 0xFF2E2E2E, 0xFFBEBEBE);
    paint_bevel((Rect_t) { 387, 78, 155, 70, 0}, 0xFF2E2E2E, 0xFFBEBEBE);

    paint_bevel((Rect_t) { 387, 148, SCREEN_WIDTH - 387, 30, 0}, 0xFF2E2E2E, 0xFFBEBEBE);
    paint_bevel((Rect_t) { 387,   0, SCREEN_WIDTH - 387, 16, 0}, 0xFF2E2E2E, 0xFFBEBEBE);

    paint_bevel((Rect_t) { 387, 16, 68, 62, 0}, 0xFF2E2E2E, 0xFFBEBEBE);
    paint_bevel((Rect_t) { 455, 16, 85, 62, 0}, 0xFF2E2E2E, 0xFFBEBEBE);

    print_string_shadow("~ASTRIID~", astriid_rect.x, astriid_rect.y, (int[]) {1, 1}, (int[]) {SBCMPURPLE, SBCLGREY}, 2 );
    print_string_shadow("Default Samp Dir:", 390, 150, (int[]) {1, 1}, (int[]) {0xFF121212, SBCLGREY}, 1 );

    paintSelectMenus();
    paintInterpolationSelect();

    paint_button(brr_button);

    repaintGUI();
    update_optmenu = false;
}

void hideOptions(void)
{
    SBC_LOG(OPT MENU SHOWING, %s, "FALSE");
    SBC_LOG(WAVEFORM SHOWING, %s, "TRUE");

    show_optmenu = update_optmenu = false;
    clearScreenArea(0, SBCDGREY, SCREEN_WIDTH * (SAMPLE_HEIGHT + 20));
}

void showOptions(void) 
{ 
    SBC_LOG(OPT MENU SHOWING, %s, "TRUE");
    SBC_LOG(WAVEFORM SHOWING, %s, "FALSE");

    show_optmenu = update_optmenu = true;
    clearScreenArea(0, SBCDGREY, SCREEN_WIDTH * (SAMPLE_HEIGHT + 20));
}

void repaintOptions(void) { if(show_optmenu) update_optmenu = true; }
bool optionsIsShowing(void) { return show_optmenu; }
Button_t* getBrrButton(void)  { return brr_button; }
