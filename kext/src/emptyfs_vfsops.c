/*
 * Created 181117  lynnl
 */

#include <libkern/libkern.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kauth.h>
#include <sys/proc.h>
#include <string.h>

#include "emptyfs_vfsops.h"
#include "emptyfs_vnops.h"
#include "emptyfs.h"
#include "utils.h"

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
    .vfs_mount = emptyfs_vfsop_mount,
    .vfs_start = emptyfs_vfsop_start,
    .vfs_unmount = emptyfs_vfsop_unmount,
    .vfs_root = emptyfs_vfsop_root,
    .vfs_getattr = emptyfs_vfsop_getattr,
};

/*
 * Get filesystem-specific mount structure
 */
struct emptyfs_mount *emptyfs_mount_from_mp(mount_t __nonnull mp)
{
    struct emptyfs_mount *mntp;
    kassert_nonnull(mp);
    mntp = vfs_fsprivate(mp);
    kassert_nonnull(mntp);
    kassert(mntp->magic == EMPTYFS_MNT_MAGIC);
    kassert(mntp->mp == mp);
    return mntp;
}

#define VFS_ATTR_BLKSZ  4096

/*
 * Initialize `stuct vfs_attr''s f_capabilities and f_attributes
 * TODO: put those attrlist into a solo header
 * see: xnu/bsd/hfs/hfs_attrlist.h
 */
static void emptyfs_init_volattrs(struct emptyfs_mount * __nonnull mntp)
{
    vol_capabilities_attr_t *cap;
    vol_attributes_attr_t *attr;

    kassert_nonnull(mntp);

    cap = &mntp->attr.f_capabilities;
    attr = &mntp->attr.f_attributes;

    cap->capabilities[VOL_CAPABILITIES_FORMAT] = 0
        | VOL_CAP_FMT_NO_ROOT_TIMES
        | VOL_CAP_FMT_CASE_SENSITIVE
        | VOL_CAP_FMT_CASE_PRESERVING
        | VOL_CAP_FMT_FAST_STATFS
        | VOL_CAP_FMT_2TB_FILESIZE
#if defined(OS_VER_MIN_REQ) && OS_VER_MIN_REQ >= __MAC_10_12
        | VOL_CAP_FMT_NO_PERMISSIONS
#elif defined(OS_VER_MIN_REQ)
#warning Some volume capabilities may unavailable under macOS target <= 10.12
#else
#warning OS_VER_MIN_REQ undefined
#endif
        ;

    /* XXX: forcibly mark all capabilities as valid? */
    cap->valid[VOL_CAPABILITIES_FORMAT] = (__typeof(*(cap->valid))) -1;

    cap->capabilities[VOL_CAPABILITIES_INTERFACES] = VOL_CAP_INT_ATTRLIST;
    cap->valid[VOL_CAPABILITIES_INTERFACES] = (__typeof(*(cap->valid))) -1;

    attr->validattr.commonattr = 0
        | ATTR_CMN_NAME
        | ATTR_CMN_DEVID
        | ATTR_CMN_FSID
        | ATTR_CMN_OBJTYPE
        | ATTR_CMN_OBJID
        | ATTR_CMN_PAROBJID     /* Q: Parent object ID? */
        | ATTR_CMN_CRTIME
        | ATTR_CMN_MODTIME
        | ATTR_CMN_CHGTIME      /* Same as ATTR_CMN_MODTIME */
        | ATTR_CMN_ACCTIME
        | ATTR_CMN_OWNERID
        | ATTR_CMN_GRPID
        | ATTR_CMN_ACCESSMASK
        | ATTR_CMN_FLAGS;

    attr->validattr.dirattr = 0;

    attr->validattr.fileattr = 0
        | ATTR_FILE_TOTALSIZE
        | ATTR_FILE_IOBLOCKSIZE
        | ATTR_FILE_DATALENGTH
        | ATTR_FILE_DATAALLOCSIZE;

    attr->validattr.forkattr = 0;

    attr->validattr.volattr = 0
        | ATTR_VOL_FSTYPE
        | ATTR_VOL_SIZE
        | ATTR_VOL_SPACEFREE
        | ATTR_VOL_SPACEAVAIL
        | ATTR_VOL_IOBLOCKSIZE
        | ATTR_VOL_OBJCOUNT
        | ATTR_VOL_FILECOUNT
        | ATTR_VOL_DIRCOUNT
        | ATTR_VOL_MAXOBJCOUNT
        | ATTR_VOL_MOUNTPOINT
        | ATTR_VOL_NAME
        | ATTR_VOL_MOUNTFLAGS
        | ATTR_VOL_MOUNTEDDEVICE
        | ATTR_VOL_CAPABILITIES
        | ATTR_VOL_UUID
        | ATTR_VOL_ATTRIBUTES;

    /* All attributes that we do support  we support natively */
    bcopy(&attr->validattr, &attr->nativeattr, sizeof(attr->validattr));
}

