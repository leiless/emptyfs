/*
 * Created 181110  lynnl
 */

#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <libkern/locks.h>

#include "emptyfs.h"
#include "utils.h"

static lck_grp_t *lckgrp = NULL;

kern_return_t emptyfs_start(kmod_info_t *ki, void *d __unused)
{
    kern_return_t e = KERN_SUCCESS;

    char *uuid = util_vma_uuid(ki->address);
    LOG("uuid: %s", uuid);
    util_mfree(uuid);

    lckgrp = lck_grp_alloc_init(LCKGRP_NAME, LCK_GRP_ATTR_NULL);
    if (lckgrp == NULL) goto out_lckgrp;
    LOG_DBG("lock group(%s) allocated", LCKGRP_NAME);

out_exit:
    return KERN_SUCCESS;

out_lckgrp:
    e = KERN_FAILURE;
    goto out_exit;
}

kern_return_t emptyfs_stop(kmod_info_t *ki __unused, void *d __unused)
{

    return KERN_SUCCESS;
}

#ifdef __kext_makefile__
extern kern_return_t _start(kmod_info_t *, void *);
extern kern_return_t _stop(kmod_info_t *, void *);

/* Will expand name if it's a macro */
#define KMOD_EXPLICIT_DECL2(name, ver, start, stop) \
    __attribute__((visibility("default")))          \
        KMOD_EXPLICIT_DECL(name, ver, start, stop)

KMOD_EXPLICIT_DECL2(BUNDLEID, KEXTBUILD_S, _start, _stop)

/* If you intended to write a kext library  NULLify _realmain and _antimain */
__private_extern__ kmod_start_func_t *_realmain = emptyfs_start;
__private_extern__ kmod_stop_func_t *_antimain = emptyfs_stop;

__private_extern__ int _kext_apple_cc = __APPLE_CC__;
#endif

