#include <mad.h>
#include <ogc/audio.h>
#include <gccore.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "audioplayback.hpp"
#include "globalstop.h"

static volatile u8 stop = 0;
volatile u32 mp3time = 0;

#define SAMPLES_PER_BUFFER 4096
#define RING_BUFFERS 8

static s16 audio_ring[RING_BUFFERS][SAMPLES_PER_BUFFER * 2]
    __attribute__((aligned(32)));

static volatile int ring_write = 0;
static volatile int ring_read = 0;
static volatile int ring_filled = 0;
static volatile int ring_bytes[RING_BUFFERS];

static volatile int dma_busy = 0;

#define INPUT_BUFFER_SIZE 8192

void dma_callback()
{
    // DMA finished current transfer
    dma_busy = 0;

    // If we have more filled buffers, start the next transfer immediately
    if (ring_filled > 0 && !stop)
    {
        int idx = ring_read;
        dma_busy = 1;
        AUDIO_InitDMA((u32)audio_ring[idx], ring_bytes[idx]);
        AUDIO_StartDMA();

        ring_read = (ring_read + 1) % RING_BUFFERS;
        ring_filled--;
    }
}

static inline s16 mad_fixed_to_s16(mad_fixed_t sample)
{
    // clip and scale from MAD’s 28.4 fixed point
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    return (s16)(sample >> (MAD_F_FRACBITS - 15));
}

struct mad_file
{
    FILE *fp;
    unsigned char buffer[INPUT_BUFFER_SIZE];
};

enum mad_flow input(void *userdata, struct mad_stream *stream)
{
    struct mad_file *mf = static_cast<mad_file *>(userdata);
    size_t remaining, read;

    if (stream->next_frame)
    {
        remaining = stream->bufend - stream->next_frame;
        memmove(mf->buffer, stream->next_frame, remaining);
    }
    else
    {
        remaining = 0;
    }

    read = fread(
        mf->buffer + remaining,
        1,
        INPUT_BUFFER_SIZE - remaining,
        mf->fp);

    if (stop || globalstop)
    {
        printf("Stopped Audio\n");
        return MAD_FLOW_STOP;
    }

    if (read == 0)
        return MAD_FLOW_STOP;

    mad_stream_buffer(stream, mf->buffer, remaining + read);
    return MAD_FLOW_CONTINUE;
}

static int checked_rate = 0;
static int paused = 0;

enum mad_flow output(void *cb_data,
                     const struct mad_header *header,
                     struct mad_pcm *pcm)
{

    mp3time += pcm->length;

    if (!checked_rate)
    {
        if (pcm->samplerate != 48000)
            printf("Warning: MP3 is %d Hz, will play at wrong speed\n",
                   pcm->samplerate);
        checked_rate = 1;
    }

    int nsamples = pcm->length;
    if (nsamples > SAMPLES_PER_BUFFER)
        nsamples = SAMPLES_PER_BUFFER;

    // Wait for space in the ring buffer if full
    while (ring_filled == RING_BUFFERS && !stop)
        usleep(1000);

    if (stop)
        return MAD_FLOW_STOP;

    s16 *buf = audio_ring[ring_write];

    while (globalpause && !stop)
    {
        VIDEO_WaitVSync();                          // wait for next frame
        if (stop) {break;}
        //memset(buf, 0, nsamples * 2 * sizeof(s16)); // fill silence
        if (!paused)
        {
            AUDIO_StopDMA();
            dma_busy = false;
            paused = 1;
        }
        
    }

    paused = 0;

    // Normal decoding
    for (int i = 0; i < nsamples; i++)
    {
        s16 l = mad_fixed_to_s16(pcm->samples[0][i]);
        s16 r = (pcm->channels == 2)
                    ? mad_fixed_to_s16(pcm->samples[1][i])
                    : l;

        buf[2 * i] = l;
        buf[2 * i + 1] = r;
    }

    // record byte length for this buffer (stereo: 2 samples per frame)
    ring_bytes[ring_write] = nsamples * 2 * sizeof(s16);

    // advance writer and increase filled count
    ring_write = (ring_write + 1) % RING_BUFFERS;
    ring_filled++;

    // If DMA is idle, start transfer immediately
    if (!dma_busy && ring_filled > 0 && !stop)
    {
        int idx = ring_read;
        dma_busy = 1;
        AUDIO_InitDMA((u32)audio_ring[idx], ring_bytes[idx]);
        AUDIO_StartDMA();

        ring_read = (ring_read + 1) % RING_BUFFERS;
        ring_filled--;
    }

    return MAD_FLOW_CONTINUE;
}

int audioloadandplay(const char *file)
{
    stop = 0;
    checked_rate = 0;
    mp3time = 0;

    printf("Loading Audio\n");

    // initialise ring indices
    ring_write = ring_read = ring_filled = 0;
    for (int i = 0; i < RING_BUFFERS; ++i)
        ring_bytes[i] = 0;

    struct mad_decoder decoder;
    struct mad_file mf;
    FILE *fp = fopen(file, "rb");
    if (!fp)
    {
        printf("File Could Not Be Opened\n");
        return 1;
    }

    mf.fp = fp;

    mad_decoder_init(
        &decoder,
        &mf,
        input,
        0, /* header */
        0, /* filter */
        output,
        0, /* error */
        0  /* message */
    );

    AUDIO_Init(NULL);
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
    AUDIO_RegisterDMACallback(dma_callback);

    dma_busy = 0;
    mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

    // Wait for any remaining queued buffers to finish playing
    while ((ring_filled > 0 || dma_busy) && !stop)
        usleep(1000);

    AUDIO_StopDMA();
    mad_decoder_finish(&decoder);
    fclose(fp);
    globalstop = 1;
    return 0;
}

void stopaudioplayback()
{
    stop = 1;
    AUDIO_StopDMA();
    // After setting stop, ensure DMA is not left running
    // The dma_callback checks stop and will not start new transfers.
}
