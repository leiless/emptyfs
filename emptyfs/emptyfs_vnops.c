/*
 * Created 181117  lynnl
 */

#include "emptyfs_vnops.h"
#include "emptyfs_vfsops.h"
#include <sys/fcntl.h>

/*
 * this variable will be set when we register VFS plugin via vfs_fsadd()
 *  it holds a pointer to the array of vnop functions for this VFS plugin
 * we declare it early :. it's referenced by the code that creates the vnodes
 */
int (**emptyfs_vnop_p)(void *);

#define VNOP_FUNC   int (*)(void *)

static inline int emptyfs_vnop_default(struct vnop_generic_args *arg __unused)
{
    return ENOTSUP;
}

static int emptyfs_vnop_lookup(struct vnop_lookup_args *);
static int emptyfs_vnop_open(struct vnop_open_args *);
static int emptyfs_vnop_close(struct vnop_close_args *);
static int emptyfs_vnop_getattr(struct vnop_getattr_args *);
static int emptyfs_vnop_readdir(struct vnop_readdir_args *);
static int emptyfs_vnop_reclaim(struct vnop_reclaim_args *);


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

    {&vnop_default_desc, (VNOP_FUNC) emptyfs_vnop_default},
    {&vnop_lookup_desc, (VNOP_FUNC) emptyfs_vnop_lookup},
    {&vnop_open_desc, (VNOP_FUNC) emptyfs_vnop_open},
    {&vnop_close_desc, (VNOP_FUNC) emptyfs_vnop_close},
    {&vnop_getattr_desc, (VNOP_FUNC) emptyfs_vnop_getattr},
    {&vnop_readdir_desc, (VNOP_FUNC) emptyfs_vnop_readdir},
    {&vnop_reclaim_desc, (VNOP_FUNC) emptyfs_vnop_reclaim},
    {NULL, NULL},
};

/*
 * this stores our vnode operations array as an input and
 *  stores a final vnode array(output) that's used to create vnodes
 *  when register VFS plugin on success with vfs_fsadd()
 */
