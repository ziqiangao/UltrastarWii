// player_grrlib.c
#include <grrlib.h>
#include <ogc/console.h>
#include <fat.h>
#include <stdio.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "audioplayback.hpp"
#include <malloc.h>
#include <string.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <mp3player.h>
#include "bgvideo.hpp"
#include "globalstop.h"

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

