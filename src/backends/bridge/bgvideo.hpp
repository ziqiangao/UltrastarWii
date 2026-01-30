#if !defined(BGVIDEO_B)
#define BGVIDEO_B

int videoloadandplay(const char* filename);
void stopvideoplayback();
void VIDPlayer_Init();
extern void (*main_thread_vsync)(void);
void play_video(const char* filename);

#endif // BGVIDEO_B
