/*
 * Full NetCast 4 application: registers with the app manager, creates an
 * EGL window surface on the LGNC display and animates a GLES2 clear color
 * plus a scissored square. Logs key events and exits on BACK, when it loses
 * the focus, on LGNC_MSG_TERMINATE or after --seconds N (default 15).
 *
 * argv must contain "--auid <hex>"; the app manager passes it.
 */
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <lgnc.h>

#include "nc4log.h"

#define WIDTH 1280
#define HEIGHT 720

static volatile sig_atomic_t running = 1;

static LGNC_STATUS_T msgHandler(LGNC_MSG_TYPE_T msg, unsigned int submsg, char *pData, unsigned short dataSize)
{
    (void) pData;
    nc4log("msgHandler: msg=%d submsg=%u size=%u", msg, submsg, dataSize);
    /* the app manager closes apps (HOA_APP_ExitApp, Home/Exit keys) by taking the focus away */
    if (msg == LGNC_MSG_TERMINATE || msg == LGNC_MSG_FOCUS_OUT)
        running = 0;
    return LGNC_HANDLED;
}

static unsigned int keyEventCallback(unsigned int key, LGNC_KEY_COND_T keyCond, LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    nc4log("key: code=%u cond=%d dev=%d type=%d", key, keyCond, keyInput ? keyInput->deviceID : -1,
           keyInput ? (int) keyInput->deviceType : -1);
    if (key == LGNC_REMOTE_BACK && keyCond == LGNC_KEY_RELEASE)
        running = 0;
    return LGNC_HANDLED;
}

static unsigned int mouseEventCallback(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond,
                                       LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    (void) keyInput;
    if (keyCond != LGNC_KEY_DRAG)
        nc4log("mouse: %d,%d key=%u cond=%d", posX, posY, key, keyCond);
    return LGNC_HANDLED;
}

static void onSignal(int sig)
{
    (void) sig;
    running = 0;
}

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
    int seconds = 15;
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--seconds") == 0)
            seconds = atoi(argv[i + 1]);
    }

    nc4log_open(argv[0]);
    for (int i = 0; i < argc; i++)
        nc4log("argv[%d] = %s", i, argv[i]);
    char cwd[256];
    nc4log("cwd = %s", getcwd(cwd, sizeof(cwd)) ? cwd : "?");
    static const char *const envVars[] = {"LD_PRELOAD", "LD_LIBRARY_PATH", "PWD", "HOME"};
    for (unsigned int i = 0; i < sizeof(envVars) / sizeof(envVars[0]); i++)
        nc4log("%s = %s", envVars[i], getenv(envVars[i]) ? getenv(envVars[i]) : "(unset)");

    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    LGNC_CALLBACKS_T callbacks = {
        .msgHandler = msgHandler,
        .keyEventCallback = keyEventCallback,
        .mouseEventCallback = mouseEventCallback,
        .joystickEventCallback = NULL,
    };
    int ret = LGNC_SYSTEM_Initialize(argc, argv, &callbacks);
    nc4log("LGNC_SYSTEM_Initialize: %d (auid 0x%llx)", ret, LGNC_SYSTEM_GetAUID());
    if (ret != LGNC_OK)
        return 1;

    int displayId = 0;
    ret = LGNC_SYSTEM_GetDisplayId(&displayId);
    nc4log("LGNC_SYSTEM_GetDisplayId: %d -> %d", ret, displayId);
    ret = LGNC_SYSTEM_SetResolution(WIDTH, HEIGHT);
    nc4log("LGNC_SYSTEM_SetResolution(%d, %d): %d", WIDTH, HEIGHT, ret);

    LGNC_CTRL_INFO_T info;
    memset(&info, 0, sizeof(info));
    if (LGNC_SYSTEM_GetSystemInfo(&info) == LGNC_OK)
        nc4log("model %.32s, sw %.32s", info.modelName, info.swVer);

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major = 0, minor = 0;
    if (dpy == EGL_NO_DISPLAY || !eglInitialize(dpy, &major, &minor)) {
        nc4log("eglInitialize failed: 0x%x", eglGetError());
        goto fail;
    }
    nc4log("EGL %d.%d: %s / %s", major, minor, eglQueryString(dpy, EGL_VENDOR), eglQueryString(dpy, EGL_VERSION));

    static const EGLint configAttribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE,
    };
    EGLConfig config;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(dpy, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
        nc4log("eglChooseConfig failed: 0x%x", eglGetError());
        goto fail;
    }

    EGLSurface surface = eglCreateWindowSurface(dpy, config, (EGLNativeWindowType) displayId, NULL);
    if (surface == EGL_NO_SURFACE) {
        nc4log("eglCreateWindowSurface failed: 0x%x", eglGetError());
        goto fail;
    }

    static const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext context = eglCreateContext(dpy, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        nc4log("eglCreateContext failed: 0x%x", eglGetError());
        goto fail;
    }
    eglMakeCurrent(dpy, surface, surface, context);
    eglSwapInterval(dpy, 1);

    EGLint surfaceWidth = 0, surfaceHeight = 0;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &surfaceWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &surfaceHeight);
    nc4log("GL: %s / %s, surface %dx%d", glGetString(GL_RENDERER), glGetString(GL_VERSION), surfaceWidth,
           surfaceHeight);

    LGNC_SYSTEM_RemoveLaunchImage();

    double start = now();
    unsigned int frames = 0;
    while (running && now() - start < seconds) {
        float t = (float) (now() - start);
        float phase = t - (int) t;

        glDisable(GL_SCISSOR_TEST);
        glClearColor(phase, 0.2f, 1.0f - phase, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        int size = surfaceHeight / 4;
        int x = (int) (phase * (surfaceWidth - size));
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, (surfaceHeight - size) / 2, size, size);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        eglSwapBuffers(dpy, surface);
        frames++;
    }
    nc4log("%u frames in %.1fs", frames, now() - start);

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, context);
    eglDestroySurface(dpy, surface);
    eglTerminate(dpy);

    nc4log("LGNC_SYSTEM_Finalize: %d", LGNC_SYSTEM_Finalize());
    return 0;

fail:
    LGNC_SYSTEM_Finalize();
    return 1;
}
