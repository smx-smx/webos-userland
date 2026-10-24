/*
 * SDL2 audio on NetCast 4: plays a sine tone through SDL's audio callback.
 *
 * By default it also opens a window, which registers with the app manager
 * (start with "nc4app run sdl-test/sdl-audio-test"), so SDL's nc4 audio driver
 * uses LGNC_DIRECTAUDIO. With --no-video only audio is initialized and the
 * driver falls back to LGNC_MEDIA (run through run.sh).
 *
 * usage: sdl-audio-test [--no-video] [--seconds N] [--freq HZ] [--rate HZ]
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "nc4log.h"

static double toneFreq = 440.0;
static double phase;
static SDL_AudioSpec obtained;
static SDL_atomic_t callbacks;

static void sdl_log(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    (void) userdata;
    (void) category;
    (void) priority;
    nc4log("SDL: %s", message);
}

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    (void) userdata;
    Sint16 *samples = (Sint16 *) stream;
    int frames = len / (int) (sizeof(Sint16) * obtained.channels);
    for (int i = 0; i < frames; i++) {
        Sint16 s = (Sint16) (6000.0 * sin(phase));
        for (int c = 0; c < obtained.channels; c++)
            samples[i * obtained.channels + c] = s;
        phase += 2.0 * M_PI * toneFreq / obtained.freq;
        if (phase > 2.0 * M_PI)
            phase -= 2.0 * M_PI;
    }
    SDL_AtomicAdd(&callbacks, 1);
}

int main(int argc, char **argv)
{
    int seconds = 5, rate = 48000, video = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-video") == 0)
            video = 0;
        else if (i < argc - 1 && strcmp(argv[i], "--seconds") == 0)
            seconds = atoi(argv[++i]);
        else if (i < argc - 1 && strcmp(argv[i], "--freq") == 0)
            toneFreq = atof(argv[++i]);
        else if (i < argc - 1 && strcmp(argv[i], "--rate") == 0)
            rate = atoi(argv[++i]);
    }

    nc4log_open(argv[0]);
    SDL_LogSetOutputFunction(sdl_log, NULL);
    /* the nc4 driver logs which LGNC API it uses */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_AUDIO, SDL_LOG_PRIORITY_DEBUG);

    if (SDL_Init((video ? SDL_INIT_VIDEO : 0) | SDL_INIT_AUDIO) != 0) {
        nc4log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    nc4log("audio driver: %s", SDL_GetCurrentAudioDriver());

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (video) {
        window = SDL_CreateWindow("sdl-audio-test", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN);
        renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
        if (renderer == NULL)
            nc4log("no window: %s", SDL_GetError());
    }

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = rate;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = audio_callback;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (dev == 0) {
        nc4log("SDL_OpenAudioDevice: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    nc4log("opened: %d Hz, %d channels, format 0x%x, %d samples per buffer", obtained.freq, obtained.channels,
           obtained.format, obtained.samples);
    SDL_PauseAudioDevice(dev, 0);

    Uint32 start = SDL_GetTicks();
    int quit = 0;
    while (!quit && SDL_GetTicks() - start < (Uint32) seconds * 1000) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = 1;
        }
        if (renderer) {
            /* green while the tone plays */
            SDL_SetRenderDrawColor(renderer, 20, 120, 60, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        } else {
            SDL_Delay(10);
        }
    }

    double elapsed = (SDL_GetTicks() - start) / 1000.0;
    int count = SDL_AtomicGet(&callbacks);
    nc4log("%d callbacks in %.1fs = %.2fs of audio requested", count, elapsed,
           (double) count * obtained.samples / obtained.freq);

    SDL_CloseAudioDevice(dev);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
