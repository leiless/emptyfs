/*
 * Created 181117  lynnl
 */

#include "emptyfs_vnops.h"

/*
 * this variable will be set when we register VFS plugin via vfs_fsadd()
 *  it holds a pointer to the array of vnop functions for this VFS plugin
 * we declare it early :. it's referenced by the code that creates the vnodes
 */
int (**emptyfs_vnop_p)(void *);

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
struct vnodeopv_desc *emptyfs_vnopv_desc_list[__EMPTYFS_OPV_SZ] = {
    &emptyfs_vnopv_desc,
};

