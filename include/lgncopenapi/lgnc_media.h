/**
 * @ingroup lgncopenapi
 * @file lgnc_media.h
 * @brief Channel-based sound playback (NetCast 4 only)
 *
 * Channel 0 plays files/streams (GStreamer playbin), channels 1-7 play raw PCM
 * buffers with the format given to LGNC_MEDIA_Initialize().
 */
#pragma once

#include "lgnc_config.h"

#if LGNC_PLATFORM_NC4

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LGNC_CHANNEL {
    LGNC_CHANNEL_FILE = 0,
    LGNC_CHANNEL_PCM_1 = 1,
    LGNC_CHANNEL_PCM_2 = 2,
    LGNC_CHANNEL_PCM_3 = 3,
    LGNC_CHANNEL_PCM_4 = 4,
    LGNC_CHANNEL_PCM_5 = 5,
    LGNC_CHANNEL_PCM_6 = 6,
    LGNC_CHANNEL_PCM_7 = 7,
} LGNC_CHANNEL;

/* Names are not official, values are */
typedef enum LGNC_CB_MSG {
    /** Playback finished (end of stream / buffer consumed) */
    LGNC_CB_MSG_END = 1,
    /** Playback started */
    LGNC_CB_MSG_PLAY_START = 2,
    /** PCM channel needs more data */
    LGNC_CB_MSG_NEED_DATA = 3,
    LGNC_CB_MSG_ERROR = 0xF000,
} LGNC_CB_MSG;

typedef void (*LGNC_MEDIA_CALLBACK_T)(LGNC_CHANNEL channel, LGNC_CB_MSG msg);

typedef enum LGNC_MEDIA_PCM_RATE_T {
    LGNC_MEDIA_PCM_RATE_48000 = 1,
    LGNC_MEDIA_PCM_RATE_44100 = 2,
    LGNC_MEDIA_PCM_RATE_32000 = 3,
    LGNC_MEDIA_PCM_RATE_24000 = 4,
    LGNC_MEDIA_PCM_RATE_16000 = 5,
    LGNC_MEDIA_PCM_RATE_12000 = 6,
    LGNC_MEDIA_PCM_RATE_8000 = 7,
} LGNC_MEDIA_PCM_RATE_T;

/* Field names are not official, layout is */
typedef struct LGNC_MEDIA_PCM_INFO_T {
    /** 1 for signed 16-bit little endian, anything else for 8-bit */
    int format;
    /** Sample rate, 0 means 44100 */
    LGNC_MEDIA_PCM_RATE_T rate;
    /** 0 for mono, non-zero for stereo */
    int stereo;
    /** appsrc buffering threshold in percent (0-100) */
    unsigned int minPercent;
} LGNC_MEDIA_PCM_INFO_T;

/**
 * @brief Initialize the media channels
 * @param callback Called from the GLib main loop thread on channel events
 * @param pcmInfo Format of PCM channels 1-7, may be NULL to only use channel 0
 */
int LGNC_MEDIA_Initialize(LGNC_MEDIA_CALLBACK_T callback, LGNC_MEDIA_PCM_INFO_T *pcmInfo);

int LGNC_MEDIA_Finalize(void);

/**
 * @brief Queue PCM data on a channel (1-7)
 */
int LGNC_MEDIA_PlayBuffer(LGNC_CHANNEL channel, const void *data, unsigned int size);

/**
 * @brief Play a file (path or URI) on a channel
 * @param repeat Stored in the channel; pass 0
 */
int LGNC_MEDIA_PlayFile(LGNC_CHANNEL channel, const char *path, int repeat);

int LGNC_MEDIA_Pause(LGNC_CHANNEL channel);

int LGNC_MEDIA_Resume(LGNC_CHANNEL channel);

int LGNC_MEDIA_Stop(LGNC_CHANNEL channel);

/** @param position Position in nanoseconds */
int LGNC_MEDIA_Seek(LGNC_CHANNEL channel, long long position);

/** @param position Receives the position in nanoseconds */
int LGNC_MEDIA_GetCurrentPosition(LGNC_CHANNEL channel, long long *position);

/** @param duration Receives the duration in nanoseconds */
int LGNC_MEDIA_GetDuration(LGNC_CHANNEL channel, long long *duration);

#ifdef __cplusplus
}
#endif

#endif /* LGNC_PLATFORM_NC4 */
