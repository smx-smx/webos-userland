/**
 * @ingroup lgncopenapi
 * @file lgnc_log.h
 * @brief Logging through the TV debug framework (NetCast 4 only)
 */
#pragma once

#include "lgnc_config.h"

#if LGNC_PLATFORM_NC4

#ifdef __cplusplus
extern "C" {
#endif

/*
 * These are function pointers, set by LGNC_DL_HOA_Initialize()
 * (i.e. LGNC_SYSTEM_Initialize()). They are NULL before that.
 */
extern int (*LGNC_LOG_DBGPrint)(const char *format, ...);
extern int (*LGNC_LOG_RAWPrint)(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* LGNC_PLATFORM_NC4 */
