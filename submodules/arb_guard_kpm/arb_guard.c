// arb_guard.c — KernelSU/KPatch-Next KPM
//
// Intercepts qcom_scm_update_rollback_version() exported by qcom-scm.ko
// before TrustZone burns the ARB eFuse (SMC svc=0x01 cmd=0x1E).
//
// Load order: qcom-scm.ko is loaded during first-stage init; this KPM is
// loaded by KernelSU after /data is mounted. The hook targets the already-
// loaded module symbol via KPatch-Next hook_func().
//
// Build: use the OKI kernel's KPatch-Next kptools to patch the Image, then
// load this KPM via a KernelSU module (place the .kpm in /data/adb/kpm/).

#include <kpm_utils.h>

KPM_NAME("arb-guard");
KPM_VERSION("1.0.0");
KPM_DESCRIPTION("Suppresses ARB eFuse writes via qcom_scm_update_rollback_version hook");
KPM_AUTHOR("gbl_root_canoe");
KPM_LICENSE("GPL v2");

static int (*orig_qcom_scm_update_rollback_version)(void) = NULL;

// Replacement: log and return success without calling TrustZone.
static int arb_guard_hook(void)
{
    pr_warn_once("arb_guard: qcom_scm_update_rollback_version suppressed — ARB eFuse protected\n");
    return 0;
}

static long arb_guard_init(const char *args, const char *event, void *reserved)
{
    int rc = hook_func("qcom_scm_update_rollback_version",
                       (void *)arb_guard_hook,
                       (void **)&orig_qcom_scm_update_rollback_version);
    if (rc) {
        pr_err("arb_guard: hook failed (%d) — symbol not found or KPM not supported\n", rc);
        return rc;
    }
    pr_info("arb_guard: hook installed on qcom_scm_update_rollback_version\n");
    return 0;
}

static long arb_guard_control(const char *args, const char *event, void *reserved)
{
    return 0;
}

static long arb_guard_exit(void *reserved)
{
    unhook_func("qcom_scm_update_rollback_version",
                (void *)arb_guard_hook,
                (void **)&orig_qcom_scm_update_rollback_version);
    pr_info("arb_guard: hook removed\n");
    return 0;
}

KPM_INIT(arb_guard_init);
KPM_CONTROL(arb_guard_control);
KPM_EXIT(arb_guard_exit);
