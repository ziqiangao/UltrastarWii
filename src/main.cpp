#include <ogc/console.h>
#include <fat.h>
#include <stdio.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "audioplayback.hpp"
#include "bgvideo.hpp"   // provides videoloadandplay()

void waitforpress()
{
    printf("Press any button to continue...\n");
    while (1)
    {
        WPAD_ScanPads();
        u32 buttons = 0;
        buttons |= WPAD_ButtonsDown(0);
        buttons |= WPAD_ButtonsDown(1);
        buttons |= WPAD_ButtonsDown(2);
        buttons |= WPAD_ButtonsDown(3);
        VIDEO_WaitVSync();
        if (buttons)
            return;
    }
}

void *audthread(void *arg)
{
    audioloadandplay("sd:/UltrastarWiiSongs/song.mp3");
    return 0;
}

int main(void)
{
    VIDEO_Init();
    WPAD_Init();

    // Get the preferred video mode
    GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate framebuffer
    void *fb = SYS_AllocateFramebuffer(rmode);

    // Configure video
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(fb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    // Determine stride
    int stride = rmode->fbWidth * VI_DISPLAY_PIX_SZ;

    // Initialise console on the framebuffer
    CON_Init(fb, 0, 0, rmode->fbWidth, rmode->xfbHeight, stride);

    // Initialise FAT
    fatInitDefault();

    // Start audio thread (safe to run in worker)
    lwp_t thread_idaud;
    s32 r0 = LWP_CreateThread(&thread_idaud, audthread, 0, NULL, 0, 50);
    if (r0 != 0)
    {
        printf("Failed to create audio thread!\n");
        waitforpress();
        return 1;
    }

    // Run video playback on the main thread (GX must run on main)
    // videoloadandplay will initialise its own GX texture/state if needed.
    if (videoloadandplay("sd:/UltrastarWiiSongs/song.h264") != 0)
    {
        printf("Video playback failed\n");
    }

    waitforpress();
    return 0;
}
