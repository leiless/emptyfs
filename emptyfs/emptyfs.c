/*
 * Created 181110  lynnl
 */

#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <libkern/locks.h>

#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/fcntl.h>

#include "emptyfs.h"
#include "utils.h"

/*
 * global locking group  which is used by all types of locks
 */
static lck_grp_t *lckgrp = NULL;

/*
 * this variable will be set when we register VFS plugin via vfs_fsadd()
 *  it holds a pointer to the array of vnop functions for this VFS plugin
 * we declare it early :. it's referenced by the code that creates the vnodes
 */
static int (**emptyfs_vnop_p)(void *);

#define VNOP_FUNC   int (*)(void *)

/*
 * describes all vnode operations supported by vnodes created by our VFS plugin
 */
static struct vnodeopv_entry_desc emptyfs_vnopv_entry_desc_list[] = {
#if 0
    /* which operation this is */
    struct vnodeop_desc *opve_op;
    /* code implementing this operation */
    int (*opve_impl)(void *);

    struct vnodeop_desc vnop_default_desc;
    struct vnodeop_desc vnop_lookup_desc;
    struct vnodeop_desc vnop_create_desc;
    struct vnodeop_desc vnop_whiteout_desc; /* obsolete */
    struct vnodeop_desc vnop_mknod_desc;
    struct vnodeop_desc vnop_open_desc;
    struct vnodeop_desc vnop_close_desc;
    struct vnodeop_desc vnop_access_desc;
    struct vnodeop_desc vnop_getattr_desc;
    struct vnodeop_desc vnop_setattr_desc;
    struct vnodeop_desc vnop_read_desc;
    struct vnodeop_desc vnop_write_desc;
    struct vnodeop_desc vnop_ioctl_desc;
    struct vnodeop_desc vnop_select_desc;
    struct vnodeop_desc vnop_exchange_desc;
    struct vnodeop_desc vnop_revoke_desc;
    struct vnodeop_desc vnop_mmap_desc;
    struct vnodeop_desc vnop_mnomap_desc;
    struct vnodeop_desc vnop_fsync_desc;
    struct vnodeop_desc vnop_remove_desc;
    struct vnodeop_desc vnop_link_desc;
    struct vnodeop_desc vnop_rename_desc;
    struct vnodeop_desc vnop_renamex_desc;
    struct vnodeop_desc vnop_mkdir_desc;
    struct vnodeop_desc vnop_rmdir_desc;
    struct vnodeop_desc vnop_symlink_desc;
    struct vnodeop_desc vnop_readdir_desc;
    struct vnodeop_desc vnop_readdirattr_desc;
    struct vnodeop_desc vnop_getattrlistbulk_desc;
    struct vnodeop_desc vnop_readlink_desc;
    struct vnodeop_desc vnop_inactive_desc;
    struct vnodeop_desc vnop_reclaim_desc;
    struct vnodeop_desc vnop_print_desc;
    struct vnodeop_desc vnop_pathconf_desc;
    struct vnodeop_desc vnop_advlock_desc;
    struct vnodeop_desc vnop_truncate_desc;
    struct vnodeop_desc vnop_allocate_desc;
    struct vnodeop_desc vnop_pagein_desc;
    struct vnodeop_desc vnop_pageout_desc;
    struct vnodeop_desc vnop_searchfs_desc;
    struct vnodeop_desc vnop_copyfile_desc;
    struct vnodeop_desc vnop_clonefile_desc;
    struct vnodeop_desc vnop_blktooff_desc;
    struct vnodeop_desc vnop_offtoblk_desc;
    struct vnodeop_desc vnop_blockmap_desc;
    struct vnodeop_desc vnop_strategy_desc;
    struct vnodeop_desc vnop_bwrite_desc;

    #if NAMEDSTREAMS
    struct vnodeop_desc vnop_getnamedstream_desc;
    struct vnodeop_desc vnop_makenamedstream_desc;
    struct vnodeop_desc vnop_removenamedstream_desc;
    #endif
#endif

    /* TODO */
    {&vnop_default_desc, NULL},
    {&vnop_lookup_desc, NULL},
    {&vnop_open_desc, NULL},
    {&vnop_close_desc, NULL},
    {&vnop_getattr_desc, NULL},
    {&vnop_readlink_desc, NULL},
    {&vnop_reclaim_desc, NULL},
    {NULL, NULL},
};

/*
 * this stores our vnode operations array as an input and
 *  stores a final vnode array(output) that's used to create vnodes
 *  when register VFS plugin on success with vfs_fsadd()
 */
static struct vnodeopv_desc emptyfs_vnopv_desc = {
#if 0
    /* ptr to the ptr to the vector where op should go */
    int (***opv_desc_vector_p)(void *);
    /* null terminated list */
    struct vnodeopv_entry_desc *opv_desc_ops;
#endif

    &emptyfs_vnop_p,
    emptyfs_vnopv_entry_desc_list,
};

/*
 * [sic]
 * this list is an array vnodeopv_desc that allows us to
 *  register multiple vnode operations arrays at the same time
 *
 * a full-featured file system would use this to register different arrays for
 *  standard vnodes, device vnodes(VBLK and VCHR), and FIFO vnodes(VFIFO), etc.
 *
 * we only support standard vnodes .: our array only has one entry
 */
static struct vnodeopv_desc *emptyfs_vnopv_desc_list[] = {
    &emptyfs_vnopv_desc,
};

/*
 * a structure that stores function pointers to all VFS routines
 *  these functions operates on the instances of the file system itself
 *  (rather NOT file system vnodes)
 */
static struct vfsops emptyfs_vfsops = {
#if 0
    vfs_mount
    vfs_start
    vfs_unmount
    vfs_root
    vfs_quotactl
    vfs_getattr
    vfs_sync
    vfs_vget
    vfs_fhtovp
    vfs_vptofh
    vfs_init
    vfs_sysctl
    vfs_setattr
    vfs_ioctl
    vfs_vget_snapdir

    vfs_reserved5
    vfs_reserved4
    vfs_reserved3
    vfs_reserved2
    vfs_reserved1
#endif

    /* TODO */
};

#define EMPTYFS_FSTNO   0           /* will assign one dynamically */
#define EMPTYFS_NAME    "emptyfs"

/*
 * VFS_TBLTHREADSAFE, VFS_TBLFSNODELOCK:
 *  we do our own internal locking and thus don't need funnel protection
 *
 * VFS_TBLNOTYPENUM:
 *  we don't have a pre-defined file system type(the VT_XXX constants in <sys/vnode.h>)
 *  VFS should dynamically assign us a type
 *
 * VFS_TBLLOCALVOL:
 *  our file system is local; causes MNT_LOCAL to be set and indicates
 *  that the first field of our file system specific mount arguments
 *  is a path to a block device
 *
 * VFS_TBL64BITREADY:
 *  we are 64-bit aware; our mount, ioctl and sysctl entry points can be
 *  called by both 32-bit and 64-bit processes; we'll use the type of
 *  process to interpret our arguments(if they're not 32/64-bit invariant)
 */
#define EMPTYFS_VFS_FLAGS ( \
    VFS_TBLTHREADSAFE   |   \
    VFS_TBLFSNODELOCK   |   \
    VFS_TBLNOTYPENUM    |   \
    VFS_TBLLOCALVOL     |   \
    VFS_TBL64BITREADY   |   \
    0                       \
)

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
    EMPTYFS_FSTNO,
    EMPTYFS_NAME,
    EMPTYFS_VFS_FLAGS,
    {NULL, NULL},
};

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

