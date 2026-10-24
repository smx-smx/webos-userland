/**
 * @ingroup lgncopenapi
 * @file lgnc_stdio.h
 * @brief Keep stdout/stderr working on NetCast 4 (helper, not part of liblgncopenapi)
 *
 * When LGNC_SYSTEM_Initialize() or LGNC_DL_HOA_Initialize() loads the TV's debug
 * framework, it points stdout/stderr to its own terminal (dup2) and hooks printf,
 * puts, fputs, fprintf/vfprintf and fwrite through liblibm.so, which the app
 * manager preloads into every app. Output then never reaches the app's own
 * stdout/stderr (e.g. a log file it was started with).
 *
 * @code
 * int fds[2];
 * lgnc_stdio_save(fds);
 * LGNC_SYSTEM_Initialize(argc, argv, &callbacks);
 * lgnc_stdio_restore(fds);
 * printf("this reaches stdout again\n");
 * @endcode
 *
 * The TV's own debug messages are then written to the app's stdout as well.
 * Needs -ldl.
 */
#pragma once

#include <dlfcn.h>
#include <unistd.h>

#ifndef RTLD_DEFAULT
/* glibc only defines it with _GNU_SOURCE */
#define RTLD_DEFAULT ((void *) 0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Duplicate stdout/stderr, call before the LGNC backend is loaded
 * @param fds receives the copies of file descriptors 1 and 2 (-1 on failure)
 */
static inline void lgnc_stdio_save(int fds[2])
{
    fds[0] = dup(1);
    fds[1] = dup(2);
}

/**
 * @brief Restore stdout/stderr saved by lgnc_stdio_save() and reset liblibm's stdio hooks
 * @param fds the file descriptors from lgnc_stdio_save(), closed afterwards
 */
static inline void lgnc_stdio_restore(int fds[2])
{
    static const char *const functions[] = {"vprintf", "puts", "fputs", "vfprintf", "fwrite"};
    char *(*libm_reset_hook)(const char *name);
    unsigned int i;

    for (i = 0; i < 2; i++) {
        if (fds[i] >= 0) {
            dup2(fds[i], (int) i + 1);
            close(fds[i]);
            fds[i] = -1;
        }
    }
    /* liblibm.so: hooks are installed with libm_setup_hook(name, fn), cleared with libm_reset_hook(name) */
    libm_reset_hook = (char *(*)(const char *)) dlsym(RTLD_DEFAULT, "libm_reset_hook");
    if (libm_reset_hook == NULL)
        return;
    for (i = 0; i < sizeof(functions) / sizeof(functions[0]); i++)
        libm_reset_hook(functions[i]);
}

#ifdef __cplusplus
}
#endif
