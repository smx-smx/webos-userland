/*
 * Show remote control / pointer / joystick input on screen.
 *
 * The last key is shown large with its linux/input.h name, below it the most
 * recent events with device type and the raw input_event. The Magic Remote
 * pointer is drawn as a cursor. Volume and mute keys are passed on to the TV.
 *
 * Every key is shown, BACK included, so the program exits when it loses the
 * focus (Home/Exit keys, "nc4app exit"), on LGNC_MSG_TERMINATE or after
 * --seconds N (default: run until closed).
 */
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <lgnc.h>

#include "font.h"
#include "keynames.h"
#include "nc4log.h"

#define WIDTH 1280
#define HEIGHT 720
#define HISTORY 14

enum event_kind { INPUT_KEY, INPUT_MOUSE, INPUT_JOYSTICK };

struct event {
    double time;
    enum event_kind kind;
    unsigned int code;
    LGNC_KEY_COND_T cond;
    int x, y;
    int hasInfo;
    LGNC_ADDITIONAL_INPUT_INFO_T info;
};

static volatile sig_atomic_t running = 1;
static double startTime;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static struct event history[HISTORY];
static unsigned int eventCount;
static struct event lastKey;
static int haveKey;
static int pointerX = -1, pointerY = -1;

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static const char *key_name(unsigned int code)
{
    switch (code) {
    /* LG specific codes, see lgnc_system.h */
    case LGNC_REMOTE_NUM_1: return "LG_NUM_1";
    case LGNC_REMOTE_NUM_2: return "LG_NUM_2";
    case LGNC_REMOTE_NUM_3: return "LG_NUM_3";
    case LGNC_REMOTE_NUM_4: return "LG_NUM_4";
    case LGNC_REMOTE_NUM_5: return "LG_NUM_5";
    case LGNC_REMOTE_NUM_6: return "LG_NUM_6";
    case LGNC_REMOTE_NUM_7: return "LG_NUM_7";
    case LGNC_REMOTE_NUM_8: return "LG_NUM_8";
    case LGNC_REMOTE_NUM_9: return "LG_NUM_9";
    case LGNC_REMOTE_NUM_10: return "LG_NUM_10";
    case LGNC_REMOTE_NUM_11: return "LG_NUM_11";
    case LGNC_REMOTE_NUM_12: return "LG_NUM_12";
    case LGNC_REMOTE_DATA_HOSO: return "LG_DATA_HOSO";
    }
    for (unsigned int i = 0; i < sizeof(key_names) / sizeof(key_names[0]); i++) {
        if (key_names[i].code == code)
            return key_names[i].name;
    }
    return "?";
}

static const char *cond_name(LGNC_KEY_COND_T cond)
{
    switch (cond) {
    case LGNC_KEY_PRESS: return "PRESS";
    case LGNC_KEY_RELEASE: return "RELEASE";
    case LGNC_KEY_REPEAT: return "REPEAT";
    case LGNC_KEY_DRAG: return "DRAG";
    case LGNC_KEY_POWER: return "POWER";
    case LGNC_KEY_GESTURE: return "GESTURE";
    default: return "?";
    }
}

static const char *device_name(LGNC_INPUT_DEV_TYPE_T type)
{
    switch (type) {
    case LGNC_INPUT_TYPE_KEYBOARD: return "keyboard";
    case LGNC_INPUT_TYPE_MOUSE: return "mouse";
    case LGNC_INPUT_TYPE_JOYSTICK: return "joystick";
    case LGNC_INPUT_TYPE_REMOTE: return "remote";
    case LGNC_INPUT_TYPE_VIRTUAL: return "virtual";
    case LGNC_INPUT_LGE: return "lge";
    case LGNC_INPUT_TYPE_NLP: return "nlp";
    case LGNC_INPUT_TYPE_SMARTREMOTE: return "smartremote";
    case LGNC_INPUT_TYPE_XPAD: return "xpad";
    case LGNC_INPUT_TYPE_HIDPAD: return "hidpad";
    case LGNC_INPUT_TYPE_WIDI: return "widi";
    case LGNC_INPUT_TYPE_PS3: return "ps3";
    default: return "?";
    }
}

