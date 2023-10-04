/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <errno.h>

#include "libbluechi/common/common.h"
#include "procfs-util.h"

#define MEMINFO_BUF_SIZE 60
#define STAT_BUF_SIZE 512

#define BASE_DECIMAL 10

#define IDLE_INDEX 3
#define IOWAIT_INDEX 4
#define STEAL_INDEX 7

#define NSEC_PER_SEC 1000000000ULL

#define DIV_ROUND_UP(n, d) (((n) + (d) -1) / (d))

static uint64_t calc_gcd64(uint64_t a, uint64_t b) {
        while (b > 0) {
                uint64_t t = a % b;

                a = b;
                b = t;
        }

        return a;
}

int procfs_cpu_get_usage(uint64_t *ret) {
        long ticks_per_second = 0;
        uint64_t sum = 0, gcd = 0, a = 0, b = 0;
        _cleanup_fclose_ FILE *f = NULL;
        char buf[STAT_BUF_SIZE];
        const char *p = buf;
        int i = 0;

        assert(ret);

        f = fopen("/proc/stat", "re");
        if (!f) {
                return -errno;
        }

        if (!fgets(buf, sizeof(buf), f) || buf[0] != 'c' /* not "cpu" */) {
                return -EINVAL;
        }

        p += strcspn(p, " \t");

        while (p && *p != '\0') {
                char *endptr = NULL;
                uint64_t val = UINT64_MAX;

                val = strtoul(p, &endptr, BASE_DECIMAL);
                if ((errno == ERANGE && val == UINT64_MAX) || (errno != EAGAIN && errno != 0 && val == 0)) {
                        return -errno;
                }

                if (i != IDLE_INDEX && i != IOWAIT_INDEX && i != STEAL_INDEX) {
                        sum += val;
                }

                p = ++endptr;
                i++;
        }

        ticks_per_second = sysconf(_SC_CLK_TCK);
        if (ticks_per_second < 0) {
                return -errno;
        }
        assert(ticks_per_second > 0);

        /* Let's reduce this fraction before we apply it to avoid overflows when converting this to μsec */
        gcd = calc_gcd64(NSEC_PER_SEC, ticks_per_second);

        a = (uint64_t) NSEC_PER_SEC / gcd;
        b = (uint64_t) ticks_per_second / gcd;

        *ret = DIV_ROUND_UP(sum * a, b);
        return 0;
}

int procfs_memory_get(uint64_t *ret_total, uint64_t *ret_used) {
        uint64_t mem_total = UINT64_MAX, mem_available = UINT64_MAX;
        _cleanup_fclose_ FILE *f = NULL;
        char buf[MEMINFO_BUF_SIZE];

        f = fopen("/proc/meminfo", "re");
        if (!f) {
                return -errno;
        }

        while (fgets(buf, sizeof(buf), f) != NULL) {
                char *c = strchr(buf, ':');
                if (!c) {
                        continue;
                }
                *c = '\0';

                if (streq(buf, "MemTotal")) {
                        mem_total = strtoul(c + 1, NULL, BASE_DECIMAL);
                        if ((errno == ERANGE && mem_total == UINT64_MAX) ||
                            (errno != EAGAIN && errno != 0 && mem_total == 0)) {
                                return -errno;
                        }
                        continue;
                }

                if (streq(buf, "MemAvailable")) {
                        mem_available = strtoul(c + 1, NULL, BASE_DECIMAL);
                        if ((errno == ERANGE && mem_available == UINT64_MAX) ||
                            (errno != EAGAIN && errno != 0 && mem_available == 0)) {
                                return -errno;
                        }
                        continue;
                }

                if (mem_total != UINT64_MAX && mem_available != UINT64_MAX) {
                        break;
                }
        }

        if (mem_available > mem_total) {
                return -EINVAL;
        }

        if (ret_total) {
                *ret_total = mem_total;
        }

        if (ret_used) {
                *ret_used = mem_total - mem_available;
        }

        return 0;
}
