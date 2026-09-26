#ifndef MOCK_LIBUTIL_H
#define MOCK_LIBUTIL_H

#include <stddef.h>
#include <stdint.h>

#if __has_include(<libutil.h>) && defined(__MidnightBSD__)
#include <libutil.h>
#else

#define HN_AUTOSCALE 0x01
#define HN_DECIMAL 0x02
#define HN_IEC_PREFIXES 0x04

int humanize_number(char *buf, size_t len, int64_t bytes, const char *suffix, int scale, int flags);

#endif
#endif
