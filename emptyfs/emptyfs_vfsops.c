/*
 * Created 181117  lynnl
 */

#include "emptyfs_vfsops.h"

static int emptyfs_vfsop_mount(struct mount *, vnode_t, user_addr_t, vfs_context_t);
static int emptyfs_vfsop_start(struct mount *, int, vfs_context_t);
static int emptyfs_vfsop_unmount(struct mount *, int, vfs_context_t);
static int emptyfs_vfsop_root(struct mount *, struct vnode **, vfs_context_t);
static int emptyfs_vfsop_getattr(struct mount *, struct vfs_attr *, vfs_context_t);

/*
 * a structure that stores function pointers to all VFS routines
 *  these functions operates on the instances of the file system itself
 *  (rather NOT file system vnodes)
 */
struct vfsops emptyfs_vfsops = {
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

    .vfs_mount = emptyfs_vfsop_mount,
    .vfs_start = emptyfs_vfsop_start,
    .vfs_unmount = emptyfs_vfsop_unmount,
    .vfs_root = emptyfs_vfsop_root,
    .vfs_getattr = emptyfs_vfsop_getattr,
};

/*
 * Called by VFS to mount an instance of our file system
 *
 * @mp      mount structure for the newly mounted filesystem
 * @devvp   either an open vnode for the backing block device which
 *          going to be mounted  or NULL
 *          in the former case  the first field of our filesystem-specific
 *          mount argument must be a pointer to C-style string holding
 *          an UTF-8 encoded path to the block device node
 * @data    filesystem-specific data passed down from userspace via mount(2)
 *          :. VFS_TBLLOCALVOL is set  the first field of this structure
 *          must be a pointer to the path of the backing block device node
 *          kernel'll interpret this argument  opening up the node for us
 * @ctx     context to authenticate for mount
 * @return  0 if success  errno o.w.
 *          if a mount call fails  the filesystem must clean up any state
 *          it has constructed  :. vfs-level mount code will not clean it up
 *
 * XXX:
 *  if VFS_TBLLOCALVOL is set  first field of the filesystem-specific mount
 *  argument is interpreted by kernel and kernel'll ADVANCE data pointer to
 *  the field straight after the path
 *  we handle this by defining filesystem-specific argument in two ways
 *  1) in userspace     first field is a pointer to the block device node path
 *  2) in kernel        we omit the field aforementioned
 *
 * XXX:
 *  if VFS_TBL64BITREADY is set(i.e. declare your fs to be 64-bit ready)
 *  you must be prepared to handle mount requests from both 32/64-bit proc.
 *  .: your filesystem-specific mount arguments must be either 32/64-bit invariant
 *  (just as this fs)  or you must interpret them differently depends on the
 *  type of process you're calling by  (see: <sys/proc.h>@proc_is64bit)
 */
static int emptyfs_vfsop_mount(
        struct mount *mp,
        vnode_t devvp,
        user_addr_t data,
        vfs_context_t ctx)
{
    UNUSED(mp, devvp, data, ctx);
    /* TODO */
    return 0;
}

static int emptyfs_vfsop_start(
        struct mount *mp,
        int flags,
        vfs_context_t ctx)
{
    UNUSED(mp, flags, ctx);
    return 0;
}

static int emptyfs_vfsop_unmount(
        struct mount *mp,
        int flags,
        vfs_context_t ctx)
{
    UNUSED(mp, flags, ctx);
    return 0;
}

static int emptyfs_vfsop_root(
        struct mount *mp,
        struct vnode **vpp,
        vfs_context_t ctx)
{
    UNUSED(mp, vpp, ctx);
    return 0;
}

static int emptyfs_vfsop_getattr(
        struct mount *mp,
        struct vfs_attr *attr,
        vfs_context_t ctx)
{
    UNUSED(mp, attr, ctx);
    return 0;
}

