#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "lgnc_openapi_types.h"

enum {
    LGNC_REMOTE_PRESS = 272,
    LGNC_REMOTE_RED = 398,
    LGNC_REMOTE_GREEN = 399,
    LGNC_REMOTE_YELLOW = 400,
    LGNC_REMOTE_BLUE = 401,
    LGNC_REMOTE_CH_UP = 402,
    LGNC_REMOTE_CH_DOWN = 403,
    LGNC_REMOTE_BACK = 412,
    LGNC_REMOTE_NUM_1 = 872,
    LGNC_REMOTE_NUM_2 = 873,
    LGNC_REMOTE_NUM_3 = 874,
    LGNC_REMOTE_NUM_4 = 875,
    LGNC_REMOTE_NUM_5 = 876,
    LGNC_REMOTE_NUM_6 = 877,
    LGNC_REMOTE_NUM_7 = 878,
    LGNC_REMOTE_NUM_8 = 879,
    LGNC_REMOTE_NUM_9 = 880,
    LGNC_REMOTE_NUM_10 = 881,
    LGNC_REMOTE_NUM_11 = 882,
    LGNC_REMOTE_NUM_12 = 883,
    LGNC_REMOTE_DATA_HOSO = 885,
};

enum LGNC_CURSOR_SIZE_T {
    LGNC_CURSOR_SIZE_L = 0,
    LGNC_CURSOR_SIZE_M = 1,
    LGNC_CURSOR_SIZE_S = 2,
    LGNC_CURSOR_SIZE_LASTEST = 3,
    LGNC_CURSOR_SIZE_LAST = 4
};

typedef enum LGNC_CURSOR_SIZE_T LGNC_CURSOR_SIZE_T;

enum LGNC_CURSOR_TYPE_T {
    LGNC_CURSOR_TYPE_A = 0,
    LGNC_CURSOR_TYPE_B = 1,
    LGNC_CURSOR_TYPE_C = 2,
    LGNC_CURSOR_TYPE_D = 3,
    LGNC_CURSOR_TYPE_DRAG = 4,
    LGNC_CURSOR_TYPE_POINT = 5,
    LGNC_CURSOR_TYPE_HOLD = 6,
    LGNC_CURSOR_TYPE_GESTURE_POINT = 7,
    LGNC_CURSOR_TYPE_DISABLE = 8,
    LGNC_CURSOR_TYPE_CUSTOM_A = 9,
    LGNC_CURSOR_TYPE_CUSTOM_B = 10,
    LGNC_CURSOR_TYPE_CUSTOM_C = 11,
    LGNC_CURSOR_TYPE_CUSTOM_D = 12,
    LGNC_CURSOR_TYPE_CUSTOM_E = 13,
    LGNC_CURSOR_TYPE_LASTEST = 14,
    LGNC_CURSOR_TYPE_EXTERNAL = 15,
    LGNC_CURSOR_TYPE_LAST = 16
};

enum LGNC_CURSOR_STATE_T {
    LGNC_CURSOR_STATE_NORMAL = 0,
    LGNC_CURSOR_STATE_PRESS = 1,
    LGNC_CURSOR_STATE_LASTEST = 2,
    LGNC_CURSOR_STATE_LAST = 3,
};

enum LGNC_CURSOR_HOTSPOT_T {
    LGNC_CURSOR_HOTSPOT_LEFTTOP = 0,
    LGNC_CURSOR_HOTSPOT_USERSETTING = 1,
    LGNC_CURSOR_HOTSPOT_LAST = 2,
};


typedef struct LGNC_CTRL_INFO LGNC_CTRL_INFO, *PLGNC_CTRL_INFO;

struct LGNC_CTRL_INFO {
    char projectName[32];
    char modelName[32];
    char hwVer[32];
    char swVer[32];
    char ESN[32];
    char toolTypeName[32];
    char serialNumber[32];
    char modelInch[8];
    char countryGroup[8];
    char szDdrRam[8];
};

typedef struct LGNC_CTRL_INFO LGNC_CTRL_INFO_T;

typedef enum LGNC_CURSOR_HOTSPOT_T LGNC_CURSOR_HOTSPOT_T;

