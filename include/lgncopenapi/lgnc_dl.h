#pragma once

#include "lgnc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LGNC_PLATFORM_NC4
/**
 * @brief Load the HOA backend libraries (libafopenapi, liblgncinapi, libEGL...)
 *
 * Called by LGNC_SYSTEM_Initialize() and LGNC_PLUGIN_Initialize().
 * @return 1 if succeeded, 0 otherwise
 * @note NetCast 4 only
 */
int LGNC_DL_HOA_Initialize(void);

int LGNC_DL_HOA_IsInitialized(void);

void LGNC_DL_HOA_Finalize(void);
#else
int LGNC_DL_Initialize();

int LGNC_DL_IsInitialized();

void LGNC_DL_Finalize();
#endif

#ifdef __cplusplus
}
#endif
