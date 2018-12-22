/*
 * Created 181117  lynnl
 */

/*
 * user*_addr_t definition used in <sys/fcntl.h>
 *  o.w. os <= 10.11(El Capitan) may unable to compile
 */
#include <i386/types.h>
#include <sys/fcntl.h>
#include <sys/dirent.h>
#include <string.h>

#include "emptyfs_vnops.h"
#include "emptyfs_vfsops.h"

/*
 * this variable will be set when we register VFS plugin via vfs_fsadd()
 *  it holds a pointer to the array of vnop functions for this VFS plugin
 * we declare it early :. it's referenced by the code that creates the vnodes
 */
int (**emptyfs_vnop_p)(void *);

#define VNOP_FUNC   int (*)(void *)

/*
 * Q: default vnode operation if a specific vnop not yet implemented?
 *      i.e. a fallback generic operation if NYI
 *
 * NOTE:
 *  you may use builtin <sys/vnode.h>#vn_default_error()
 *  it merely return ENOTSUP  like what we do
 *  the KPI resides in com.apple.kpi.bsd
 */
static inline int emptyfs_vnop_default(struct vnop_generic_args *arg)
{
    kassert_nonnull(arg);
    LOG_TRA("desc: %p", arg->a_desc);
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
struct vnodeopv_desc *emptyfs_vnopv_desc_list[__EFS_VNOPV_SZ] = {
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
 * Called by VFS to do a directory lookup
 * @desc    (unused) identity which vnode operation(lookup in such case)
 * @dvp     the directory to search
 * @vpp     pointer to a vnode where we return the found item
 *          the resulting vnode must have an io refcnt.
 *          and the caller is responsible to RELEASE it
 * @cnp     describes the name to search for  more see <sys/vnode.h>
 * @ctx     identity of the calling process
 * @return  0 if found  errno o.w.
 *
 * see:
 *  search 'macos dot underscore files'
 *  https://www.dropboxforum.com/t5/Syncing-and-uploads/What-can-we-do-with-files/td-p/186881
 *  https://apple.stackexchange.com/questions/14980/why-are-dot-underscore-files-created-and-how-can-i-avoid-them
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

    LOG_TRA("desc: %p dvp: %p %#x vpp: %p %p cnp: %u %#x %s %s",
            desc, dvp, vnode_vid(dvp), vpp, *vpp,
            cnp->cn_nameiop, cnp->cn_flags, cnp->cn_pnbuf, cnp->cn_nameptr);

    /* Trivial implementation */

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
 * Called by VFS to open a file for access
 * @vp      the vnode being opened
 * @mode    open flags
 * @ctx     identity of the calling process
 * @return  always 0
 *
 * [sic]
 * this entry is rarely useful :. VFS can read a file vnode without ever open it
 * .: any work that you'd usually do here you have to do lazily in your
 *  read/write entry points
 *
 * see: man open(2)
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
    kassert(vnode_isdir(vp));
    assert_valid_vnode(vp);
    /* NOTE: there seems too many open flags */
    kassert_known_flags(mode, O_CLOEXEC | O_DIRECTORY | O_EVTONLY |
                                O_NONBLOCK | O_APPEND | FREAD | FWRITE);
    kassert_nonnull(ctx);

    LOG_TRA("desc: %p vp: %p %#x mode: %#x", desc, vp, vnode_vid(vp), mode);

    /* Empty implementation */

    return 0;
}

/**
 * Caller by VFS to close a file from access
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
    kassert(vnode_isdir(vp));
    assert_valid_vnode(vp);
    /* NOTE: there seems too many open flags */
    kassert_known_flags(fflag, O_EVTONLY | O_NONBLOCK | O_APPEND | FREAD | FWRITE);
    kassert_nonnull(ctx);

    LOG_TRA("desc: %p vp: %p %#x fflag: %#x", desc, vp, vnode_vid(vp), fflag);

    /* Empty implementation */

    return 0;
}