/*
 * Initialize VFS attributes in `struct emptyfs_mount' with appropriate
 *  static values   (this done in initialization time  .: it's thread-safe)
 */
static void emptyfs_init_attrs(
        struct emptyfs_mount * __nonnull mntp,
        vfs_context_t __nonnull ctx)
{
    kauth_cred_t cred;
    uid_t uid;
    struct timespec ts;
    uuid_string_t uuid;

    kassert_nonnull(mntp);
    kassert_nonnull(ctx);

    cred = vfs_context_ucred(ctx);
    kassert_nonnull(cred);
    uid = kauth_cred_getuid(cred);

    mntp->attr.f_objcount = 1;
    mntp->attr.f_filecount = 0;
    mntp->attr.f_dircount = 1;
    mntp->attr.f_maxobjcount = 1;

    mntp->attr.f_bsize = VFS_ATTR_BLKSZ;
    mntp->attr.f_iosize = VFS_ATTR_BLKSZ;
    mntp->attr.f_blocks = 1;
    mntp->attr.f_bfree = 0;
    mntp->attr.f_bavail = 0;
    mntp->attr.f_bused = 1;
    mntp->attr.f_files = 1;
    mntp->attr.f_ffree = 0;

    mntp->attr.f_fsid.val[0] = mntp->devid;
    mntp->attr.f_fsid.val[1] = vfs_typenum(mntp->mp);
    mntp->attr.f_owner = uid;

    emptyfs_init_volattrs(mntp);

    nanotime(&ts);
    LOG_DBG("fs ctime mtim atime: %ld", ts.tv_sec + (ts.tv_nsec / 1000000000L));
    bcopy(&ts, &mntp->attr.f_create_time, sizeof(ts));
    bcopy(&ts, &mntp->attr.f_modify_time, sizeof(ts));
    bcopy(&ts, &mntp->attr.f_access_time, sizeof(ts));

    mntp->attr.f_fssubtype = 0;

    mntp->attr.f_vol_name = mntp->volname;

    uuid_generate_random(mntp->attr.f_uuid);
    format_uuid_string(mntp->attr.f_uuid, uuid);
    LOG_DBG("file system UUID: %s", uuid);

    /* remaining not supported implicitly */
}

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
 *  if VFS_TBL64BITREADY is set(i.e. claim your fs to be 64-bit ready)
 *  you must be prepared to handle mount requests from both 32/64-bit proc.
 *  .: your filesystem-specific mount arguments must be either 32/64-bit invariant
 *  (just as this fs)  or you must interpret them differently depends on the
 *  type of process you're calling by  (see: <sys/proc.h>@proc_is64bit)
 */
