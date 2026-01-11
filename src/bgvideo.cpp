#include "h264bsd/h264bsd_decoder.h"
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogc/gx.h>
#include <gccore.h>
#include <ogc/gu.h>      // guOrtho
#include <ogc/machine/processor.h> // DCFlushRange

#define TEX_W 1024
#define TEX_H 512
#define READ_BUF 4096

static u32 *g_texBuf = NULL;
static GXTexObj g_texObj;

/* minimal GX init - call once before rendering frames */
static void init_gx_state(void)
{
    // Clear previous vertex descriptors
    GX_ClearVtxDesc();

    // Set up vertex formats
    GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    // Setup projection (orthographic)
    Mtx44 proj;
    guOrtho(proj, 0.0f, 640.0f, 480.0f, 0.0f, -1.0f, 1.0f);
    GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);

    // Viewport
    GX_SetViewport(0, 0, 640, 480, 0, 1);
    GX_SetScissor(0, 0, 640, 480);
    GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GX_SetCullMode(GX_CULL_NONE);
}


/* ensure texture buffer exists */
static int ensureTextureBuffer(void)
{
    if (g_texBuf) return 0;
    g_texBuf = (u32*)memalign(32, TEX_W * TEX_H * sizeof(u32));
    if (!g_texBuf) return -1;
    memset(g_texBuf, 0x00, TEX_W * TEX_H * sizeof(u32));
    return 0;
}

/* init texture object (call after ensureTextureBuffer) */
static void initTextureObj(void)
{
    GX_InitTexObj(&g_texObj, g_texBuf, TEX_W, TEX_H, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
}

/* draw full-screen quad, mapping used portion (umax/vmax) */
static void draw_quad_scaled(f32 umax, f32 vmax)
{
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    GX_Position3f32(0.0f, 0.0f, 0.0f);               GX_TexCoord2f32(0.0f, 0.0f);
    GX_Position3f32(640.0f, 0.0f, 0.0f);             GX_TexCoord2f32(umax, 0.0f);
    GX_Position3f32(640.0f, 480.0f, 0.0f);           GX_TexCoord2f32(umax, vmax);
    GX_Position3f32(0.0f, 480.0f, 0.0f);             GX_TexCoord2f32(0.0f, vmax);

    GX_End();
}

/* blit RGBA frame into top-left of g_texBuf */
static void blitFrameToTexture(u32 *tex, u32 texW, u32 texH,
                               const u32 *frame, u32 frameW, u32 frameH)
{
    if (!tex || !frame) return;
    u32 copyW = (frameW < texW) ? frameW : texW;
    u32 copyH = (frameH < texH) ? frameH : texH;
    for (u32 y = 0; y < copyH; ++y)
        memcpy(tex + y * texW, frame + y * frameW, copyW * sizeof(u32));
}

/* Robust streaming decode + render loop */
int videoloadandplay(const char *file)
{
    if (ensureTextureBuffer() != 0) {
        printf("texture alloc failed\n");
        return 1;
    }
    initTextureObj();
    init_gx_state();

    FILE *fp = fopen(file, "rb");
    if (!fp) {
        printf("Failed to open %s\n", file);
        return 1;
    }

    /* streaming read buffer */
    u8 *rbuf = (u8*)malloc(READ_BUF);
    if (!rbuf) { fclose(fp); return 1; }
    size_t have = 0;    /* bytes currently in rbuf */

    storage_t *decoder = h264bsdAlloc();
    if (!decoder) { free(rbuf); fclose(fp); return 1; }
    if (h264bsdInit(decoder, 0) != 0) { free(rbuf); fclose(fp); h264bsdFree(decoder); return 1; }

    int running = 1;
    while (running) {
        /* fill buffer if low */
        if (have < READ_BUF/2) {
            size_t r = fread(rbuf + have, 1, READ_BUF - have, fp);
            have += r;
            if (r == 0 && have == 0) { /* EOF and nothing left */
                break;
            }
        }

        /* decode from rbuf */
        u8 *ptr = rbuf;
        while (have > 0) {
            u32 bytesConsumed = 0;
            u32 status = h264bsdDecode(decoder, ptr, (u32)have, 0, &bytesConsumed);

            /* handle statuses */
            if (status == H264BSD_PIC_RDY) {
                u32 picId, isIdrPic, numErrMbs;
                u32 *picData = h264bsdNextOutputPictureRGBA(decoder, &picId, &isIdrPic, &numErrMbs);
                if (picData) {
                    /* Obtain actual frame size from decoder if available.
                       Replace hardcoded values with decoder accessors if your build exposes them. */
                    u32 frameW = 854; /* <-- replace if possible */
                    u32 frameH = 480;

                    blitFrameToTexture(g_texBuf, TEX_W, TEX_H, picData, frameW, frameH);

                    /* flush and upload (umax/vmax map used region) */
                    DCFlushRange(g_texBuf, TEX_W * TEX_H * sizeof(u32));
                    GX_LoadTexObj(&g_texObj, GX_TEXMAP0);

                    f32 umax = (f32)frameW / (f32)TEX_W;
                    f32 vmax = (f32)frameH / (f32)TEX_H;
                    draw_quad_scaled(umax, vmax);

                    GX_DrawDone();
                    VIDEO_Flush();
                    VIDEO_WaitVSync();
                }
            } else if (status == H264BSD_HDRS_RDY || status == H264BSD_RDY) {
                /* keep feeding; nothing to render yet */
            } else if (status == H264BSD_ERROR || status == H264BSD_PARAM_SET_ERROR || status == H264BSD_MEMALLOC_ERROR) {
                printf("Decoder error: status=%u\n", status);
                running = 0;
                break;
            }

            /* Advance by bytesConsumed if decoder consumed data.
               If bytesConsumed == 0, break to read more bytes from file. */
            if (bytesConsumed > 0) {
                ptr += bytesConsumed;
                have -= bytesConsumed;
            } else {
                /* Need more input to proceed; exit inner loop and read more */
                break;
            }
        }

        /* move remaining bytes to start of buffer */
        if (have > 0 && ptr != rbuf) {
            memmove(rbuf, ptr, have);
        }

        /* if EOF and nothing in buffer -> stop */
        if (feof(fp) && have == 0) break;
    }

    /* cleanup */
    h264bsdShutdown(decoder);
    h264bsdFree(decoder);
    free(rbuf);
    fclose(fp);

    return 0;
}
