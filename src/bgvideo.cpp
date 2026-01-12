#include "decode.hpp"
#include <ogc/gx.h>
#include <gccore.h>
#include <ogc/gu.h>
#include <ogc/machine/processor.h>
#include <malloc.h>
#include <string>

#define TEX_W 1024
#define TEX_H 512

class H264GXDecoder : public H264StreamDecoder
{
private:
    u32 *texBuf;
    GXTexObj texObj;

public:
    H264GXDecoder() : texBuf(nullptr)
    {
        // allocate texture buffer
        texBuf = (u32 *)memalign(32, TEX_W * TEX_H * sizeof(u32));
        if (texBuf)
            memset(texBuf, 0, TEX_W * TEX_H * sizeof(u32));

        // init GX texture
        GX_InitTexObj(&texObj, texBuf, TEX_W, TEX_H, GX_TF_RGBA8,
                      GX_CLAMP, GX_CLAMP, GX_FALSE);

        initGXState();
    }

    ~H264GXDecoder()
    {
        if (texBuf)
            free(texBuf);
    }

protected:
    virtual void onPictureReady(u32 *picture, u32 picId, u32 isIdrPic, u32 numErrMbs) override
    {
        // assume picture is RGBA (or convert to RGBA if needed)
        u32 frameW = 854; // replace with actual decoder width if available
        u32 frameH = 480;

        // blit into texture buffer
        for (u32 y = 0; y < frameH; ++y)
        {
            memcpy(texBuf + y * TEX_W, picture + y * frameW, frameW * sizeof(u32));
        }

        // flush to memory and upload to GX
        DCFlushRange(texBuf, TEX_W * TEX_H * sizeof(u32));
        GX_LoadTexObj(&texObj, GX_TEXMAP0);

        // draw quad
        float umax = (float)frameW / TEX_W;
        float vmax = (float)frameH / TEX_H;
        drawQuad(umax, vmax);

        GX_DrawDone();
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }

    virtual void onHeadersReady(u32 width, u32 height) override
    {
        // can use headers to store frame size
        printf("Video size: %u x %u\n", width, height);
    }

    virtual void onError(const std::string &error) override
    {
        printf("Decoder error: %s\n", error.c_str());
    }

private:
    void initGXState()
    {
        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
        GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

        Mtx44 proj;
        guOrtho(proj, 0.0f, 640.0f, 480.0f, 0.0f, -1.0f, 1.0f);
        GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);

        GX_SetViewport(0, 0, 640, 480, 0, 1);
        GX_SetScissor(0, 0, 640, 480);
        GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        GX_SetCullMode(GX_CULL_NONE);
    }

    void drawQuad(float umax, float vmax)
    {
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(0.0f, 0.0f, 0.0f);
        GX_TexCoord2f32(0.0f, 0.0f);
        GX_Position3f32(640.0f, 0.0f, 0.0f);
        GX_TexCoord2f32(umax, 0.0f);
        GX_Position3f32(640.0f, 480.0f, 0.0f);
        GX_TexCoord2f32(umax, vmax);
        GX_Position3f32(0.0f, 480.0f, 0.0f);
        GX_TexCoord2f32(0.0f, vmax);
        GX_End();
    }
};

int videoloadandplay(const char* file)
{
    H264GXDecoder decoder;
    FILE *fp = fopen(file, "rb");
    if (!fp)
        return 1;

    u8 buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        decoder.processChunk(buffer, bytesRead);
    }
    fclose(fp);
    return 0;
}

void stopvideoplayback() {

}