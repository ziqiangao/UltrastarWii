#define PL_MPEG_IMPLEMENTATION
#include "../bridge/pl_mpeg.h"

#define TEX_WIDTH 428
#define TEX_HEIGHT 240

#include <grrlib.h>
#include <fat.h>
#include <stdio.h>
#include <gctypes.h>
#include "../bridge/bgvideo.hpp"
#include "../bridge/audioplayback.hpp"
#include <ogc/lwp_watchdog.h>

static int cr_r[256];
static int cb_b[256];
static int cb_g[256];
static int cr_g[256];

void VIDPlayer_Init()
{
    for (int i = 0; i < 256; i++)
    {
        int c = i - 128;
        cr_r[i] = (359 * c) >> 8;
        cb_b[i] = (454 * c) >> 8;
        cb_g[i] = (88 * c) >> 8;
        cr_g[i] = (183 * c) >> 8;
    }
    // init GRRLIB (this calls GX_Init internally and sets modes)
    if (GRRLIB_Init() < 0)
    {
        printf("GRRLIB_Init failed\n");
    }
}

// --- timing helpers (same as before) ---
u64 lasttime = 0;

static double dt()
{
    u64 now = mp3time;
    double delta = (mp3time - lasttime) / 48000.0;
    lasttime = now;
    // printf("%f\n",delta);
    return delta;
}

// --- fast integer YCbCr -> RGB565 conversion for Wii ---
static inline u16 ycbcr_to_rgb565_lut(int y, int cb, int cr)
{
    int r = y + cr_r[cr];
    int g = y - cb_g[cb] - cr_g[cr];
    int b = y + cb_b[cb];

    if (r & ~255)
        r = (r < 0) ? 0 : 255;
    if (g & ~255)
        g = (g < 0) ? 0 : 255;
    if (b & ~255)
        b = (b < 0) ? 0 : 255;

    return (u16)(((r & 0xF8) << 8) |
                 ((g & 0xFC) << 3) |
                 (b >> 3));
}

static inline void GRRLIB_SetPixelRGB565(
    GRRLIB_texImg *tex,
    int x,
    int y,
    u16 rgb565)
{
    // GX tiles RGB565 as 4×4 texels, 16 bits each
    // Each tile = 4*4*2 = 32 bytes

    const int tile_w = 4;
    const int tile_h = 4;

    int tiles_per_row = tex->w / tile_w;

    int tile_x = x / tile_w;
    int tile_y = y / tile_h;

    int in_tile_x = x & 3;
    int in_tile_y = y & 3;

    int tile_index = tile_y * tiles_per_row + tile_x;
    int tile_offset = tile_index * 32; // bytes per tile

    int pixel_offset = (in_tile_y * tile_w + in_tile_x) * 2;

    u8 *base = (u8 *)tex->data;
    *(u16 *)(base + tile_offset + pixel_offset) = rgb565;
}

