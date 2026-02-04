#include <gccore.h>

int pwrofflag;
int resetflag;

void pwrcallback() {
    pwrofflag = 1;
}

void rstcallback(u32 irq, void* ctx) {
    resetflag = 1;
}

int main(void)
{
    VIDEO_Init();
    SYS_SetPowerCallback(pwrcallback);
    SYS_SetResetCallback(rstcallback);
    while (1)
    {
        VIDEO_WaitVSync();
        if (pwrofflag) {
            SYS_ResetSystem(SYS_POWEROFF,0,0);
        }
        if (resetflag) {
            SYS_ResetSystem(SYS_RETURNTOMENU,0,0);
        }
    }
    
    return 0;
}