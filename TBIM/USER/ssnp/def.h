#ifndef DEF_H
#define DEF_H

#include "cpu.h"

#define SSNP_MIN(x,y) (((x)<(y))?(x):(y))

#if BUTE_ORDER == BIG_ENDIAN//大端不用改变
#define HTONS(x)  (x)
#define NTOHS(x)  (x)
#define HTONL(x)  (x)
#define NTONL(x)  (x)
#else
#define HTONS(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >>8))//16位
#define NTOHS(x)  HTONS(x)
#define HTONL(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000UL) >> 8) | (((x) & 0xff000000UL) >> 24))
#define NTONL(x) HTONL(x)
#endif


#endif