static void format_event(const struct event *e, char *buf, size_t size)
{
    int n;
    switch (e->kind) {
    case INPUT_KEY:
        n = snprintf(buf, size, "%7.2f key   %-18s %4u %-7s", e->time, key_name(e->code), e->code,
                     cond_name(e->cond));
        break;
    case INPUT_MOUSE:
        n = snprintf(buf, size, "%7.2f mouse %4d,%-4d %-8s %4u %-7s", e->time, e->x, e->y, key_name(e->code),
                     e->code, cond_name(e->cond));
        break;
    default:
        n = snprintf(buf, size, "%7.2f joy  ", e->time);
        break;
    }
    if (e->hasInfo && n > 0 && (size_t) n < size) {
        snprintf(buf + n, size - n, " %s#%d ev=%u/%u/%d", device_name(e->info.deviceType), e->info.deviceID,
                 e->info.event.type, e->info.event.code, e->info.event.value);
    }
}

static void add_event(struct event *e, LGNC_ADDITIONAL_INPUT_INFO_T *info)
{
    char line[160];

    e->time = now() - startTime;
    if (info != NULL) {
        e->hasInfo = 1;
        e->info = *info;
    }
    format_event(e, line, sizeof(line));
    nc4log("%s", line);

    pthread_mutex_lock(&lock);
    history[eventCount % HISTORY] = *e;
    eventCount++;
    if (e->kind == INPUT_KEY) {
        lastKey = *e;
        haveKey = 1;
    }
    pthread_mutex_unlock(&lock);
}

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
    struct event e = {.kind = INPUT_KEY, .code = key, .cond = keyCond};
    add_event(&e, keyInput);
    /* let the TV change the volume */
    if (key == KEY_VOLUMEUP || key == KEY_VOLUMEDOWN || key == KEY_MUTE)
        return LGNC_NOT_HANDLED;
    return LGNC_HANDLED;
}

static unsigned int mouseEventCallback(int posX, int posY, unsigned int key, LGNC_KEY_COND_T keyCond,
                                       LGNC_ADDITIONAL_INPUT_INFO_T *keyInput)
{
    pthread_mutex_lock(&lock);
    pointerX = posX;
    pointerY = posY;
    pthread_mutex_unlock(&lock);
    /* pointer movement is shown as the cursor, not in the history */
    if (keyCond != LGNC_KEY_DRAG) {
        struct event e = {.kind = INPUT_MOUSE, .code = key, .cond = keyCond, .x = posX, .y = posY};
        add_event(&e, keyInput);
    }
    return LGNC_HANDLED;
}

static void joystickEventCallback(LGNC_ADDITIONAL_INPUT_INFO_T *e)
{
    struct event ev = {.kind = INPUT_JOYSTICK};
    add_event(&ev, e);
}

static void onSignal(int sig)
{
    (void) sig;
    running = 0;
}

/* ---- drawing: one shader for both solid rectangles and alpha-textured glyphs ---- */

static const char *vertexShader =
    "attribute vec2 pos;\n"
    "attribute vec2 uv;\n"
    "uniform vec2 screen;\n"
    "varying vec2 vuv;\n"
    "void main() {\n"
    "    vuv = uv;\n"
    "    gl_Position = vec4(pos.x / screen.x * 2.0 - 1.0, 1.0 - pos.y / screen.y * 2.0, 0.0, 1.0);\n"
    "}\n";

static const char *fragmentShader =
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "uniform vec4 color;\n"
    "uniform float solid;\n"
    "varying vec2 vuv;\n"
    "void main() {\n"
    "    float a = solid > 0.5 ? 1.0 : texture2D(tex, vuv).a;\n"
    "    gl_FragColor = vec4(color.rgb, color.a * a);\n"
    "}\n";

