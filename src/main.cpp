#include <ogc/console.h>
#include <fat.h>
#include <stdio.h>
#include <wiiuse/wpad.h>
#include "audioplayback.hpp"

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

    // Use full screen, starting at 0,0
    CON_Init(fb, 0, 0, rmode->fbWidth, rmode->xfbHeight, stride);


    fatInitDefault();

    int status = loadandplay("sd:/UltrastarWiiSongs/song.mp3");

    waitforpress();

    return 0;
}