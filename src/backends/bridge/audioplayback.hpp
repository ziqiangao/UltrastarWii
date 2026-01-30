#if !defined(AUDIO_B)
#define AUDIO_B

#include <stdint.h>
extern int globalpause;
extern int globalstop;

extern volatile uint32_t mp3time;
int audioloadandplay(const char* file);
void stopaudioplayback();

#endif // AUDIO_B
