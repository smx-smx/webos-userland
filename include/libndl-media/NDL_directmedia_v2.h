/* NDL DirectMedia API 2 (webOS 5+) */

#pragma once

#ifdef __cplusplus
extern "C" {
#else
/* needed for bool in C */
# include <stdbool.h>
#endif

#ifndef NDL_DIRECTMEDIA_API_VERSION
# define NDL_DIRECTMEDIA_API_VERSION 2
#elif (NDL_DIRECTMEDIA_API_VERSION != 2)
# error "this header requires NDL_DIRECTMEDIA_API_VERSION 2"
#endif

#include "NDL_directmedia_common.h"

/**
 * @return 0 on success, -1 on failure
 */
int NDL_DirectAudioGetAvailableBufferSize(int *available);

/**
 * @return 0 on success, -1 on failure
 */
int NDL_DirectAudioGetTotalBufferSize(int *total);

/**
 * Play audio stream
 *
 * @param buffer Data buffer
 * @param size Buffer size
 * @param pts
 * @return 0 on success, -1 on failure
 */
int NDL_DirectAudioPlay(void *buffer, unsigned int size, long long pts);

/**
 * Initialize NDL library
 *
 * @param app_id Application ID
 * @return 0 on success, -1 on failure
 */
int NDL_DirectMediaInit(const char *app_id);

/**
 * Set an external window id for the video output
 *
 * @return 0 on success, -1 on failure
 */
int NDL_DirectMediaSetWindowId(const char *windowid);

/**
 * Check if multi channel audio output works
 *
 * @param isSupported 0 = multi channel is not supported,
 *                    1 = supported, but no connected device supports multi channel PCM,
 *                    2 = the connected device supports multi channel PCM,
 *                        but Digital Sound Out is not pass through,
 *                    3 = current settings support multi channel PCM.
 *                    Only 3 means multi channel PCM will play.
 * @return 0 on success, -1 on failure
 */
int NDL_DirectAudioSupportMultiChannel(int *isSupported);

/**
 * Register a callback that reports multi channel PCM support changes
 *
 * @param cb Sound settings callback
 * @return 0 on success, -1 on failure
 */
int NDL_DirectAudioRegisterCallback(NDLMultiChannelPCMCallback cb);

/**
 * @return 0 on success, -1 on failure
 */
int NDL_DirectEffectGetAvailableBufferSize(unsigned int *avail);

/**
 * @return 0 on success, -1 on failure
 */
int NDL_DirectEffectLoad(NDL_DIRECTAUDIO_PCM_INFO_T *info, unsigned int *preferredSize);

/**
 * Plays PCM audio using ALSA
 *
 * @param buffer Data buffer
 * @param size Buffer size
 * @return 0 on success, -1 on failure
 */
int NDL_DirectEffectPlay(void *buffer, unsigned int size);

/**
 * @return 0 on success, -1 on failure
 */
int NDL_DirectEffectUnload(void);

/**
 *
 * @param info
 * @param callback
 * @return 0 on success, -1 on failure
 */
int NDL_DirectMediaLoad(NDL_DIRECTMEDIA_DATA_INFO_T *data, NDLMediaLoadCallback callback);

/**
 * Unloads SMP and cleans up DA/DV/DE.
 *
 * @return 0 on success, -1 on failure
 */
int NDL_DirectMediaUnload(void);

/**
 * Unload NDL DirectMedia implementation library
 */
void NDL_DirectMedia_DL_Finalize(void);

/**
 * Load NDL DirectMedia implementation library
 *
 * @return true on success, false on failure
 */
bool NDL_DirectMedia_DL_Initialize(void);

/**
 * Load NDL DirectMedia implementation library
 *
 * @return Whether impl library has been loaded
 */
bool NDL_DirectMedia_DL_IsInitialized(void);

/**
 * @return 0 on success, -1 on failure
 */
int NDL_DirectVideoFlushRenderBuffer(void);

/**
 * Get the number of frames waiting in the render queue
 *
 * @param length Frame count, not bytes
 * @return 0 on success, -1 on failure
 */
int NDL_DirectVideoGetRenderBufferLength(int *length);

/**
 * Play video stream
 *
 * @param buffer Data buffer
 * @param size Buffer size
 * @param pts
 * @return 0 on success, -1 on failiure
 */
int NDL_DirectVideoPlay(void *buffer, unsigned int size, long long pts);

/**
 * Set the render queue length at which the pipeline starts to drop frames
 *
 * @param threshold Number of frames in the render queue
 * @return 0 on success, -1 on failure
 */
int NDL_DirectVideoSetFrameDropThreshold(int threshold);

/**
 * Set HDR static metadata. Call it after NDL_DirectMediaLoad.
 *
 * @param hdrInfo Mastering display metadata plus the H.265 VUI color codes
 * @return 0 on success, -1 on failure
 */
int NDL_DirectVideoSetHDRInfo(NDL_DIRECTVIDEO_HDR_INFO_T hdrInfo);



#ifdef __cplusplus
}
#endif