static int emptyfs_vfsop_mount(
        struct mount *mp,
        vnode_t devvp,
        user_addr_t udata,
        vfs_context_t ctx)
{
    int e, e2;
    struct emptyfs_mnt_args args;
    struct emptyfs_mount *mntp;
    struct vfsstatfs *st;

    kassert_nonnull(mp);
    kassert_nonnull(devvp);
    kassert_nonnull(udata);     /* Q: what if user passed NULL mount(3) arg. */
    kassert_nonnull(ctx);

    LOG_DBG("mp: %p devvp: %p(%d) %#x data: %#llx",
            mp, devvp, vnode_vtype(devvp), vnode_vid(devvp), udata);

    /*
     * this fs doesn't support over update a volume's state
     * for example: upgrade it from readonly to readwrite
     */
    if (vfs_isupdate(mp) || vfs_iswriteupgrade(mp)) {
        e = ENOTSUP;
        LOG_ERR("mount %p is updating  we doesn't support it", mp);
        goto out_exit;
    }

    e = copyin(udata, &args, sizeof(args));
    if (e) {
        LOG_ERR("copyin() fail  errno: %d", e);
        goto out_exit;
    }

    if (args.magic != EMPTYFS_MNTARG_MAGIC) {
        e = EINVAL;
        LOG_ERR("bad mount arguments from mount(2)  magic: %#x", args.magic);
        goto out_exit;
    }

    mntp = util_malloc(sizeof(*mntp), M_ZERO);
    if (mntp == NULL) {
        e = ENOMEM;
        LOG_ERR("util_malloc() fail  errno: %d", e);
        goto out_exit;
    }

    vfs_setfsprivate(mp, mntp);

    /* fill out fields in our mount point */

    e = vnode_ref(devvp);
    if (e) {
        LOG_ERR("vnode_ref() fail  errno: %d", e);
        goto out_exit;
    }
    mntp->devvp = devvp;
    mntp->devid = vnode_specrdev(devvp);

    mntp->mtx_root = lck_mtx_alloc_init(lckgrp, NULL);
    if (mntp->mtx_root == NULL) {
        e = ENOMEM;
        LOG_ERR("lck_mtx_alloc_init() fail  errno: %d", e);
        goto out_exit;
    }

    mntp->magic = EMPTYFS_MNT_MAGIC;
    mntp->mp = mp;
    mntp->dbg_mode = args.dbg_mode;
    kassert(strlen(EMPTYFS_NAME) < sizeof(mntp->volname));
    (void) strlcpy(mntp->volname, EMPTYFS_NAME, sizeof(mntp->volname));
    /*
     * make sure that mntp->devid initialized
     * :. emptyfs_init_attrs() reads it
     */
    emptyfs_init_attrs(mntp, ctx);

    kassert(!mntp->is_root_attaching);
    kassert(!mntp->is_root_waiting);
    kassert(mntp->rootvp == NULL);

    st = vfs_statfs(mp);
    kassert_nonnull(st);
    kassert(!strcmp(st->f_fstypename, EMPTYFS_NAME));

    st->f_bsize = mntp->attr.f_bsize;
    st->f_iosize = mntp->attr.f_iosize;
    st->f_blocks = mntp->attr.f_blocks;
    st->f_bfree = mntp->attr.f_bfree;
    st->f_bavail = mntp->attr.f_bavail;
    st->f_bused = mntp->attr.f_bused;
    st->f_files = mntp->attr.f_files;
    st->f_ffree = mntp->attr.f_ffree;
    st->f_fsid = mntp->attr.f_fsid;
    st->f_owner = mntp->attr.f_owner;

    vfs_setflags(mp, MNT_RDONLY | MNT_NOEXEC | MNT_NOSUID |
                        MNT_NODEV | MNT_IGNORE_OWNERSHIP);

    /* no need to call vnode_setmountedon() :. the system already done that */

    if (args.force_fail) {
        /* force failure allows us to test unmount path */
        e = ECANCELED;
        LOG_ERR("mount emptyfs success yet force failure  errno: %d", e);
        goto out_exit;
    } else {
        LOG_INF("mount emptyfs success  rdev: %#x dbg: %d",
                    mntp->devid, mntp->dbg_mode);
    }

out_exit:
    if (e) {
        e2 = emptyfs_vfsop_unmount(mp, MNT_FORCE, ctx);
        kassertf(e2 == 0, "why force unmount fail?  errno: %d", e2);
    }

    return e;
}

