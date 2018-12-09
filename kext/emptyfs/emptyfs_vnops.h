/*
 * Created 181117  lynnl
 */

#ifndef __EMPTYFS_VNOPS_H
#define __EMPTYFS_VNOPS_H

#include <sys/mount.h>
#include <sys/vnode.h>
#include "utils.h"

#define __EFS_VNOPV_SZ      1      /* used in this header only */
readonly_extern struct vnodeopv_desc *emptyfs_vnopv_desc_list[__EFS_VNOPV_SZ];

extern int (**emptyfs_vnop_p)(void *);

#endif /* __EMPTYFS_VNOPS_H */

