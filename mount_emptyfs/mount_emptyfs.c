/*
 * Created 181208
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>

#define MOUNT_EMPTYFS_VERSION   "0.1"

static __dead2 void usage(char *argv0)
{
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

static __dead2 void version(char *argv0)
{
    fprintf(stderr,
            "%s v%s\n"
            "built date %s %s\n"
            "built with c++ inc %s\n\n",
            basename(argv0), MOUNT_EMPTYFS_VERSION,
            __DATE__, __TIME__,
            __VERSION__);
    exit(0);
}

int main(int argc, char *argv[])
{
    int e = 0;
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

    puts("TODO");

    return e;
}

