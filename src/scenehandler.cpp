#include "scenehandler.hpp"
#include "globalstop.h"
#include <grrlib.h>

void drawTitleScreen()
{
}

void drawConfigScreen()
{
}

void drawSongSelectScreen()
{
}

void drawSongScreen()
{
}

static Scene currentscene = Scene::Title;

int drawscene()
{
    switch (currentscene)
    {
    case Scene::Title:
        drawTitleScreen();
        break;

    case Scene::Config:
        drawConfigScreen();
        break;

    case Scene::SongSelect:
        drawSongSelectScreen();
        break;

    case Scene::Song:
        drawSongScreen();
        break;

    default:
        return -1;
    }
    if (main_thread_vsync) main_thread_vsync();
    VIDEO_WaitVSync();
    return 0;
}

void changescene(Scene scene)
{
    currentscene = scene;
}

Scene getscene()
{
    return currentscene;
}

/*
s32 my_mp3_reader(void *cb_data, void *buffer, s32 len)
{
    FILE *file = (FILE *)cb_data;
    return fread(buffer, 1, len, file);
}

// --- audio thread ---
void *audthread(void *arg)
{
    audioloadandplay("sd:/UltrastarWiiSongs/song.mp3");
    return NULL;
}

volatile u8 globalstop = 0;
volatile u8 resetflag = 0;

void pwrcallback() {
    globalstop = 1;
}

void rstcallback(u32 irq, void* ctx) {
    globalstop = 1;
    resetflag = 1;
}

void vsync() {

}

int main(void)
{
    globalstop = 0;
    CON_EnableGecko(1, true);
    SYS_STDIO_Report(true);
    // init libOGC subsystems we still use
    WPAD_Init();

    // init FAT (GRRLIB may add FAT to devoptab, but safe to call)
    fatInitDefault();
    VIDPlayer_Init();
    MP3Player_Init();


    SYS_SetPowerCallback(pwrcallback);
    SYS_SetResetCallback(rstcallback);

    // start audio playback thread

    globalstop = 0;
    lwp_t thread_idaud;
    s32 r0 = LWP_CreateThread(&thread_idaud, audthread, NULL, NULL, 0, 70);
    if (r0 != 0)
    {
        printf("Failed to create audio thread!\n");
        return 1;
    }

    videoloadandplay("sd:/UltrastarWiiSongs/song.mpg");

    GRRLIB_Exit();

    if (resetflag) {
        SYS_ResetSystem(SYS_RETURNTOMENU,0,0);
        return 0;
    }

    // waitforpress();
    return 0;
}

*/