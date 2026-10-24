/*
 * Play a sine tone on LGNC_MEDIA PCM channel 1, feeding 4 KiB chunks from
 * the media callback.
 *
 * Only the HOA backend is loaded (LGNC_DL_HOA_Initialize), the process is
 * not registered with the app manager, so no --auid is needed.
 *
 * usage: lgnc-tone [--seconds N] [--freq HZ]
 */
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lgnc.h>
#include <lgnc_dl.h>

#include "nc4log.h"

#define RATE 48000
#define CHUNK 4096

static volatile sig_atomic_t running = 1;

static int16_t *pcm;
static unsigned int pcmSize;
static unsigned int pcmOffset;
static pthread_mutex_t feedLock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int chunksFed, callbacks[4];

static int feed(void)
{
    pthread_mutex_lock(&feedLock);
    unsigned int len = CHUNK;
    if (pcmOffset + len > pcmSize)
        len = pcmSize - pcmOffset;
    int ret = LGNC_MEDIA_PlayBuffer(LGNC_CHANNEL_PCM_1, (const char *) pcm + pcmOffset, len);
    if (ret == LGNC_OK) {
        pcmOffset = (pcmOffset + len) % pcmSize;
        chunksFed++;
    }
    pthread_mutex_unlock(&feedLock);
    return ret;
}

static void mediaCallback(LGNC_CHANNEL channel, LGNC_CB_MSG msg)
{
    if (msg == LGNC_CB_MSG_END || msg == LGNC_CB_MSG_NEED_DATA) {
        callbacks[msg]++;
        if (channel == LGNC_CHANNEL_PCM_1 && running)
            feed();
    } else {
        if (msg == LGNC_CB_MSG_PLAY_START)
            callbacks[msg]++;
        nc4log("media: channel=%d msg=0x%x", channel, msg);
    }
}

static void onSignal(int sig)
{
    (void) sig;
    running = 0;
}

int main(int argc, char **argv)
{
    int seconds = 5;
    double freq = 440.0;
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--seconds") == 0)
            seconds = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "--freq") == 0)
            freq = atof(argv[i + 1]);
    }

    nc4log_open(argv[0]);
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    if (!LGNC_DL_HOA_Initialize()) {
        nc4log("LGNC_DL_HOA_Initialize failed");
        return 1;
    }
    int ret;

    /* one second of 16-bit stereo, loops seamlessly for integer frequencies */
    pcmSize = RATE * 2 * sizeof(int16_t);
    pcm = malloc(pcmSize);
    for (int i = 0; i < RATE; i++) {
        int16_t s = (int16_t) (8000.0 * sin(2.0 * M_PI * freq * i / RATE));
        pcm[i * 2] = s;
        pcm[i * 2 + 1] = s;
    }

    LGNC_MEDIA_PCM_INFO_T pcmInfo = {
        .format = 1,
        .rate = LGNC_MEDIA_PCM_RATE_48000,
        .stereo = 1,
        .minPercent = 20,
    };
    ret = LGNC_MEDIA_Initialize(mediaCallback, &pcmInfo);
    nc4log("LGNC_MEDIA_Initialize: %d", ret);
    if (ret != LGNC_OK)
        goto out;

    nc4log("playing %.0f Hz for %d s", freq, seconds);
    ret = feed();
    nc4log("first LGNC_MEDIA_PlayBuffer: %d", ret);

    time_t end = time(NULL) + seconds;
    while (running && time(NULL) < end) {
        struct timespec ts = {0, 100 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    running = 0;

    nc4log("fed %u chunks (%.1f s of audio); callbacks: end=%u start=%u need=%u", chunksFed,
           chunksFed * (double) CHUNK / (RATE * 4), callbacks[LGNC_CB_MSG_END], callbacks[LGNC_CB_MSG_PLAY_START],
           callbacks[LGNC_CB_MSG_NEED_DATA]);
    nc4log("LGNC_MEDIA_Stop: %d", LGNC_MEDIA_Stop(LGNC_CHANNEL_PCM_1));
    nc4log("LGNC_MEDIA_Finalize: %d", LGNC_MEDIA_Finalize());

out:
    LGNC_DL_HOA_Finalize();
    free(pcm);
    return ret == LGNC_OK ? 0 : 1;
}
