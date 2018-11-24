/*
 * Created 181110  lynnl
 */

#include <sys/systm.h>
#include <libkern/OSAtomic.h>
#include <mach-o/loader.h>
#include <sys/vnode.h>

#include "utils.h"

static void util_mstat(int opt)
{
    static volatile SInt64 cnt = 0;
    switch (opt) {
    case 0:
        if (OSDecrementAtomic64(&cnt) > 0) return;
        break;
    case 1:
        if (OSIncrementAtomic64(&cnt) >= 0) return;
        break;
    default:
        if (cnt == 0) return;
        break;
    }
#ifdef DEBUG
    panicf("FIXME: potential memleak  opt: %d cnt: %lld", opt, cnt);
#else
    LOG_BUG("FIXME: potential memleak  opt: %d cnt: %lld", opt, cnt);
#endif
}

void *util_malloc(size_t size, int flags)
{
    /* _MALLOC `type' parameter is a joke */
    void *addr = _MALLOC(size, M_TEMP, flags);
    if (likely(addr != NULL)) util_mstat(1);
    return addr;
}

/**
 * Poor replica of _REALLOC() in XNU
 *
 * /System/Library/Frameworks/Kernel.framework/Resources/SupportedKPIs-all-archs.txt
 * Listed all supported KPI  as it revealed
 *  _MALLOC and _FREE both supported  where _REALLOC not exported by Apple
 *
 * @param addr0     Address needed to reallocation
 * @param sz0       Original size of the buffer
 * @param sz1       New size
 * @param flags     Flags to malloc
 * @return          Return NULL on fail  O.w. new allocated buffer
 *
 * NOTE:
 *  You should generally avoid allocate zero-length(new buffer size)
 *  the behaviour is implementation-defined(_MALLOC return NULL in such case)
 *
 * See:
 *  xnu/bsd/kern/kern_malloc.c@_REALLOC
 *  wiki.sei.cmu.edu/confluence/display/c/MEM04-C.+Beware+of+zero-length+allocations
 */
static void *util_realloc2(void *addr0, size_t sz0, size_t sz1, int flags)
{
    void *addr1;

    /*
     * [sic] _REALLOC(NULL, ...) is equivalent to _MALLOC(...)
     * XXX  in such case, we require its original size must be zero
     */
    if (addr0 == NULL) {
        kassert(sz0 == 0);
        addr1 = _MALLOC(sz1, M_TEMP, flags);
        goto out_exit;
    }

    if (unlikely(sz1 == sz0)) {
        addr1 = addr0;
        goto out_exit;
    }

    addr1 = _MALLOC(sz1, M_TEMP, flags);
    if (unlikely(addr1 == NULL))
        goto out_exit;

    memcpy(addr1, addr0, MIN(sz0, sz1));
    _FREE(addr0, M_TEMP);

out_exit:
    return addr1;
}

void *util_realloc(void *addr0, size_t sz0, size_t sz1, int flags)
{
    void *addr1 = util_realloc2(addr0, sz0, sz1, flags);
    /*
     * If addr0 is notnull yet addr1 null  the reference shouldn't change
     *  since addr0 won't be free in such case
     */
    if (!addr0 && addr1) util_mstat(1);
    return addr1;
}

void util_mfree(void *addr)
{
    if (addr != NULL) util_mstat(0);
    _FREE(addr, M_TEMP);
}

/* XXX: call when all memory freed */
void util_massert(void)
{
    util_mstat(2);
}

/*
 * kcb stands for kernel callbacks  a global refcnt used in kext
 */
static int kcb(int opt)
{
    static volatile SInt i = 0;
    static struct timespec ts = {0, 1e+6};  /* 100ms */
    SInt rd;

    switch (opt) {
    case 0:
        do {
            if ((rd = i) < 0) break;
        } while (!OSCompareAndSwap(rd, rd + 1, &i));
        return rd;

    case 1:
        rd = OSDecrementAtomic(&i);
        kassert(rd > 0);
        return rd;

    case 2:
        do {
            while (i > 0) msleep((void *) &i, NULL, PWAIT, NULL, &ts);
        } while (!OSCompareAndSwap(0, (UInt32) -1, &i));
        break;

    default:
        panicf("invalid option  opt: %d", i);
    }

    return i;
}

/**
 * Increase refcnt of activated kext callbacks
 * @return      refcnt before get  -1 if failed to get(must check)
 */
int util_get_kcb(void)
{
    return kcb(0);
}

/**
 * Decrease refcnt of activated kext callbacks
 * @return      refcnt before put
 */
int util_put_kcb(void)
{
    return kcb(1);
}

/**
 * Invalidate further kcb operations(should call only once)
 * @return      always return -1
 */
int util_invalidate_kcb(void)
{
    return kcb(2);
}

#define UUID_STR_BUFSZ    37  /* EOS included */

/**
 * Extract UUID load command from a Mach-O address
 *
 * @addr    Mach-O starting address
 * @return  NULL if failed  o.w. a new allocated buffer
 *          You need to free the buffer explicitly by util_mfree
 */
char *util_vma_uuid(const vm_address_t addr)
{
    char *s = NULL;
    uint8_t *p = (void *) addr;
    struct mach_header *h = (struct mach_header *) p;
    struct load_command *lc;
    uint32_t i;
    uint8_t *u;

    kassert_nonnull(addr);

    if (h->magic == MH_MAGIC || h->magic == MH_CIGAM) {
        p += sizeof(struct mach_header);
    } else if (h->magic == MH_MAGIC_64 || h->magic == MH_CIGAM_64) {
        p += sizeof(struct mach_header_64);
    } else {
        goto out_bad;
    }

    for (i = 0; i < h->ncmds; i++, p += lc->cmdsize) {
        lc = (struct load_command *) p;
        if (lc->cmd == LC_UUID) {
            u = p + sizeof(*lc);
            s = util_malloc(UUID_STR_BUFSZ, M_NOWAIT);
            if (unlikely(s == NULL)) goto out_bad;

            (void) snprintf(s, UUID_STR_BUFSZ,
                    "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
                    "%02x%02x-%02x%02x%02x%02x%02x%02x",
                    u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7],
                    u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]);
            break;
        }
    }

out_bad:
    return s;
}

void format_uuid_string(const uuid_t u, uuid_string_t output)
{
    (void)
    snprintf(output, UUID_STR_BUFSZ,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7],
            u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]);
}

