#include <mad.h>
#include <ogc/audio.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "audioplayback.hpp"

u8 stop = 0;

#define SAMPLES_PER_BUFFER 4096

static s16 audio_buffers[2][SAMPLES_PER_BUFFER * 2]
    __attribute__((aligned(32)));

static volatile int current_buffer = 0;
static volatile int dma_busy = 0;

#define INPUT_BUFFER_SIZE 8192

void dma_callback()
{
    dma_busy = 0;
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


    if (stop)
        return MAD_FLOW_STOP;

    if (read == 0)
        return MAD_FLOW_STOP;

    mad_stream_buffer(stream, mf->buffer, remaining + read);
    return MAD_FLOW_CONTINUE;
}

static int checked_rate = 0;

enum mad_flow output(void *cb_data,
                     const struct mad_header *header,
                     struct mad_pcm *pcm)
{

    if (!checked_rate)
    {
        if (pcm->samplerate != 48000)
            printf("Warning: MP3 is %d Hz, will play at wrong speed\n",
                   pcm->samplerate);
        checked_rate = 1;
    }

    s16 *buffer = audio_buffers[current_buffer];
    int nsamples = pcm->length;

    if (nsamples > SAMPLES_PER_BUFFER)
        nsamples = SAMPLES_PER_BUFFER;

    for (int i = 0; i < nsamples; i++)
    {
        s16 l = mad_fixed_to_s16(pcm->samples[0][i]);
        s16 r = (pcm->channels == 2)
                    ? mad_fixed_to_s16(pcm->samples[1][i])
                    : l;

        buffer[2 * i] = l;
        buffer[2 * i + 1] = r;
    }

    while (dma_busy)
        usleep(10);

    dma_busy = 1;

    AUDIO_InitDMA((u32)buffer, nsamples * 2 * sizeof(s16));
    AUDIO_StartDMA();

    current_buffer ^= 1;

    return MAD_FLOW_CONTINUE;
}

int audioloadandplay(const char* file)
{
    stop = 0;
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
    AUDIO_StopDMA();
    mad_decoder_finish(&decoder);
    fclose(fp);
    return 0;
}

void stopaudioplayback() {
    stop = 1;
}