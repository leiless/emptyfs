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

