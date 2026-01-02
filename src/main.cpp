#include <grrlib.h>
#include <wiiuse/wpad.h>
#include <gccore.h>
#include "NotoSans-SemiBold_ttf.h"
#include <mii.h>
#include <string>
#include <sstream> // for std::ostringstream
#include <cstring>
#include "miidraw.h"
#include <utility>
#include "miihandler.h"

int main()
{
    // Initialize video and GRRLIB
    VIDEO_Init();
    WPAD_Init();
    GRRLIB_Init();
    // GRRLIB_MiisInit();
    // GRRLIB_CreateStaticMiis();

    Mii *miis = loadMiis_Wii();

    // get preferred mode and TV width/height
    GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);
    u16 tvWidth = rmode->viWidth;   // e.g. 640 or ~854 for widescreen
    u16 tvHeight = rmode->viHeight; // usually 480

    // build orthographic projection that uses actual TV width
    Mtx ortho;
    guOrtho(ortho, 0.0f, (float)tvHeight, 0.0f, (float)tvWidth, -300.0f, 300.0f);
    GX_LoadProjectionMtx(ortho, GX_ORTHOGRAPHIC);

    GRRLIB_ttfFont *font = GRRLIB_LoadTTF(NotoSans_SemiBold_ttf, NotoSans_SemiBold_ttf_size);

    int soundmode = CONF_GetSoundMode();

    while (1)
    {
        WPAD_ScanPads(); // read controller input

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
        {
            GRRLIB_Exit(); // clean up GRRLIB
            return 0;
        }

        GX_LoadProjectionMtx(ortho, GX_ORTHOGRAPHIC);

        // Convert integer to string
        char cstr[16]; // enough to hold a number like 12345
        std::sprintf(cstr, "%d", NoOfMiis);

        // Draw text every frame
        GRRLIB_PrintfTTF(0, 0, font, "Miis On system: ", 20, 0xFFFFFFFF);
        GRRLIB_PrintfTTF(160, 0, font, cstr, 20, 0xFFFFFFFF);

        char cstrres[16]; // enough to hold a number like 12345
        std::sprintf(cstrres, "%dx%d", tvWidth, tvHeight);

        GRRLIB_PrintfTTF(0, 20, font, cstrres, 20, 0xFFFFFFFF);

        GRRLIB_PrintfTTF(0, 40, font, soundmode ? soundmode == 2 ? "Surround" : "Stereo" : "Mono", 20, 0xFFFFFFFF);

        for (int i = 0; i < NoOfMiis && i < 100; ++i)
        {
            uint16_t utf16_name[11]; // Mii names are 11 code units
            // Copy the bytes from miis[i].name
            for (int j = 0; j < 11; j++)
                utf16_name[j] = miis[i].name[j]; // directly, no cast

            char utf8_buf[32]; // 11 UTF-16 chars max → 3 bytes each + null
            MiiName_UTF16_to_UTF8(utf16_name, 11, utf8_buf, sizeof(utf8_buf));

            int y = 60 + i * 18;
            GRRLIB_PrintfTTF(0, y, font, utf8_buf, 18, 0xFFFFFFFF);
        }

        GRRLIB_Render(); // render frame to screen
        VIDEO_WaitVSync();
    }

    GRRLIB_Exit();
    return 0;
}