static struct vnodeopv_desc emptyfs_vnopv_desc = {
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

/**
 * Check if a given vnode is valid in our filesystem
 *  in this fs  the only valid vnode is the root vnode
 *  .: it's a trivial implementation
 */
static void assert_valid_vnode(vnode_t vp)
{
#ifdef DEBUG
    int valid;
    struct emptyfs_mount *mntp;

    kassert_nonnull(vp);

    mntp = emptyfs_mount_from_mp(vnode_mount(vp));

    lck_mtx_lock(mntp->mtx_root);
    valid = (vp == mntp->rootvp);
    lck_mtx_unlock(mntp->mtx_root);

    kassertf(valid, "invalid vnode %p  vid: %#x type: %d",
                        vp, vnode_vid(vp), vnode_vtype(vp));
#else
    kassert_nonnull(vp);
#endif
}

/**
 * Called by VFS to do a file lookup
 * @desc    (unused) identity which vnode operation(lookup in such case)
 * @dvp     the directory to search
 * @vpp     pointer to a vnode where we return the found item
 *          the resulting vnode must have an io refcnt.
 *          and the caller is responsible to release it
 * @cnp     describes the name to search for  more see its definition
 * @ctx     identity of the calling process
 * @return  0 if found  errno o.w.
 */
static int emptyfs_vnop_lookup(struct vnop_lookup_args *ap)
{
    int e;
    struct vnodeop_desc *desc;
    vnode_t dvp;
    vnode_t *vpp;
    struct componentname *cnp;
    vfs_context_t ctx;
    vnode_t vp = NULL;

    kassert_nonnull(ap);
    desc = ap->a_desc;
    dvp = ap->a_dvp;
    vpp = ap->a_vpp;
    cnp = ap->a_cnp;
    ctx = ap->a_context;
    kassert_nonnull(desc);
    kassert_nonnull(dvp);
    kassertf(vnode_isdir(dvp), "vnop_lookup() dvp type: %d", vnode_vtype(dvp));
    assert_valid_vnode(dvp);
    kassert_nonnull(vpp);
    kassert_nonnull(cnp);
    kassert_nonnull(ctx);

    if (cnp->cn_flags & ISDOTDOT) {
        /*
         * Implement lookup for ".."(i.e. parent directory)
         * Currently this fs only has one vnode(i.e. the root vnode)
         *  and parent of root vnode is always itself
         *  it's equals to "." in such case
         * The implementation is trivial
         */
        e = vnode_get(dvp);
        if (e == 0) vp = dvp;
    } else if (!strcmp(cnp->cn_nameptr, ".")) {
        /* Ditto */
        e = vnode_get(dvp);
        if (e == 0) vp = dvp;
    } else {
        LOG_DBG("vnop_lookup() ENOENT  op: %#x flags: %#x name: %s pn: %s",
            cnp->cn_nameiop, cnp->cn_flags, cnp->cn_nameptr, cnp->cn_pnbuf);
        e = ENOENT;
    }

    /*
     * under all circumstances we should update *vpp
     *  .: we can maintain post-condition
     */
    *vpp = vp;

    if (e == 0) {
        kassert_nonnull(*vpp);
    } else {
        kassert_null(*vpp);
    }

    return e;
}

/**
 * Called by VFS to open a vnode for access
 * @vp      the vnode being opened
 * @mode    open flags
 * @ctx     identity of the calling process
 * @return  always 0
 *
 * [sic]
 * this entry is rarely useful :. VFS can read a file vnode without ever open it
 * .: any work that you'd usually do here you have to do lazily in your
 *  read/write entry points
 */
static int emptyfs_vnop_open(struct vnop_open_args *ap)
{
    struct vnodeop_desc *desc;
    vnode_t vp;
    int mode;
    vfs_context_t ctx;

    kassert_nonnull(ap);
    desc = ap->a_desc;
    vp = ap->a_vp;
    mode = ap->a_mode;
    ctx = ap->a_context;
    kassert_nonnull(desc);
    assert_valid_vnode(vp);
    kassert_known_flags(mode, O_EVTONLY | O_NONBLOCK | O_APPEND | FREAD | FWRITE);
    kassert_nonnull(ctx);

    return 0;
}

/**
 * Caller by VFS to close a vnode for access
 *
 * [sic]
 * this entry is not as useful as you might think :. a vnode can be accessed
 *  after the last close(for example, it has been mmaped)
 * in most cases  the work you might think do here
 *  you end up doing in vnop_inactive
 */
static int emptyfs_vnop_close(struct vnop_close_args *ap)
{
    struct vnodeop_desc *desc;
    vnode_t vp;
    int fflag;
    vfs_context_t ctx;

    kassert_nonnull(ap);
    desc = ap->a_desc;
    vp = ap->a_vp;
    fflag = ap->a_fflag;
    ctx = ap->a_context;
    kassert_nonnull(desc);
    assert_valid_vnode(vp);
    kassert_known_flags(fflag, O_EVTONLY | O_NONBLOCK | O_APPEND | FREAD | FWRITE);
    kassert_nonnull(ctx);

    /* empty implementation */

    return 0;
}

static int emptyfs_vnop_getattr(struct vnop_getattr_args *ap)
{
    struct vnodeop_desc *desc;
    vnode_t vp;
    struct vnode_attr *vap;
    vfs_context_t ctx;

    kassert_nonnull(ap);
    desc = ap->a_desc;
    vp = ap->a_vp;
    vap = ap->a_vap;
    ctx = ap->a_context;
    kassert_nonnull(desc);
    assert_valid_vnode(vp);
    kassert_nonnull(vap);
    kassert_nonnull(ctx);

    /* TODO */

    return 0;
}

static int emptyfs_vnop_readdir(struct vnop_readdir_args *ap)
{
    UNUSED(ap);
    return 0;
}

/*
 * High-level function to relacim a vnode
 */
static void detach_root_vnode(struct emptyfs_mount *mntp, vnode_t vp)
{
    int e;

    kassert_nonnull(mntp);
    kassert_nonnull(vp);

    lck_mtx_lock(mntp->mtx_root);

    /*
     * [sic]
     * if `is_root_attaching' is set  rootvp will and must be NULL
     *  if in such case  we just return
     * that's expected behaviour if the system tries to reclaim the vnode
     *  while other thread is in process of attaching it
     */
    if (mntp->is_root_attaching) {
        kassert_null(mntp->rootvp);
    }

    if (mntp->rootvp != NULL) {
        /* The only possible vnode is the root vnode */
        kassert(mntp->rootvp == vp);

        e = vnode_removefsref(mntp->rootvp);
        kassertf(e == 0, "vnode_removefsref() fail  errno: %d", e);

        mntp->rootvp = NULL;
    } else {
        /* Do nothing  someone else beat this reclaim */
    }

    lck_mtx_unlock(mntp->mtx_root);
}

/**
 * Called by VFS to disassociate a vnode from underlying fsnode
 *
 * @vp      the vnode to reclaim
 * @ctx     identity of the calling process
 * @return  always 0(the kernel will panic if fail)
 *
 * trivial reclaim implementation
 *  it's NOT the point where  for example you write the fsnode back to disk
 *  rather you should do this in vnop_inactive entry point
 * in a proper file system  this entry would have to coordinate
 *  with fsnode hash layer
 */
static int emptyfs_vnop_reclaim(struct vnop_reclaim_args *ap)
{
    struct vnodeop_desc *desc;
    vnode_t vp;
    vfs_context_t ctx;
    struct emptyfs_mount *mntp;

    kassert_nonnull(ap);
    desc = ap->a_desc;
    vp = ap->a_vp;
    ctx = ap->a_context;
    kassert_nonnull(desc);
    assert_valid_vnode(vp);
    kassert_nonnull(ctx);

    /* do reclaim as if we have a fsnoe hash layer */
    mntp = emptyfs_mount_from_mp(vnode_mount(vp));
    detach_root_vnode(mntp, vp);

    return 0;
}

