/**
 * @ingroup lgncopenapi
 * @file lgnc_plugin.h
 * @brief APIs for plugin-typed application
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lgnc_openapi_types.h"

/**
 * @brief Initializes as plugin
 * @param callbacks Event callbacks, must not be NULL.
 * @return 0 if succeeded
 */
int LGNC_PLUGIN_Initialize(LGNC_CALLBACKS_T *callbacks);

#if LGNC_PLATFORM_NC4
/**
 * @brief Set the area of the screen used by the plugin
 * @note NetCast 4 only; LGNC_PLUGIN_Initialize() sets 0, 0, 1280, 720
 */
int LGNC_PLUGIN_SetDisplayArea(int x, int y, int w, int h);
#else
/**
 * @brief Set application ID
 *
 * Must be called AFTER LGNC_PLUGIN_Initialize.
 *
 * @param appId Application ID, e.g. "com.example.application"
 * @return 0 if succeeded
 *
 */
int LGNC_PLUGIN_SetAppId(const char *appId);
#endif

int LGNC_PLUGIN_Finalize();

#ifdef __cplusplus
}
#endif