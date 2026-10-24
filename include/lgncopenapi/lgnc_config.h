/**
 * @ingroup lgncopenapi
 * @file lgnc_config.h
 * @brief Selects which LGNC API surface the headers declare
 *
 * Define LGNC_PLATFORM_NC4 to 1 to target NetCast 4.x (liblgncopenapi.so.2.01.02),
 * which only declares functions that exist on NC4. Headers installed by the
 * nc4 CMake preset have this set already.
 */
#pragma once

#ifndef LGNC_PLATFORM_NC4
#define LGNC_PLATFORM_NC4 0
#endif