static GLuint program;
static GLint locPos, locUv, locScreen, locColor, locSolid;

static GLuint compile(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        nc4log("shader: %s", log);
    }
    return shader;
}

static int init_gl(int width, int height)
{
    program = glCreateProgram();
    glAttachShader(program, compile(GL_VERTEX_SHADER, vertexShader));
    glAttachShader(program, compile(GL_FRAGMENT_SHADER, fragmentShader));
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
        return -1;
    glUseProgram(program);
    locPos = glGetAttribLocation(program, "pos");
    locUv = glGetAttribLocation(program, "uv");
    locScreen = glGetUniformLocation(program, "screen");
    locColor = glGetUniformLocation(program, "color");
    locSolid = glGetUniformLocation(program, "solid");
    glUniform2f(locScreen, (float) width, (float) height);
    glUniform1i(glGetUniformLocation(program, "tex"), 0);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_CELL_W * FONT_COUNT, FONT_CELL_H, 0, GL_ALPHA, GL_UNSIGNED_BYTE,
                 font_atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glEnableVertexAttribArray(locPos);
    glEnableVertexAttribArray(locUv);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return 0;
}

static void draw(const GLfloat *verts, int count, float r, float g, float b, float a, int solid)
{
    glUniform4f(locColor, r, g, b, a);
    glUniform1f(locSolid, solid ? 1.0f : 0.0f);
    glVertexAttribPointer(locPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), verts);
    glVertexAttribPointer(locUv, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), verts + 2);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

static void quad(GLfloat *v, float x0, float y0, float x1, float y1, float u0, float u1)
{
    const GLfloat q[6][4] = {
        {x0, y0, u0, 0}, {x1, y0, u1, 0}, {x0, y1, u0, 1},
        {x1, y0, u1, 0}, {x1, y1, u1, 1}, {x0, y1, u0, 1},
    };
    memcpy(v, q, sizeof(q));
}

static void rect(float x, float y, float w, float h, float r, float g, float b, float a)
{
    GLfloat v[24];
    quad(v, x, y, x + w, y + h, 0, 0);
    draw(v, 6, r, g, b, a, 1);
}

static void text(float x, float y, float scale, const char *s, float r, float g, float b)
{
    static GLfloat v[256 * 24];
    int n = 0;
    for (; *s && n < 256; s++, x += FONT_CELL_W * scale) {
        int c = (unsigned char) *s - FONT_FIRST;
        if (c <= 0 || c >= FONT_COUNT)
            continue;
        quad(v + n * 24, x, y, x + FONT_CELL_W * scale, y + FONT_CELL_H * scale, (float) c / FONT_COUNT,
             (float) (c + 1) / FONT_COUNT);
        n++;
    }
    draw(v, n * 6, r, g, b, 1.0f, 0);
}