/*
 * Called by VFS to get attributes about a vnode
 *  (i.e. backing support of stat getattrlist syscalls)
 *
 * @vp      the vnode whose attributes to retrieve
 * @vap     describes the attributes requested upon return
 * @ctx     identity of the calling process
 * @return  0 if found  errno o.w.
 *
 * [sic] XXX:
 *  for attributes whose values readily available  use VATTR_RETURN
 *    macro to unilaterally return the value
 *
 *  for attributes whose values hard to calculate  use VATTR_IS_ACTIVE to see
 *    if the caller requested that attribute
 *    copy the value into the appropriate field if it's
 */
static int emptyfs_vnop_getattr(struct vnop_getattr_args *ap)
{
    struct vnodeop_desc *desc;
    vnode_t vp;
    struct vnode_attr *vap;
    vfs_context_t ctx;
    struct emptyfs_mount *mntp;

    kassert_nonnull(ap);
    desc = ap->a_desc;
    vp = ap->a_vp;
    vap = ap->a_vap;
    ctx = ap->a_context;
    kassert_nonnull(desc);
    /* We have only one vnode(i.e. the root vnode) */
    kassert(vnode_isdir(vp));
    assert_valid_vnode(vp);
    kassert_nonnull(vap);
    kassert_nonnull(ctx);

    LOG_TRA("desc: %p vp: %p %#x va_active: %#llx va_supported: %#llx",
            desc, vp, vnode_vid(vp), vap->va_active, vap->va_supported);

    mntp = emptyfs_mount_from_mp(vnode_mount(vp));

    /* Trivial implementation */

    /*
     * [sic]
     * Implementation of stat(2) requires that we support va_rdev
     *  even on vnodes that aren't device vnode
     */
    VATTR_RETURN(vap, va_rdev, 0);
    VATTR_RETURN(vap, va_nlink, 2);
    VATTR_RETURN(vap, va_data_size, sizeof(struct dirent) << 1);

    /* umask 0555 */
    VATTR_RETURN(vap, va_mode,
        S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    VATTR_RETURN(vap, va_create_time, mntp->attr.f_create_time);
    VATTR_RETURN(vap, va_access_time, mntp->attr.f_access_time);
    VATTR_RETURN(vap, va_modify_time, mntp->attr.f_modify_time);
    VATTR_RETURN(vap, va_change_time, mntp->attr.f_modify_time);

    VATTR_RETURN(vap, va_fileid, 2);
    VATTR_RETURN(vap, va_fsid, mntp->devid);

#if 0
    VATTR_RETURN(vap, va_type, XXX);    /* Handled by VFS */
    /*
     * Let VFS get from f_mntonname
     * see: emptyfs_vfsops.c#emptyfs_vfsop_mount()
     */
    VATTR_RETURN(vap, va_name, XXX);
#endif

    LOG_TRA("va_active: %#llx va_supported: %#llx",
            vap->va_active, vap->va_supported);

    return 0;
}

/**
 * Safe wrapper of uiomove()
 * @addr        source address
 * @size        source address buffer size
 * @uio         "move" destination
 * @return      0 if success  errno o.w.
 */
static int uiomove_atomic(
        void * __nonnull addr,
        size_t size,
        uio_t __nonnull uio)
{
    int e;

    kassert_nonnull(addr);
    kassert_nonnull(uio);

    if (unlikely(size > INT_MAX)) {
        e = ERANGE;
    } else if (unlikely(size > (user_size_t) uio_resid(uio))) {
        e = ENOBUFS;
    } else {
        e = uiomove(addr, (int) size, uio);
    }

    if (e) {
        LOG_ERR("uiomove_atomic() fail  size: %zu resid: %lld errno: %d",
                    size, uio_resid(uio), e);
    }

    return e;
}

/*
 * Called by VFS to iterate contents of a directory
 *  (i.e. backing support of getdirentries syscall)
 *
 * @vp          the directory we're iterating
 * @uio         destination information for resulting direntries
 * @flags       iteration options
 *              currently there're 4 options  neither of which we support
 *              needed if the file system is to be NFS exported
 * @eofflag     return a flag to indicate if we reached the last directory entry
 *              should be set to 1 if the end of the directory has been reached
 * @numdirent   return a count of number of directory entries that we've read
 *              should be set to number of entries written into buffer
 * @ctx         identity of the calling process
 *
 * [sic]
 * The hardest thing to understand about this entry point is the UIO management
 *  there are two tricky aspects
 * For more info you should check sample code func docs
 */
static int emptyfs_vnop_readdir(struct vnop_readdir_args *ap)
{
    int e = 0;
    struct vnodeop_desc *desc;
    vnode_t vp;
    struct uio *uio;
    int flags;
    int *eofflag;
    int *numdirent;
    vfs_context_t ctx;

    int eof = 0;
    int num = 0;
    struct dirent di;
    off_t index;

    kassert_nonnull(ap);
    desc = ap->a_desc;
    vp = ap->a_vp;
    uio = ap->a_uio;
    flags = ap->a_flags;
    eofflag = ap->a_eofflag;
    numdirent = ap->a_numdirent;
    ctx = ap->a_context;
    kassert_nonnull(desc);
    kassert(vnode_isdir(vp));
    assert_valid_vnode(vp);
    kassert_nonnull(uio);
    kassert_known_flags(flags,
        VNODE_READDIR_EXTENDED | VNODE_READDIR_REQSEEKOFF |
        VNODE_READDIR_SEEKOFF32 | VNODE_READDIR_NAMEMAX);
    /* eofflag and numdirent can be NULL */
    kassert_nonnull(ctx);

    LOG_TRA("desc: %p vp: %p %#x flags: %#x uio: "
            "(isuserspace: %d resid: %lld "
            "iovcnt: %d offset: %lld "
            "curriovbase: %llu curriovlen: %llu)",
            desc, vp, vnode_vid(vp), flags,
            uio_isuserspace(uio), uio_resid(uio),
            uio_iovcnt(uio), uio_offset(uio),
            uio_curriovbase(uio), uio_curriovlen(uio));

    /* Trivial implementation */

    if (flags & (VNODE_READDIR_EXTENDED | VNODE_READDIR_REQSEEKOFF |
                VNODE_READDIR_SEEKOFF32 | VNODE_READDIR_NAMEMAX)) {
        /* only make sense if backing file system is NFS exported */
        e = EINVAL;
        goto out_exit;
    }

    di.d_fileno = 2;                /* Q: why NOT set dynamically? */
    di.d_reclen = sizeof(di);
    di.d_type = DT_DIR;

    kassert(uio_offset(uio) % 7 == 0);
    index = uio_offset(uio) / 7;

    if (index == 0) {
        di.d_namlen = (uint8_t) strlen(".");
        strlcpy(di.d_name, ".", sizeof(di.d_name));
        e = uiomove_atomic(&di, sizeof(di), uio);
        if (e == 0) {
            num++;
            index++;
        }
    }

    if (e == 0 && index == 1) {
        di.d_namlen = (uint8_t) strlen("..");
        strlcpy(di.d_name, "..", sizeof(di.d_name));
        e = uiomove_atomic(&di, sizeof(di), uio);
        if (e == 0) {
            num++;
            index++;
        }
    }

    /*
     * If we failed :. there wasn't enough space in user space buffer
     *  just swallow the error  this will resulting getdirentries(2) returning
     *  less than the buffer size(possibly even zero)
     *  the caller is expected to cope with that
     */
    if (e == ENOBUFS) {
        e = 0;
    } else if (e) {
        goto out_exit;
    }

    /* Update uio offset and set EOF flag */
    uio_setoffset(uio, index * 7);
    eof = index > 1;

    /* Copy out any info requested by caller */
    if (eofflag != NULL)    *eofflag = eof;
    if (numdirent != NULL)  *numdirent = num;

    LOG_TRA("eofflag: %p %d numdirent: %p %d", eofflag, eof, numdirent, num);

out_exit:
    return e;
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
 * [sic] Release filesystem-internal resources for a vnode
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

    LOG_TRA("desc: %p vp: %p %#x", desc, vp, vnode_vid(vp));

    /* do reclaim as if we have a fsnoe hash layer */
    mntp = emptyfs_mount_from_mp(vnode_mount(vp));
    detach_root_vnode(mntp, vp);

    return 0;
}

