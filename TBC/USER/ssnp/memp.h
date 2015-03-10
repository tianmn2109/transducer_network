#ifndef MEMP_H
#define MEMP_H

#include  "opt.h"


typedef enum
{
#define SSNP_MEMPOOL(name,num,size,desc) MEMP_##name,
#include "memp_std.h"
	MEMP_MAX
}memp_t;

void memp_init(void);
void* memp_alloc(memp_t type);
void memp_free(memp_t type,void* mem);

#endif
