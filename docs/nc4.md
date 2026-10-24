# NetCast 4 support

NetCast 4.x TVs (2013-2014, e.g. MT5398 with PowerVR SGX 544MP, glibc 2.12.2)
ship an older *NDK* than webOS: `liblgncopenapi.so.2.01.02` in
`/mnt/lg/res/lglib`. It is a thin wrapper that `dlopen()`s the HOA backend
(`libafopenapi`, `liblginputopenapi`, `liblgncinapi`, `libEGL`, GStreamer 0.10...)
at `LGNC_SYSTEM_Initialize()` / `LGNC_PLUGIN_Initialize()` time.

## Building

```sh
SDK=.../arm-lgtv-linux-gnueabi_sdk-buildroot   # openlgtv/buildroot-nc4 release
cmake --preset nc4 -DCMAKE_TOOLCHAIN_FILE=$SDK/share/buildroot/toolchainfile.cmake \
      -DCMAKE_INSTALL_PREFIX=/path/to/prefix
cmake --build build/nc4 && cmake --install build/nc4
```

The `nc4` preset (`WEBOS_USERLAND_PLATFORM=nc4`) only builds the libraries that
exist on NC4, each exporting exactly the symbols of the real one
(`src/nc4/*.lds`, generated with `tools/gen-lds.sh`):

| Library | NC4 | webOS stub | Notes |
| --- | --- | --- | --- |
| liblgncopenapi.so.2 | 261 | 233 | 54 in common, see below |
| libEGL.so.1 | 34 | 44 | EGL 1.4, subset of the webOS stub |
| libGLESv1_CM.so.1 | 145 | 641 | GLES 1.1 core + few OES extensions |
| libGLESv2.so.2 | 142 | 641 | GLES 2.0 core only, use `eglGetProcAddress` for extensions |

Not available on NC4: wayland-egl, GLES3, NDL_directmedia, ndl-directmedia2,
playerAPIs, AcbAPI, helpers, pf-1.0, dile_*, hal_*, media-resource-calculator.

The installed headers have `LGNC_PLATFORM_NC4` set to 1 in `lgnc_config.h`;
in that mode they only declare functions present on NC4. When using the
headers from the source tree pass `-DLGNC_PLATFORM_NC4=1` (`lgncopenapi.pc`
of the nc4 preset does this).

Programs must link `-ldl -lpthread` themselves: `liblgncopenapi.so.2` uses
`dlopen()` without depending on libdl. Stock apps only work because procmgr
LD_PRELOADs `liblibm.so`, which pulls libdl in.

## liblgncopenapi API surface

Functions, by family (`_dl_*` are the 128 internal HOA function pointers).

| Family | Both | NC4 only | webOS only |
| --- | --- | --- | --- |
| `LGNC_SYSTEM_*` | 21 | `RemoveLaunchImage` | `GetPanelType`, `ShowToastMessage` |
| `LGNC_DIRECTAUDIO_*` | 6 | `OpenSub`, `PlaySub`, `CloseSub` | |
| `LGNC_DIRECTVIDEO_*` | `Open`, `Play`, `Close` | `CheckIsInitialized` | `PlayEx`, `SetCallback`, `_SetDisplayWindow`, `_SetCustomDisplayWindow` |
| `LGNC_GAMEPAD_*` | 5 | | |
| `LGNC_MRCU_*` | 10 | | 14 `_LGNC_MRCU_*` internals |
| `LGNC_PLUGIN_*` | 9 | `GetServiceNameWithAuid`, `SendKeyEventToApp` | `SetAppId`, `PushEvent`, `IsScreensaverEnabled`, `ResetScreenSaverTimer`, `GamepadUnregisterCallback`, `_IsGameMode`, `_IsGamepadSleep` |
| `LGNC_DL_*` | | `LGNC_DL_HOA_Initialize/IsInitialized/Finalize` | `LGNC_DL_Initialize/IsInitialized/Finalize` |
| `LGNC_MEDIA_*` | | 11 (channel sound player) | |
| `LGNC_S3D_*` | | 4 (stereoscopic 3D) | |
| `LGNC_IAP_*`, `LGNC_PUSH_*`, `LGNC_APPDRM_*` | | 10, 2, 7 (in-app purchase, push, NCG DRM file I/O) | |
| `LGNC_UTILS_*`, `LGNC_IN_GAMEPAD_*` | | 19, 4 (internal helpers) | |
| `LGNC_LOG_*` | | `DBGPrint`, `FDBGPrint`, `RAWPrint` (function pointer *variables*) | 8 `_LGNC_LOG_*` |
| `LGNC_egl*` | | | 33, NC4 uses plain EGL |
| `LGNC_LS_*` (Luna bus) | | | 12 |
| `LGNC_BLUETOOTH_*` | | | 11 |
| `_LGNC_CURSOR_*`, `_LGNC_EVENT_*`, `_LGNC_DIRECTINPUT_*`, `_LGNC_TVSERVICE_*`, `_LGNC_UTIL_*` | | | 8, 4, 4, 5, 36 |
| `LGNC_GRAPHIC_*` | | | 5 |
| `JO_*`, `_Get*`/`_Set*` | | | 5, 16 |

