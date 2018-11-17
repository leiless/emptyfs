/*
 * Created 181117  lynnl
 */

#include "emptyfs_vfsops.h"

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

    /* TODO */
};