/*
 * Called by VFS to mark a mount as ready to be used
 *
 * after receiving this calldown
 *  a filesystem will be hooked into the mount list
 *  and should expect calls down from the VFS layer
 *
 * this entry point isn't particularly useful  to avoid concurrency problems
 *  you should do all mount initialization in VFS mount entry point
 * moreover  it's unnecessary to implement this :. in the VFS_START call
 *  if you glue a NULL to vfs_start field  it returns ENOTSUP
 *  and the caller ignores the result
 *
 * @mp      the mount structure reference
 * @flags   unused
 * @ctx     context to authenticate for mount
 * @return  return value is ignored
 *
 * see:
 *  xnu/bsd/vfs/kpi_vfs.c#VFS_START
 *  xnu/bsd/vfs/vfs_subr.c#vfs_mountroot
 *  xnu/bsd/vfs/vfs_syscalls.c#mount_common
 */
static int emptyfs_vfsop_start(
        struct mount *mp,
        int flags,
        vfs_context_t ctx)
{
    kassert_nonnull(mp);
    kassert_known_flags(flags, 0);
    kassert_nonnull(ctx);

    LOG_DBG("mp: %p flags: %#x", mp, flags);

    return 0;
}

/*
 * Called by VFS to unmount a volume
 * @mp          mount reference
 * @flags       unmount flags
 * @ctx         identity of the calling process
 * @return      0 if success  errno o.w.
 */
static int emptyfs_vfsop_unmount(
        struct mount *mp,
        int flags,
        vfs_context_t ctx)
{
    int e;
    int flush_flags;
    struct emptyfs_mount *mntp;

    kassert_nonnull(mp);
    kassert_known_flags(flags, MNT_FORCE);
    kassert_nonnull(ctx);

    LOG_DBG("mp: %p flags: %#x", mp, flags);

    flush_flags = (flags & MNT_FORCE) ? FORCECLOSE : 0;

    e = vflush(mp, NULL, flush_flags);
    if (e) {
        LOG_ERR("vflush() fail  errno: %d", e);
        goto out_exit;
    }

    mntp = vfs_fsprivate(mp);
    if (mntp == NULL) goto out_exit;

    if (mntp->devvp != NULL) {
        vnode_rele(mntp->devvp);
        mntp->devvp = NULL;
        mntp->devid = 0;
    }

    kassert(!mntp->is_root_attaching);
    kassert(!mntp->is_root_waiting);

    /*
     * vflush() call above forces VFS to reclaim any vnode in our volume
     *  in such case  the root vnode should be NULL
     */
    kassert(mntp->rootvp == NULL);

    if (mntp->mtx_root != NULL) lck_mtx_free(mntp->mtx_root, lckgrp);

    mntp->magic = 0;    /* our mount invalidated  reset the magic */

    util_mfree(mntp);

out_exit:
    return e;
}

/**
 * @return      root vnode of the volume(will create if necessary)
 *              resulting vnode has an io refcnt. whcih the caller is
 *              responsible to release it via vnode_put()
 */