// --- pl_mpeg video callback: writes into GRRLIB texture data ---
// Replace vid_decode with this faster implementation
static void vid_decode(plm_t *plm, plm_frame_t *frame, void *user)
{
    if (!frame || !user)
        return;

    GRRLIB_texImg *tex = (GRRLIB_texImg *)user;

    const int w = frame->width;
    const int h = frame->height;

    uint8_t *Y = frame->y.data;
    uint8_t *Cb = frame->cb.data;
    uint8_t *Cr = frame->cr.data;

    const int plane_w = frame->y.width;
    const int chroma_w = frame->cb.width;

    const int tex_w = tex->w; // may be padded
    const int tile_w = 4;
    const int tiles_per_row = tex_w / tile_w;
    u8 *base = (u8 *)tex->data;

    // For each output row: compute tile row base once, then write pixels with pointer increments.
    for (int yy = 0; yy < h; yy++)
    {
        int y_index = yy * plane_w;
        int c_index = (yy / 2) * chroma_w;

        int tile_y = yy / 4;
        int in_tile_y = yy & 3;
        int tile_row_offset = tile_y * tiles_per_row * 32; // 32 bytes per tile

        // start at tile_x = 0, in_tile_x = 0
        int tile_x = 0;
        int in_tile_x = 0;

        // pointer to first pixel of this row within the tiled texture
        u8 *dst_ptr = base + tile_row_offset + (tile_x * 32) + ((in_tile_y * 4 + in_tile_x) * 2);

        for (int xx = 0; xx < w; xx++)
        {
            int Yv = Y[y_index + xx];
            int Cbv = Cb[c_index + (xx >> 1)];
            int Crv = Cr[c_index + (xx >> 1)];

            u16 px = ycbcr_to_rgb565_lut(Yv, Cbv, Crv);

            // write pixel and advance pointer
            *(u16 *)dst_ptr = px;
            dst_ptr += 2;

            in_tile_x++;
            if (in_tile_x == 4)
            {
                // move to start of next tile in the same tile row
                tile_x++;
                in_tile_x = 0;
                dst_ptr = base + tile_row_offset + (tile_x * 32) + ((in_tile_y * 4) * 2);
            }
        }
    }

    // Single, necessary upload call for this frame
    GRRLIB_FlushTex(tex);
}

static u8 stop = 0;
static float ddtt = 0;
void (*main_thread_vsync)(void);

int videoloadandplay(const char *filename)
{
    stop = 0;
    printf("Loading Video\n");
    // open mpg file
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Video File Open Fail\n");
        return 1;
    }

    // create decoder from FILE*
    plm_t *plm = plm_create_with_file(file, true);
    if (!plm)
    {
        printf("Video playback failed\n");
        return 1;
    }

    // create an empty GRRLIB texture with RGB565 format
    GRRLIB_texImg *video_tex = GRRLIB_CreateEmptyTextureFmt(TEX_WIDTH, TEX_HEIGHT, GX_TF_RGB565);
    if (!video_tex)
    {
        printf("Failed to create GRRLIB texture\n");
        plm_destroy(plm);
        return 1;
    }

    // ensure the tex w/h match requested resolution (tex->w may be padded)
    // We'll draw the texture at screen size, so scaling will be handled by GRRLIB_DrawImg.

    // register callback; pass GRRLIB_texImg* as user pointer
    plm_set_video_decode_callback(plm, vid_decode, video_tex);

    plm_set_audio_enabled(plm, false);
    // main loop: decode and render using GRRLIB
    dt();
    while (!plm_has_ended(plm))
    {

        if (stop || globalstop)
        {
            printf("Stopped Video\n");
            break;
        }

        ddtt = dt();
        // advance decoder (fixed step avoids audio drift)
        plm_decode(plm, ddtt);

        // render with GRRLIB
        GRRLIB_2dMode(); // ensure 2D mode
        // draw at (0,0) scale 1.0: depends on how you want to position/scale - this draws top-left
        // To draw fullscreen, use (0,0) and scale to screen size. Here we draw at 0,0 and let GRRLIB scale if needed:
        GRRLIB_DrawImg(0, 0, video_tex, 0, 1.5, 2, 0xFFFFFFFF);

        // submit frame
        if (main_thread_vsync)
            main_thread_vsync();
        GRRLIB_Render();
    }

    // cleanup
    plm_destroy(plm);
    GRRLIB_FreeTexture(video_tex);
    return 0;
}

void stopvideoplayback()
{
    stop = 1;
}

static u8 video_playing = 0;

int videoloadandplay_safe(const char *filename)
{
    if (video_playing)
        return 1;
    video_playing = 1;

    int result = videoloadandplay(filename);

    video_playing = 0;
    return result;
}

void play_video(const char *filename)
{
    stopvideoplayback(); // stop previous video
    while (video_playing)
        VIDEO_WaitVSync();           // wait until fully stopped
    videoloadandplay_safe(filename); // start new video
}
