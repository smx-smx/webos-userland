/*
 * SDL2 on NetCast 4: needs SDL built with the nc4 video driver, which registers
 * with the app manager (start through "nc4app run sdl-test/sdl-test") and turns
 * remote keys and the pointer into SDL events.
 *
 * Draws a moving square with the GLES 2 renderer, logs key and mouse events and
 * quits on SDL_QUIT (sent when the app loses focus) or after --seconds N
 * (default 30).
 */
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "nc4log.h"

static void sdl_log(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    (void) userdata;
    (void) category;
    (void) priority;
    nc4log("SDL: %s", message);
}

int main(int argc, char **argv)
{
    int seconds = 30;
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--seconds") == 0)
            seconds = atoi(argv[i + 1]);
    }

    nc4log_open(argv[0]);
    SDL_LogSetOutputFunction(sdl_log, NULL);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        nc4log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    nc4log("video driver: %s", SDL_GetCurrentVideoDriver());

    SDL_Window *window = SDL_CreateWindow("sdl-test", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN);
    if (window == NULL) {
        nc4log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        nc4log("SDL_CreateRenderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    nc4log("renderer: %s", info.name);

    Uint32 start = SDL_GetTicks();
    unsigned int frames = 0;
    int quit = 0;
    while (!quit && SDL_GetTicks() - start < (Uint32) seconds * 1000) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                nc4log("SDL_QUIT");
                quit = 1;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                nc4log("key %s: %s%s", e.type == SDL_KEYDOWN ? "down" : "up",
                       SDL_GetScancodeName(e.key.keysym.scancode), e.key.repeat ? " (repeat)" : "");
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                nc4log("mouse button %d %s at %d,%d", e.button.button,
                       e.type == SDL_MOUSEBUTTONDOWN ? "down" : "up", e.button.x, e.button.y);
                break;
            default:
                break;
            }
        }

        int x = (int) ((SDL_GetTicks() - start) / 4 % (1280 - 120));
        SDL_SetRenderDrawColor(renderer, 20, 40, 90, 255);
        SDL_RenderClear(renderer);
        SDL_Rect square = {x, 300, 120, 120};
        SDL_SetRenderDrawColor(renderer, 255, 200, 40, 255);
        SDL_RenderFillRect(renderer, &square);
        SDL_RenderPresent(renderer);
        frames++;
    }
    nc4log("%u frames in %.1fs", frames, (SDL_GetTicks() - start) / 1000.0);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
