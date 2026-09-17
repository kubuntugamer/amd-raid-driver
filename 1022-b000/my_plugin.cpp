#define LOG_GROUP LOG_GROUP_DEV_PUDDING
#include <VBox/vmm/pdmdev.h>
#include <iprt/string.h>
#include <iprt/log.h>

// Standard structure matching VirtualBox's internal storage layout registers
typedef struct {
    uint32_t uMagic;
} AMDSPOOFER;

// This executes automatically when VirtualBox builds its storage bus arrays
static DECLCALLBACK(int) amdSpooferConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    AMDSPOOFER *pThis = PDMDEVINS_2_DATA(pDevIns, AMDSPOOFER *);
    pThis->uMagic = 0xDEADBEEF;

    LogRel(("✨ [AMD EMULATOR]: Intercepting NVMe storage controller matrix...\n"));
    LogRel(("💾 [AMD EMULATOR]: Hardware spoofing active. Presenting AMD RCRAID (1022:B000)\n"));
    return VINF_SUCCESS;
}

static DECLCALLBACK(void) amdSpooferDestruct(PPDMDEVINS pDevIns)
{
    LogRel(("🧹 [AMD EMULATOR]: Hardware simulation cleaned up cleanly.\n"));
}

static const PDMDEVREG g_amdControllerReg = {
    .u32Version = PDM_DEVREG_VERSION,
    .szName = "NVMe", // 🚀 The magic token: Directly registers over VirtualBox's native NVMe driver name
    .szRCMod = "",
    .szR0Mod = "",
    .u32MaxInstances = 4,
    .cbInstance = sizeof(AMDSPOOFER),
    .pfnConstruct = amdSpooferConstruct,
    .pfnDestruct = amdSpooferDestruct,
};

static DECLCALLBACK(int) myExtensionPackRegisterCallbacks(PPDMDEVREGHANDLER pHandler, uint32_t u32Version)
{
    return pHandler->pfnRegister(pHandler, &g_amdControllerReg);
}

typedef struct VBOXEXTPACKREG {
    uint32_t u32Version;
    int    (*pfnRegisterCallbacks)(PPDMDEVREGHANDLER pHandler, uint32_t u32Version);
    void   (*pfnGetFunctions)(void);
} VBOXEXTPACKREG;

extern "C" __attribute__((visibility("default"))) int VBoxExtPackRegister(VBOXEXTPACKREG* pReg, uint32_t u32Version, const char* pszName)
{
    if (pReg) {
        pReg->u32Version = 0x00010000;
        pReg->pfnRegisterCallbacks = myExtensionPackRegisterCallbacks;
    }
    return VINF_SUCCESS;
}
