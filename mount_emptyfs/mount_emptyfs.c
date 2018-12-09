/*
 * Created 181208
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/param.h>
#include <sys/mount.h>
#include "emptyfs_mnt_args.h"

#define MOUNT_EMPTYFS_VERSION   "0.1"

#define LOG(fmt, ...)   printf(EMPTYFS_NAME ": " fmt "\n", ##__VA_ARGS__)
#ifdef DEBUG
#define LOG_DBG(fmt, ...)   LOG("[DBG] " fmt, ##__VA_ARGS__)
#else
#define LOG_DBG(fmt, ...)   ((void) 0, ##__VA_ARGS__)
#endif
#define LOG_ERR(fmt, ...)   LOG("[ERR] " fmt, ##__VA_ARGS__)

#define ASSERT_NONNULL(p)   assert(p != NULL)

static __dead2 void usage(char * __nonnull argv0)
{
    ASSERT_NONNULL(argv0);
    fprintf(stderr,
            "usage:\n\t"
            "%s [-d | -f] specrdev fsnode\n\t"
            "%s -v\n\n\t"
            "-d, --debug-mode   mount in debug mode(verbose output)\n\t"
            "-f, --force-fail   force mount failure\n\t"
            "-v, --version      print version\n\t"
            "-h, --help         print this help\n\t"
            "specrdev           special raw device\n\t"
            "fsnode             file-system node\n\n",
            basename(argv0), basename(argv0));
    exit(1);
}

static __dead2 void version(char * __nonnull argv0)
{
    ASSERT_NONNULL(argv0);
    fprintf(stderr,
            "%s version %s\n"
            "built date %s %s\n"
            "built with c++ inc %s\n\n",
            basename(argv0), MOUNT_EMPTYFS_VERSION,
            __DATE__, __TIME__,
            __VERSION__);
    exit(0);
}

static int do_mount(
        const char * __nonnull fspec,
        const char *__nonnull mp,
        uint32_t dbg_mode,
        uint32_t force_fail)
{
    int e;
    struct emptyfs_mnt_args mnt_args;
    char realmp[MAXPATHLEN];

    ASSERT_NONNULL(fspec);
    ASSERT_NONNULL(mp);

    /*
     * [sic]
     * We have to canonicalise the mount point :. o.w.
     *  umount(8) cannot unmount it by name
     */
    if (realpath(mp, realmp) == NULL) {
        e = -1;
        LOG_ERR("realpath(3) fail  fspec: %s mp: %s errno: %d",
                    fspec, mp, errno);
        goto out_exit;
    }

#ifndef KERNEL
    mnt_args.fspec = fspec;
#endif
    mnt_args.magic = EMPTYFS_MNTARG_MAGIC;
    mnt_args.dbg_mode = dbg_mode;
    mnt_args.force_fail = force_fail;

    e = mount(EMPTYFS_NAME, realmp, 0, &mnt_args);
    if (e == -1) {
        LOG_ERR("mount(2) fail  fspec: %s mp: %s errno: %d",
                    fspec, realmp, errno);
    }

out_exit:
    return e;
}

int main(int argc, char *argv[])
{
    int ch;
    int idx;
    int dbg_mode = 0;
    int force_fail = 0;
    struct option opt[] = {
        {"debug-mode", no_argument, &dbg_mode, 1},
        {"force-fail", no_argument, &force_fail, 1},
        {"version", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, no_argument, NULL, 0},
    };
    char *fspec;
    char *mp;

    while ((ch = getopt_long(argc, argv, "dfvh", opt, &idx)) != -1) {
        switch (ch) {
        case 'd':
            dbg_mode = 1;
            break;
        case 'f':
            force_fail = 1;
            break;
        case 'v':
            version(argv[0]);
        case 'h':
        case '?':
        default:
            usage(argv[0]);
        }
    }

    if (argc - optind != 2) usage(argv[0]);
    fspec = argv[optind];
    mp = argv[optind+1];

    LOG_DBG("dbg_mode: %d force_fail: %d fspec: %s mp: %s",
                dbg_mode, force_fail, fspec, mp);

    return do_mount(fspec, mp, dbg_mode, force_fail);
}

