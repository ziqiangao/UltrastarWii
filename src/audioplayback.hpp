#if !defined(AUDIOPLAYBACK)
#define AUDIOPLAYBACK

extern volatile u32 mp3time;
int audioloadandplay(const char* file);
void stopaudioplayback();

#endif // AUDIOPLAYBACK