static void render(int width, int height)
{
    char line[160];

    glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    rect(0, 0, width, 70, 0.15f, 0.25f, 0.55f, 1.0f);
    text(40, 20, 1.0f, "NC4 input test - press keys on the remote, Home or Exit quits", 1, 1, 1);

    pthread_mutex_lock(&lock);
    struct event key = lastKey;
    int gotKey = haveKey;
    unsigned int count = eventCount;
    struct event recent[HISTORY];
    memcpy(recent, history, sizeof(recent));
    int px = pointerX, py = pointerY;
    pthread_mutex_unlock(&lock);

    if (gotKey) {
        snprintf(line, sizeof(line), "%s", key_name(key.code));
        text(40, 100, 3.0f, line, 1.0f, 0.85f, 0.3f);
        snprintf(line, sizeof(line), "code %u (0x%x)  %s", key.code, key.code, cond_name(key.cond));
        text(40, 190, 1.5f, line, 0.8f, 0.8f, 0.8f);
        if (key.hasInfo) {
            snprintf(line, sizeof(line), "device %s #%d, input_event type %u code %u value %d",
                     device_name(key.info.deviceType), key.info.deviceID, key.info.event.type,
                     key.info.event.code, key.info.event.value);
            text(40, 235, 1.0f, line, 0.6f, 0.6f, 0.6f);
        }
    } else {
        text(40, 100, 3.0f, "waiting for input...", 0.5f, 0.5f, 0.5f);
    }

    snprintf(line, sizeof(line), "%u events", count);
    text(40, 285, 1.0f, line, 0.5f, 0.7f, 1.0f);
    if (px >= 0) {
        snprintf(line, sizeof(line), "pointer %d,%d", px, py);
        text(300, 285, 1.0f, line, 0.5f, 0.7f, 1.0f);
    }

    unsigned int shown = count < HISTORY ? count : HISTORY;
    for (unsigned int i = 0; i < shown; i++) {
        const struct event *e = &recent[(count - 1 - i) % HISTORY];
        format_event(e, line, sizeof(line));
        float shade = 1.0f - 0.05f * i;
        text(40, 320 + i * (FONT_CELL_H + 2), 1.0f, line, shade, shade, shade);
    }

    if (px >= 0) {
        float sx = px * (float) width / WIDTH, sy = py * (float) height / HEIGHT;
        rect(sx - 2, sy - 12, 4, 24, 1, 0.3f, 0.3f, 1);
        rect(sx - 12, sy - 2, 24, 4, 1, 0.3f, 0.3f, 1);
    }
}

int main(int argc, char **argv)
{
    int seconds = 0;
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--seconds") == 0)
            seconds = atoi(argv[i + 1]);
    }

    nc4log_open(argv[0]);
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);
    startTime = now();

    LGNC_CALLBACKS_T callbacks = {
        .msgHandler = msgHandler,
        .keyEventCallback = keyEventCallback,
        .mouseEventCallback = mouseEventCallback,
        .joystickEventCallback = joystickEventCallback,
    };
    int ret = LGNC_SYSTEM_Initialize(argc, argv, &callbacks);
    nc4log("LGNC_SYSTEM_Initialize: %d (auid 0x%llx)", ret, LGNC_SYSTEM_GetAUID());
    if (ret != LGNC_OK)
        return 1;

    int displayId = 0;
    if (LGNC_SYSTEM_GetDisplayId(&displayId) != LGNC_OK || LGNC_SYSTEM_SetResolution(WIDTH, HEIGHT) != LGNC_OK) {
        nc4log("no display, is the app focused?");
        goto fail;
    }

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY || !eglInitialize(dpy, NULL, NULL)) {
        nc4log("eglInitialize failed: 0x%x", eglGetError());
        goto fail;
    }
    static const EGLint configAttribs[] = {
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE,
    };
    EGLConfig config;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(dpy, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
        nc4log("eglChooseConfig failed: 0x%x", eglGetError());
        goto fail;
    }
    EGLSurface surface = eglCreateWindowSurface(dpy, config, (EGLNativeWindowType) displayId, NULL);
    static const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext context = eglCreateContext(dpy, config, EGL_NO_CONTEXT, contextAttribs);
    if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT ||
        !eglMakeCurrent(dpy, surface, surface, context)) {
        nc4log("EGL setup failed: 0x%x", eglGetError());
        goto fail;
    }
    eglSwapInterval(dpy, 1);

    EGLint width = WIDTH, height = HEIGHT;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height);
    if (init_gl(width, height) != 0) {
        nc4log("shader setup failed");
        goto fail;
    }
    LGNC_SYSTEM_RemoveLaunchImage();
    nc4log("ready, %dx%d", width, height);

    while (running && (seconds == 0 || now() - startTime < seconds)) {
        render(width, height);
        eglSwapBuffers(dpy, surface);
    }

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
