/*
 * Created 181208
 */
#ifndef __EMPTYFS_MNT_ARGS_H
#define __EMPTYFS_MNT_ARGS_H

#define EMPTYFS_NAME                "emptyfs"

/* The largest 32-bit De Bruijn constant */
#define EMPTYFS_MNTARG_MAGIC        0x0fb9ac52

struct emptyfs_mnt_args {
#ifndef KERNEL
    /* block special device to mount  example: /dev/disk0s1 */
    const char *fspec;
#endif
    uint32_t magic;         /* must be EMPTYFS_MNTARG_MAGIC */
    uint32_t dbg_mode;      /* enable debug for verbose output */
    uint32_t force_fail;    /* if non-zero  mount(2) will always fail */
};

#endif
