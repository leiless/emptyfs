/*
 * Created 181117  lynnl
 */

#ifndef __EMPTYFS_VFSOPS_H
#define __EMPTYFS_VFSOPS_H

#include <sys/mount.h>
#include <libkern/locks.h>
#include "utils.h"

readonly_extern struct vfsops emptyfs_vfsops;

/* The second largest 32-bit De Bruijn constant */
#define EMPTYFS_MNT_MAGIC       0x0fb9ac4a

#define EMPTYFS_VL_MAXLEN       32

struct emptyfs_mount {
    /* must be EMPTYFS_MNT_MAGIC */
    uint32_t magic;
    /* backing pointer to the mount_t */
    mount_t mp;
    /* debug mode passed from mount arguments */
    uint32_t dbg_mode;
    /* raw dev_t of the device we're mounted on */
    dev_t devid;
    /* backing device vnode of above;  we use a refcnt. on it */
    vnode_t devvp;
    /* volume name(UTF8 encoded) */
    char volname[EMPTYFS_VL_MAXLEN];
    /* pre-calculated volume attributes */
    struct vfs_attr attr;

    /* mutex lock used to protect following fields */
    lck_mtx_t *mtx_root;

    /* true if someone is attaching a root vnode */
    uint8_t is_root_attaching;
    /*
     * true if someone is waiting for such an attach to complete
     */
    uint8_t is_root_waiting;
    /*
     * the root vnode
     * we hold NO reference to this  you must reconfirm its existence each time
     */
    vnode_t rootvp;
};

struct emptyfs_mount *emptyfs_mount_from_mp(mount_t);

#endif /* __EMPTYFS_VFSOPS_H */