typedef enum LGNC_CURSOR_SIZE_T LGNC_CURSOR_SIZE_T;

typedef enum LGNC_CURSOR_STATE_T LGNC_CURSOR_STATE_T;

typedef enum LGNC_CURSOR_TYPE_T LGNC_CURSOR_TYPE_T;

typedef struct LGNC_CUSTOM_CURSOR {
    char fileInfo[256];
    LGNC_CURSOR_TYPE_T cursorType;
    LGNC_CURSOR_SIZE_T cursorSize;
    LGNC_CURSOR_STATE_T cursorState;
    LGNC_CURSOR_HOTSPOT_T cursorHotSpot;
    unsigned short gapX;
    unsigned short gapY;
} LGNC_CUSTOM_CURSOR_T;

/**
 * @brief Initialize the application
 *
 * On NC4 this registers the process with the TV application manager.
 * argv must contain "--auid <hex>" (the app manager passes it), otherwise
 * LGNC_INVALID_PARAMS is returned. callbacks->msgHandler must not be NULL.
 *
 * The window ID for eglCreateWindowSurface() can then be obtained
 * with LGNC_SYSTEM_GetDisplayId().
 */
int LGNC_SYSTEM_Initialize(int argc, char **argv, LGNC_CALLBACKS_T *callbacks);

int LGNC_SYSTEM_Finalize(void);

int LGNC_SYSTEM_GetDisplayId(int *displayId /* very likely to be wl_egl_window */);

int LGNC_SYSTEM_GetDisplayResolution(int *width, int *height);

/**
 * @brief Get the UI language
 * @param language buffer of at least 4 bytes, receives a NUL-terminated 3-character language code on NC4
 */
int LGNC_SYSTEM_GetLanguage(char *language);

/**
 * @brief Get the per-application writable directory
 * @param directory buffer of at least 4097 bytes
 */
int LGNC_SYSTEM_GetSaveDirectory(char *directory);

int LGNC_SYSTEM_GetSystemInfo(LGNC_CTRL_INFO *info);

int LGNC_SYSTEM_GetVersion(int *major, int *minor, int *patch);

#if LGNC_PLATFORM_NC4
/**
 * @brief Select the cursor shape of the application window
 *
 * Use one of LGNC_CURSOR_TYPE_CUSTOM_A..E to show a cursor registered with
 * LGNC_SYSTEM_SetCustomCursor().
 */
int LGNC_SYSTEM_SetCursorShape(LGNC_CURSOR_TYPE_T type, LGNC_CURSOR_SIZE_T size, LGNC_CURSOR_STATE_T state);

/**
 * @brief Register custom cursor images
 * @param cursors Array of cursors; fileInfo is the path to a PNG image
 * @param count Number of elements in cursors
 */
int LGNC_SYSTEM_SetCustomCursor(LGNC_CUSTOM_CURSOR_T *cursors, unsigned int count);

/**
 * @brief Deprecated on NC4, always returns LGNC_OK. Use a transparent custom cursor instead.
 */
int LGNC_SYSTEM_ShowCursor(int show);

int LGNC_SYSTEM_GetCursorPosition(int *x, int *y);

int LGNC_SYSTEM_SetCursorPosition(int x, int y);

/**
 * @brief Set the resolution of the application window (EGL surface)
 *
 * Must be between 640x360 and 1280x720.
 */
int LGNC_SYSTEM_SetResolution(unsigned int width, unsigned int height);

/**
 * @brief Block audio and/or video output of the TV
 */
int LGNC_SYSTEM_SetAVBlock(int blockAudio, int blockVideo);

int LGNC_SYSTEM_ResetDisplayArea(void);

/**
 * @brief Clear LGLaunchImage.png, shown by LGNC_SYSTEM_Initialize() from the app directory
 */
int LGNC_SYSTEM_RemoveLaunchImage(void);

/**
 * @brief Application unique ID, passed as "--auid <hex>" on the command line
 */
unsigned long long LGNC_SYSTEM_GetAUID(void);
#else
int LGNC_SYSTEM_SetCursorShape(void);

int LGNC_SYSTEM_SetCustomCursor(void);
#endif

#ifdef __cplusplus
}
#endif