The headers do not declare `LGNC_S3D_*`, `LGNC_IAP_*`, `LGNC_PUSH_*`,
`LGNC_APPDRM_*`, `LGNC_MRCU_*`, `LGNC_SYSTEM_Get/SetVolume` and
`LGNC_SYSTEM_Get/SetMute` yet: their prototypes are not confirmed.

## Writing an NC4 app

```c
LGNC_CALLBACKS_T cb = { msgHandler, keyEventCallback, mouseEventCallback, NULL };
LGNC_SYSTEM_Initialize(argc, argv, &cb);   /* needs "--auid <hex>" in argv */
int win;
LGNC_SYSTEM_GetDisplayId(&win);
LGNC_SYSTEM_SetResolution(1280, 720);      /* 640x360 .. 1280x720 */
EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
/* ... eglCreateWindowSurface(dpy, config, (EGLNativeWindowType) win, NULL) ... */
LGNC_SYSTEM_Finalize();
```

Notes:

- `LGNC_SYSTEM_Initialize()` returns `LGNC_INVALID_PARAMS` unless argv contains
  `--auid <hex>` (the app manager passes `--service <name> --auid <hex>`). It
  shows `LGLaunchImage.png` from the app directory; `LGNC_SYSTEM_RemoveLaunchImage()`
  clears it.
- Every `LGNC_*` call `assert()`s that the HOA backend is loaded. Without
  registering as an app, `LGNC_DL_HOA_Initialize()` loads it; `LGNC_MEDIA_*`
  works this way, while window/system queries return `LGNC_ERROR`.
- procmgr LD_PRELOADs `liblibm.so`, which hooks `printf`, `puts`, `fwrite`, etc.
  and swallows their output; log with `write(2)`.
- `LGNC_LOG_DBGPrint` is a function pointer, NULL before initialization.
- The app manager closes apps (`HOA_APP_ExitApp`, Home/Exit keys) by sending
  `LGNC_MSG_FOCUS_OUT`, not `LGNC_MSG_TERMINATE`: exit on focus loss and call
  `LGNC_SYSTEM_Finalize()`, which reports the exit with `HOA_APP_ExitAppEx`.
- Sound: `LGNC_MEDIA_Initialize(callback, &pcmInfo)`; channel 0 plays files,
  channels 1-7 take PCM through `LGNC_MEDIA_PlayBuffer()`, fed on
  `LGNC_CB_MSG_NEED_DATA` callbacks (4 KiB chunks).

See `examples/nc4` for working programs.

## Launching apps

Apps are started by **appctrl** (`lg.service.special.appctrl`), which looks
the AUID up in its app lists and asks **procmgr** to execute
`<binary> <exec_arg> [<ExecuteAppEx param>] --service <name> --auid <hex>`
with the app framework environment (LD_PRELOAD of liblibm/libgal/libmtal,
`LD_LIBRARY_PATH`, cwd = app directory). Downloaded and USB developer apps run as user `game` (uid 500), system apps as `root` with full capabilities. appctrl tracks the app
as live, so when it calls `LGNC_SYSTEM_Initialize()` its
`HOA_APP_RequestToFocusedApp` is granted: the TV switches the OSD to the app
window and keys go to it. When the app exits, focus returns to the previous app.