static int get_root_vnode(
        struct emptyfs_mount * __nonnull mntp,
        vnode_t * __nonnull vpp)
{
    int e;
    int e2;
    vnode_t vn = NULL;
    uint32_t vid;
    struct vnode_fsparam param;

    kassert_nonnull(mntp);
    kassert_nonnull(vpp);
    kassert_null(*vpp);

    lck_mtx_lock(mntp->mtx_root);

    do {
        kassert_null(vn);
        lck_mtx_assert(mntp->mtx_root, LCK_MTX_ASSERT_OWNED);

        if (mntp->is_root_attaching) {
            mntp->is_root_waiting = 1;
            (void) msleep(&mntp->rootvp, mntp->mtx_root, PINOD, NULL, NULL);
            kassert(mntp->is_root_waiting == 0);
            e = EAGAIN;
        } else if (mntp->rootvp == NULLVP) {
            mntp->is_root_attaching = 1;
            lck_mtx_unlock(mntp->mtx_root);

            param.vnfs_mp = mntp->mp;
            param.vnfs_vtype = VDIR;
            param.vnfs_str = NULL;
            param.vnfs_dvp = NULL;
            param.vnfs_fsnode = NULL;
            param.vnfs_vops = emptyfs_vnop_p;
            param.vnfs_markroot = 1;
            param.vnfs_marksystem = 0;
            param.vnfs_rdev = 0;        /* we don't support VBLK and VCHR */
            param.vnfs_filesize = 0;    /* nonsense for VDIR */
            param.vnfs_cnp = NULL;
            param.vnfs_flags = VNFS_NOCACHE | VNFS_CANTCACHE;   /* no namecache */

            e = vnode_create(VNCREATE_FLAVOR, sizeof(param), &param, &vn);
            if (e == 0) {
                kassert_nonnull(vn);
                LOG_DBG("vnode_create() ok  vn: %p vid: %#x", vn, vnode_vid(vn));
            } else {
                kassert_null(vn);
                LOG_ERR("vnode_create() fail  errno: %d", e);
            }

            lck_mtx_lock(mntp->mtx_root);
            if (e == 0) {
                kassert_null(mntp->rootvp);
                mntp->rootvp = vn;
                e2 = vnode_addfsref(vn);
                kassertf(e2 == 0, "vnode_addfsref() fail  errno: %d", e2);

                kassert(mntp->is_root_attaching);
                mntp->is_root_attaching = 0;
                if (mntp->is_root_waiting) {
                    mntp->is_root_waiting = 0;      /* reset beforehand */
                    wakeup(&mntp->rootvp);
                }
            }
        } else {
            /* we already have a root vnode  try get with vnode vid */
            vn = mntp->rootvp;
            kassert_nonnull(vn);
            vid = vnode_vid(vn);
            lck_mtx_unlock(mntp->mtx_root);

            e = vnode_getwithvid(vn, vid);
            if (e == 0) {
                /*
                 * ok  vnode_getwithvid() has taken an IO refcnt. to the vnode
                 * this reference prevent from being reclaimed in the interim
                 */
            } else {
                /*
                 * the vnode has been relaimed  likely between dropping the lock
                 *  and calling the vnode_getwithvid()
                 * .: we loop again to get updated vnode root(hopefully)
                 */
                LOG_ERR("vnode_getwithvid() fail  errno: %d", e);
                vn = NULL;                      /* loop invariant */
                e = EAGAIN;
            }

            lck_mtx_lock(mntp->mtx_root);       /* loop invariant */
        }
    } while (e == EAGAIN);

    lck_mtx_unlock(mntp->mtx_root);

    if (e == 0) {
        kassert_nonnull(vn);
        *vpp = vn;
    } else {
        kassert_null(vn);
    }

    return e;
}

/*
 * Called by VFS to get root vnode of this file system
 * @mp      the mount structure reference
 * @vpp     pointer to a vnode reference
 *          on success we must update it and have an io refcnt on it
 *          and it's the caller's responsibility to release it
 * @ctx     context to authenticate for getting the root
 */
static int emptyfs_vfsop_root(
        struct mount *mp,
        struct vnode **vpp,
        vfs_context_t ctx)
{
    int e;
    vnode_t vn = NULL;
    struct emptyfs_mount *mntp;

    kassert_nonnull(mp);
    kassert_nonnull(vpp);
    kassert_nonnull(ctx);

    LOG_DBG("mp: %p vpp: %p %p", mp, vpp, *vpp);

    mntp = emptyfs_mount_from_mp(mp);
    e = get_root_vnode(mntp, &vn);
    /* under all circumstances we should set *vpp to maintain post-conditions */
    *vpp = vn;

    if (e == 0) {
        kassert_nonnull(*vpp);
    } else {
        kassert_null(*vpp);
    }

    LOG_DBG("vpp: %p %p", vpp, *vpp);

    return e;
}

