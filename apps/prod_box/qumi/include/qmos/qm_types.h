#ifndef __QM_TYPES_H__
#define __QM_TYPES_H__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

//#define INCLUDE_STDINT


#ifndef INCLUDE_STDINT
//typedef char                int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
#endif

#ifdef CONFIG_CPU_8BIT
typedef long                int32_t;
typedef long long           int64_t;
typedef unsigned long       uint32_t;
typedef unsigned long long  uint64_t;
typedef double              float64_t;
#else
#ifndef INCLUDE_STDINT
typedef int                 int32_t;
typedef unsigned int        uint32_t;
#endif

typedef long long           int64_t;
typedef unsigned long long  uint64_t;
typedef float               float32_t;
typedef double              float64_t;
typedef unsigned char       bool_t;

#endif

/* boolean type definitions */
#define QM_TRUE                         1               /**< boolean true  */
#define QM_FALSE                        0               /**< boolean fails */

#ifndef NULL
#define NULL                ((void *) 0)
#endif

#define swab16(x)                                                           \
    ((uint16_t)(                                                            \
    (((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |                            \
    (((uint16_t)(x) & (uint16_t)0xff00U) >> 8) ))

#define swab32(x)                                                           \
    ((uint32_t)(                                                            \
    (((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |                      \
    (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8)  |                     \
    (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8)  |                     \
    (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ))

#define swab64(x)                                                           \
    ((uint64_t)(                                                            \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) |   \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) |   \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) |   \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) |   \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) |   \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |   \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) |   \
    (uint64_t)(((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56) ))


#ifdef CONFIG_CPU_BIGENDIAN
#define be64_to_cpu(x)      (x)
#define be32_to_cpu(x)      (x)
#define be16_to_cpu(x)      (x)
#define cpu_to_be64(x)      (x)
#define cpu_to_be32(x)      (x)
#define cpu_to_be16(x)      (x)
#else
#define be64_to_cpu(x)      swab64(x)
#define be32_to_cpu(x)      swab32(x)
#define be16_to_cpu(x)      swab16(x)
#define cpu_to_be64(x)      swab64(x)
#define cpu_to_be32(x)      swab32(x)
#define cpu_to_be16(x)      swab16(x)
#endif


#endif

