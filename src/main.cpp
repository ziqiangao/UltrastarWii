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

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define TEX_WIDTH 640
#define TEX_HEIGHT 360

// --- timing helpers (same as before) ---
u64 lasttime = 0;

double dt()
{
    u64 now = gettime();
    double delta = diff_msec(lasttime ? lasttime : now, now) / 1000.0;
    lasttime = now;
    return delta;
}

// --- fast integer YCbCr -> RGB565 conversion for Wii ---
static inline u16 ycbcr_to_rgb565_int(int yv, int cbv, int crv)
{
    // Use ITU-R BT.601-ish integer approximations
    int r = (298 * (yv - 16) + 409 * (crv - 128) + 128) >> 8;
    int g = (298 * (yv - 16) - 100 * (cbv - 128) - 208 * (crv - 128) + 128) >> 8;
    int b = (298 * (yv - 16) + 516 * (cbv - 128) + 128) >> 8;

    if (r < 0)
        r = 0;
    else if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    else if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    else if (b > 255)
        b = 255;

    return (u16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// --- audio thread ---
void *audthread(void *arg)
{
    audioloadandplay("sd:/UltrastarWiiSongs/song.mp3");
    return NULL;
}

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

// --- pl_mpeg video callback: writes into GRRLIB texture data ---
void vid_decode(plm_t *plm, plm_frame_t *frame, void *user)  
{  
    if (!frame || !user) return;  
  
    GRRLIB_texImg *tex = (GRRLIB_texImg*)user;  
    u16 *dst = (u16*)tex->data;  
  
    const int w = frame->width;  
    const int h = frame->height;  
  
    uint8_t *Y  = frame->y.data;  
    uint8_t *Cb = frame->cb.data;  
    uint8_t *Cr = frame->cr.data;  
  
    const int plane_w   = frame->y.width;  
    const int chroma_w  = frame->cb.width;  
  
    // CRITICAL: Use tex->w for output stride, not frame width  
    const int tex_stride = tex->w;  
  
    for (int yy = 0; yy < h; yy++)  
    {  
        int y_index = yy * plane_w;  
        int c_index = (yy / 2) * chroma_w;  
        for (int xx = 0; xx < w; xx++)  
        {  
            int Yv  = Y[y_index + xx];  
            int Cbv = Cb[c_index + (xx / 2)];  
            int Crv = Cr[c_index + (xx / 2)];  
  
            dst[yy * tex_stride + xx] = ycbcr_to_rgb565_int(Yv, Cbv, Crv);  
        }  
    }  
  
    GRRLIB_FlushTex(tex);  
}

int main(void)
{
    CON_EnableGecko(1, true);
    SYS_STDIO_Report(true);
    // init libOGC subsystems we still use
    VIDEO_Init();
    WPAD_Init();

    // init GRRLIB (this calls GX_Init internally and sets modes)
    if (GRRLIB_Init() < 0)
    {
        printf("GRRLIB_Init failed\n");
        return 1;
    }

    // optional background clear colour
    GRRLIB_SetBackgroundColour(0, 0, 0, 255);

    // init FAT (GRRLIB may add FAT to devoptab, but safe to call)
    fatInitDefault();

    // start audio playback thread
    lwp_t thread_idaud;
    s32 r0 = LWP_CreateThread(&thread_idaud, audthread, NULL, NULL, 0, 50);
    if (r0 != 0)
    {
        printf("Failed to create audio thread!\n");
        waitforpress();
        return 1;
    }

    // open mpg file
    FILE *file = fopen("sd:/UltrastarWiiSongs/song.mpg", "rb");
    if (!file)
    {
        printf("Video File Open Fail\n");
        waitforpress();
        return 1;
    }

    // create decoder from FILE*
    plm_t *plm = plm_create_with_file(file, true);
    if (!plm)
    {
        printf("Video playback failed\n");
        waitforpress();
        return 1;
    }

    // create an empty GRRLIB texture with RGB565 format
    GRRLIB_texImg *video_tex = GRRLIB_CreateEmptyTextureFmt(TEX_WIDTH, TEX_HEIGHT, GX_TF_RGB565);
    if (!video_tex)
    {
        printf("Failed to create GRRLIB texture\n");
        plm_destroy(plm);
        waitforpress();
        return 1;
    }
    if (video_tex)
    {
        printf("Texture created: requested=%dx%d, actual=%dx%d\n",
               TEX_WIDTH, TEX_HEIGHT, video_tex->w, video_tex->h);
    }

    // ensure the tex w/h match requested resolution (tex->w may be padded)
    // We'll draw the texture at screen size, so scaling will be handled by GRRLIB_DrawImg.

    // register callback; pass GRRLIB_texImg* as user pointer
    plm_set_video_decode_callback(plm, vid_decode, video_tex);

    // main loop: decode and render using GRRLIB
    dt();
    while (!plm_has_ended(plm))
    {
        // advance decoder (fixed step avoids audio drift)
        plm_decode(plm, dt());

        // render with GRRLIB
        GRRLIB_2dMode(); // ensure 2D mode
        // draw at (0,0) scale 1.0: depends on how you want to position/scale - this draws top-left
        // To draw fullscreen, use (0,0) and scale to screen size. Here we draw at 0,0 and let GRRLIB scale if needed:
        GRRLIB_DrawImg(0, 0, video_tex, 0, 1.0, 1.0, 0xFFFFFFFF);

        // submit frame
        GRRLIB_Render();
    }

    // cleanup
    plm_destroy(plm);
    GRRLIB_FreeTexture(video_tex);
    GRRLIB_Exit();

    waitforpress();
    return 0;
}
