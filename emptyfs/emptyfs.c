/*
 * Created 181110  lynnl
 */

#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <libkern/locks.h>

#if 0
#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#endif

#include "emptyfs.h"
#include "utils.h"
#include "emptyfs_vfsops.h"
#include "emptyfs_vnops.h"

/*
 * this struct describe overall VFS plugin
 *  passed as a parameter to vfs_fsadd to register this file system
 */
static struct vfs_fsentry emptyfs_vfsentry = {
#if 0
    /* vfs operations */
    struct vfsops *vfe_vfsops;
    /* # of vnodeopv_desc being registered (reg, spec, fifo ...) */
    int vfe_vopcnt;
    /* null terminated */
    struct vnodeopv_desc **vfe_opvdescs;
    /* historic filesystem type number */
    int vfe_fstypenum;
    /* filesystem type name */
    char vfe_fsname[MFSNAMELEN];
    /* defines the FS capabilities */
    uint32_t    vfe_flags;
    /* reserved for future use; set this to zero */
    void *vfe_reserv[2];
#endif

    &emptyfs_vfsops,
    ARRAY_SIZE(emptyfs_vnopv_desc_list),
    emptyfs_vnopv_desc_list,
    EMPTYFS_FSTYPENO,
    EMPTYFS_NAME,
    EMPTYFS_VFS_FLAGS,
    {NULL, NULL},
};

/*
 * global locking group  which is used by all types of locks
 */
lck_grp_t *lckgrp = NULL;

/*
 * an opaque vfs handle which will be passed to vfs_fsremove()
 */
static vfstable_t emptyfs_vfstbl_ref = NULL;

kern_return_t emptyfs_start(kmod_info_t *ki, void *d __unused)
{
    kern_return_t e = KERN_SUCCESS;

    char *uuid = util_vma_uuid(ki->address);
    LOG("kext executable uuid %s", uuid);
    util_mfree(uuid);

    lckgrp = lck_grp_alloc_init(LCKGRP_NAME, LCK_GRP_ATTR_NULL);
    if (lckgrp == NULL) goto out_lckgrp;
    LOG_DBG("lock group(%s) allocated", LCKGRP_NAME);

    e = vfs_fsadd(&emptyfs_vfsentry, &emptyfs_vfstbl_ref);
    if (e != 0) {
        LOG_ERR("vfs_fsadd() failure  errno: %d", e);
        goto out_vfsadd;
    }
    LOG_DBG("%s file system registered", emptyfs_vfsentry.vfe_fsname);

    LOG("loaded %s version %s build %s (%s %s)",
        BUNDLEID_S, KEXTVERSION_S, KEXTBUILD_S, __TIMESTAMP__, __TZ__);

out_exit:
    return e;

out_vfsadd:
    lck_grp_free(lckgrp);

out_lckgrp:
    e = KERN_FAILURE;
    goto out_exit;
}

kern_return_t emptyfs_stop(kmod_info_t *ki __unused, void *d __unused)
{
    /* TODO */
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

