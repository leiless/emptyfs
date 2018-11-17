/*
 * Created 181110  lynnl
 */

#ifndef __EMPTYFS_H
#define __EMPTYFS_H

#include <libkern/locks.h>
#include "utils.h"

#ifndef __kext_makefile__
#define __TZ__          "GMT"
#define BUNDLEID_S      "cn.junkman.kext.emptyfs"
#define KEXTVERSION_S   "0000.00.01"
#define KEXTBUILD_S     "1"
#endif

#define LCKGRP_NAME     BUNDLEID_S ".lckgrp"

#define EMPTYFS_FSTYPENO    0           /* will assign one dynamically */
#define EMPTYFS_NAME        "emptyfs"

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

readonly_extern lck_grp_t *lckgrp;

#endif /* __EMPTYFS_H */

