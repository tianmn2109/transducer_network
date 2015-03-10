#ifndef MEM_H
#define MEM_H

#include "opt.h"

typedef u32t mem_size_t;         //#未完成#：这里应该是u16t还是u32t，将来会改动的地方
typedef u32t    mem_ptr_t;       //将来会改动的地方,对象地址




#define SSNP_MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT-1))
#define SSNP_MEM_ALIGN(addr) ((void *)(((mem_ptr_t)(addr) + MEM_ALIGNMENT - 1) & ~(mem_ptr_t)(MEM_ALIGNMENT-1)))


void mem_init(void);
void* mem_trim(void* mem,mem_size_t newsize);
void* mem_alloc(mem_size_t size);
void mem_free(void* mem);

#endif
