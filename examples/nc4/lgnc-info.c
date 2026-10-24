/*
 * Print system information through liblgncopenapi, without registering
 * as an application (only the HOA backend is loaded).
 */
#include <string.h>

#include <lgnc.h>
#include <lgnc_dl.h>

#include "nc4log.h"

int main(int argc, char **argv)
{
    int ret;

    (void) argc;
    nc4log_open(argv[0]);

    if (!LGNC_DL_HOA_Initialize()) {
        nc4log("LGNC_DL_HOA_Initialize failed");
        return 1;
    }

    int major = 0, minor = 0, patch = 0;
    ret = LGNC_SYSTEM_GetVersion(&major, &minor, &patch);
    nc4log("GetVersion: %d -> %d.%d.%d", ret, major, minor, patch);

    int width = 0, height = 0;
    ret = LGNC_SYSTEM_GetDisplayResolution(&width, &height);
    nc4log("GetDisplayResolution: %d -> %dx%d", ret, width, height);

    char language[8] = {0};
    ret = LGNC_SYSTEM_GetLanguage(language);
    nc4log("GetLanguage: %d -> '%s'", ret, language);

    char saveDir[4097] = {0};
    ret = LGNC_SYSTEM_GetSaveDirectory(saveDir);
    nc4log("GetSaveDirectory: %d -> '%s'", ret, saveDir);

    LGNC_CTRL_INFO_T info;
    memset(&info, 0, sizeof(info));
    ret = LGNC_SYSTEM_GetSystemInfo(&info);
    nc4log("GetSystemInfo: %d", ret);
    if (ret == LGNC_OK) {
        nc4log("  projectName  = %.32s", info.projectName);
        nc4log("  modelName    = %.32s", info.modelName);
        nc4log("  hwVer        = %.32s", info.hwVer);
        nc4log("  swVer        = %.32s", info.swVer);
        nc4log("  toolTypeName = %.32s", info.toolTypeName);
        nc4log("  modelInch    = %.8s", info.modelInch);
        nc4log("  countryGroup = %.8s", info.countryGroup);
        nc4log("  szDdrRam     = %.8s", info.szDdrRam);
    }

    LGNC_DL_HOA_Finalize();
    return 0;
}