/*
 * Called by VFS to get information about this file system
 *
 * @mp      the mount structure reference
 * @attr    describe the attributes requested and the place to stores the result
 * @ctx     context to authenticate for mount
 * @return  0 :. always success
 *
 * this implementation is trivial :. our file system attributes are static
 */
static int emptyfs_vfsop_getattr(
        struct mount *mp,
        struct vfs_attr *attr,
        vfs_context_t ctx)
{
    struct emptyfs_mount *mntp;

    kassert_nonnull(mp);
    kassert_nonnull(attr);
    kassert_nonnull(ctx);

    LOG_DBG("mp: %p attr: %p f_active: %#llx f_supported: %#llx",
                mp, attr, attr->f_active, attr->f_supported);

    mntp = emptyfs_mount_from_mp(mp);

    VFSATTR_RETURN(attr, f_objcount, mntp->attr.f_objcount);
    VFSATTR_RETURN(attr, f_filecount, mntp->attr.f_filecount);
    VFSATTR_RETURN(attr, f_dircount, mntp->attr.f_dircount);
    VFSATTR_RETURN(attr, f_maxobjcount, mntp->attr.f_maxobjcount);

    VFSATTR_RETURN(attr, f_bsize, mntp->attr.f_bsize);
    VFSATTR_RETURN(attr, f_iosize, mntp->attr.f_iosize);
    VFSATTR_RETURN(attr, f_blocks, mntp->attr.f_blocks);
    VFSATTR_RETURN(attr, f_bfree, mntp->attr.f_bfree);
    VFSATTR_RETURN(attr, f_bavail, mntp->attr.f_bavail);
    VFSATTR_RETURN(attr, f_bused, mntp->attr.f_bused);
    VFSATTR_RETURN(attr, f_files, mntp->attr.f_files);
    VFSATTR_RETURN(attr, f_ffree, mntp->attr.f_ffree);
    VFSATTR_RETURN(attr, f_fsid, mntp->attr.f_fsid);
    VFSATTR_RETURN(attr, f_owner, mntp->attr.f_owner);

    VFSATTR_RETURN(attr, f_capabilities, mntp->attr.f_capabilities);
    VFSATTR_RETURN(attr, f_attributes, mntp->attr.f_attributes);

    VFSATTR_RETURN(attr, f_create_time, mntp->attr.f_create_time);
    VFSATTR_RETURN(attr, f_modify_time, mntp->attr.f_modify_time);
    VFSATTR_RETURN(attr, f_access_time, mntp->attr.f_access_time);
    /* no support f_backup_time */

    VFSATTR_RETURN(attr, f_fssubtype, mntp->attr.f_fssubtype);

    if (VFSATTR_IS_ACTIVE(attr, f_vol_name)) {
        strncpy(attr->f_vol_name, mntp->attr.f_vol_name, EMPTYFS_VOLNAME_MAXLEN);
        attr->f_vol_name[EMPTYFS_VOLNAME_MAXLEN-1] = '\0';
        VFSATTR_SET_SUPPORTED(attr, f_vol_name);
    }

    /* no support f_signature */
    /* no support f_carbon_fsid */

    if (VFSATTR_IS_ACTIVE(attr, f_uuid)) {
        kassert(sizeof(mntp->attr.f_uuid) == sizeof(attr->f_uuid));
        bcopy(mntp->attr.f_uuid, attr->f_uuid, sizeof(attr->f_uuid));
        VFSATTR_SET_SUPPORTED(attr, f_uuid);
    }

    /* no support f_quota */
    /* no support f_reserved */

    LOG_DBG("f_active: %#llx f_supported: %#llx",
            attr->f_active, attr->f_supported);

    return 0;
}

