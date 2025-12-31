#include "GRRLIB.h"

int main() {
    GRRLIB_Init();                    // Initialize video and GRRLIB
    GRRLIB_SetBackgroundColour(0,0,0,255);

    while (1) {
        WPAD_ScanPads();              // Read Wii Remote
        GRRLIB_DrawImg(100, 100, texture, 0, 1, 1, 255); // Draw an image
        GRRLIB_Render();              // Swap buffers

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) break;
    }

    GRRLIB_Exit();                     // Cleanup
    return 0;
}
