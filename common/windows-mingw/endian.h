/* endian.h

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#include <stdint.h>
#include <sys/config.h>
#include "machine_endian.h"

/*#ifdef  __USE_BSD*/
# ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN __LITTLE_ENDIAN
# endif
# ifndef BIG_ENDIAN
#  define BIG_ENDIAN    __BIG_ENDIAN
# endif
# ifndef PDP_ENDIAN
#  define PDP_ENDIAN    __PDP_ENDIAN
# endif
# ifndef BYTE_ORDER
#  define BYTE_ORDER    __BYTE_ORDER
# endif
/*#endif*/

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define __LONG_LONG_PAIR(HI, LO) LO, HI
#elif __BYTE_ORDER == __BIG_ENDIAN
# define __LONG_LONG_PAIR(HI, LO) HI, LO
#endif

#if __BSD_VISIBLE

#include "bits_byteswap.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN

#define htobe16(x) __bswap_16(x)
#define htobe32(x) __bswap_32(x)
#define htobe64(x) __bswap_64(x)

#define be16toh(x) __bswap_16(x)
#define be32toh(x) __bswap_32(x)
#define be64toh(x) __bswap_64(x)

#define htole16(x) (x)
#define htole32(x) (x)
#define htole64(x) (x)

#define le16toh(x) (x)
#define le32toh(x) (x)
#define le64toh(x) (x)

#endif /*__BYTE_ORDER == __LITTLE_ENDIAN*/

#if __BYTE_ORDER == __BIG_ENDIAN

#define htobe16(x) (x)
#define htobe32(x) (x)
#define htobe64(x) (x)

#define be16toh(x) (x)
#define be32toh(x) (x)
#define be64toh(x) (x)

#define htole16(x) __bswap_16(x)
#define htole32(x) __bswap_32(x)
#define htole64(x) __bswap_64(x)

#define le16toh(x) __bswap_16(x)
#define le32toh(x) __bswap_32(x)
#define le64toh(x) __bswap_64(x)

#endif /*__BYTE_ORDER == __BIG_ENDIAN*/

#endif /*__BSD_VISIBLE*/

/* Alignment-agnostic encode/decode bytestream to/from little/big endian. */

static __inline uint16_t
be16dec(const void *pp)
{
    uint8_t const *p = (uint8_t const *)pp;

    return ((p[0] << 8) | p[1]);
}

static __inline uint32_t
be32dec(const void *pp)
{
    uint8_t const *p = (uint8_t const *)pp;

    return (((unsigned)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static __inline uint64_t
be64dec(const void *pp)
{
    uint8_t const *p = (uint8_t const *)pp;

    return (((uint64_t)be32dec(p) << 32) | be32dec(p + 4));
}

static __inline uint16_t
le16dec(const void *pp)
{
    uint8_t const *p = (uint8_t const *)pp;

    return ((p[1] << 8) | p[0]);
}

static __inline uint32_t
le32dec(const void *pp)
{
    uint8_t const *p = (uint8_t const *)pp;

    return (((unsigned)p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

static __inline uint64_t
le64dec(const void *pp)
{
    uint8_t const *p = (uint8_t const *)pp;

    return (((uint64_t)le32dec(p + 4) << 32) | le32dec(p));
}

static __inline void
be16enc(void *pp, uint16_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = (u >> 8) & 0xff;
    p[1] = u & 0xff;
}

static __inline void
be32enc(void *pp, uint32_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = (u >> 24) & 0xff;
    p[1] = (u >> 16) & 0xff;
    p[2] = (u >> 8) & 0xff;
    p[3] = u & 0xff;
}

static __inline void
be64enc(void *pp, uint64_t u)
{
    uint8_t *p = (uint8_t *)pp;

    be32enc(p, (uint32_t)(u >> 32));
    be32enc(p + 4, (uint32_t)(u & 0xffffffffU));
}

static __inline void
le16enc(void *pp, uint16_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = u & 0xff;
    p[1] = (u >> 8) & 0xff;
}

static __inline void
le32enc(void *pp, uint32_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = u & 0xff;
    p[1] = (u >> 8) & 0xff;
    p[2] = (u >> 16) & 0xff;
    p[3] = (u >> 24) & 0xff;
}

static __inline void
le64enc(void *pp, uint64_t u)
{
    uint8_t *p = (uint8_t *)pp;

    le32enc(p, (uint32_t)(u & 0xffffffffU));
    le32enc(p + 4, (uint32_t)(u >> 32));
}

#endif /*_ENDIAN_H_*/