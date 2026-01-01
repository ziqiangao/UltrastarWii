#include <grrlib.h>
#include <fat.h>
#include <wiikeyboard/usbkeyboard.h>
#include <wiiuse/wpad.h>
#include <gccore.h>

int main()
{
    // Initialize video and GRRLIB
    VIDEO_Init();
    WPAD_Init();
    GRRLIB_Init();

    while (1)
    {
        WPAD_ScanPads();

        // Set background colour based on any button pressed
        if (WPAD_ButtonsHeld(0))
        {
            GRRLIB_SetBackgroundColour(10, 10, 10, 255); // white
        }
        else
        {
            GRRLIB_SetBackgroundColour(0, 0, 0, 255); // black
        }

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
        {                  // HOME button
            GRRLIB_Exit(); // clean up GRRLIB
            return 0;      // exit main loop
        }

        GRRLIB_Render(); // update the screen
        VIDEO_WaitVSync();
    }

    GRRLIB_Exit(); // cleanup (won’t be reached in infinite loop)
    return 0;
}