A program started from a shell with a made-up `--auid` registers fine, but only
gets focus (and full frame rate) if no other app is in the foreground; with My
Apps or Home focused, `LGNC_SYSTEM_GetDisplayId()` fails. Exiting such a process
calls `HOA_APP_ExitAppEx` for an AUID appctrl never launched; once that coincided
with the TV rebooting.

Everything is D-Bus on the session bus (`/tmp/dbus-*`, the address is in
appctrl's environment), with `dbus-send` available on the TV:

| Call | Arguments | Notes |
| --- | --- | --- |
| `appctrl /hoa/app HOA_APP_ExecuteApp` | `s caller, t auid, u, u` | options are ignored |
| `appctrl /hoa/app HOA_APP_ExecuteAppEx` | `s caller, t auid, s param, u, u` | param is appended to the command line |
| `appctrl /hoa/app HOA_APP_ExitApp` | `t auid, u exitcode` | 0 = no focus, 1 = back, 3 = Home, 224 = return |
| `appctrl /hoa/app HOA_APP_GetFocusedAuid` | | `t` |
| `appctrl /hoa/app HOA_APP_GetAppInfo` | `t auid` | `u` result, AUID, type, name, icons... |
| `appctrl /hoa/uc AC_SEND_RequestSystemAppsUpdate` | | reloads the system app list |
| `upctrl /hoa/uc AC_SEND_GetManifestBufferFromFile` | `s path` | decrypts NCG manifests |
| signal `/hoa/proc HOA_PROC_SendEventToProcess` | `u event, u param` | host events to all processes |

### App lists

| List | Location | Manifest |
| --- | --- | --- |
| system | aliases in `/mnt/addon/contents/systemapps.xml`, My Apps visibility in `systemapps_launcher.xml` | `/mnt/system_app/preloadedapps/apps/<alias>/` if present, else `/mnt/addon/contents/system/<alias>/`; plaintext |
| downloaded | `/mnt/appstore/apps/installed/<app_id>/` | NCG-encrypted only |
| USB developer apps | `<usb>/lgapps/installed/<app_id>/`, rescanned on `HOST_EVT_USB_APPSTORE_CHANGED` (61519, param = HOA USB device number) | NCG-encrypted only: upctrl returns an error for plaintext manifests |

The AUID is the `app_id`. Manifest `app_type` 7, 8 and 9 are native programs
(4 too); `file_name` is relative to the app directory unless it starts with
`/mnt/`, `exec_arg` adds arguments.

### Homebrew without NCG

`tools/nc4/nc4app` (installed to `nc4-homebrew/` by `examples/nc4`) registers
the USB folder as a system app:

```sh
/mnt/usb/usb2/Drive1/nc4-homebrew/nc4app setup     # "NC4 Homebrew" appears in My Apps
/mnt/usb/usb2/Drive1/nc4-homebrew/nc4app run lgnc-gles --seconds 30
/mnt/usb/usb2/Drive1/nc4-homebrew/nc4app teardown
```

`setup` writes a plaintext manifest for `nc4-launcher` (AUID 79999), symlinks
`/mnt/system_app/preloadedapps/apps/nc4homebrew` to the folder, bind-mounts
copies of `systemapps.xml`/`systemapps_launcher.xml` with the extra entry from
`/tmp` and calls `AC_SEND_RequestSystemAppsUpdate`. No firmware file is changed
and the bind mounts disappear on reboot (run `setup` again; the symlink is
harmless without them).

- **Manual:** pick "NC4 Homebrew" in My Apps, `nc4-launcher` runs the first line
  of `nc4-homebrew/autostart`.
- **Automated:** `nc4app run <program> [args]` starts the same app with the
  program as `HOA_APP_ExecuteAppEx` parameter and waits for it to exit;
  `nc4app exit 79999` closes it (with SIGTERM/SIGKILL fallbacks for programs
  that ignore focus loss).

Either way the program sees the same command line and environment as a stock app.
Being a system app, it runs as `root` with all capabilities, so a misbehaving
program can affect the whole TV.
