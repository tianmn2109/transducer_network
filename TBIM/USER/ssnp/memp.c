
#include "sys.h"
#include "opt.h"
#include "mem.h"
#include "memp.h"
#include "pbuf.h"//编译展开#include "memp_std.h"后所有的sizeof(struct)都应该包含在这里
#include "dp.h"
#include "sdp.h"
#include "tp.h"
#include "ssnp.h"
#include "timer.h"
#include "netbuf.h"
//类似于linux的slab分配器

#define MEMP_SIZE   0
#define MEMP_ALIGN_SIZE(x) (SSNP_MEM_ALIGN_SIZE(x))


struct memp
{
	struct memp* next;
};

static struct memp* memp_tab[MEMP_MAX];//每一类内存池的第一个可用地址

static const u16t memp_sizes[MEMP_MAX]={
#define SSNP_MEMPOOL(name,num,size,desc) SSNP_MEM_ALIGN_SIZE(size),
#include "memp_std.h"
};

static const u16t memp_num[MEMP_MAX]={
#define SSNP_MEMPOOL(name,num,size,desc)  (num),
#include "memp_std.h"
};

static u8t memp_memory[MEM_ALIGNMENT -1
#define SSNP_MEMPOOL(name,num,size,desc) + ( (num) * (MEMP_SIZE + MEMP_ALIGN_SIZE(size) ) )
#include "memp_std.h"
];

void memp_init()
{
	struct memp* memp;
	u16t i;
	u16t j;
	
	memp=(struct memp*)SSNP_MEM_ALIGN(memp_memory);

	for(i=0;i<MEMP_MAX;i++)
	{
		memp_tab[i]=NULL;
		for(j=0;j<memp_num[i];j++)
		{
			memp->next=memp_tab[i];//注意每一类内存池都会只有一个内存块指向NULL，因此当此类内存使用完后memp_tab[type]会指向空
			memp_tab[i]=memp;
			memp=(struct memp*)(void*)((u8t *)memp + MEMP_SIZE + memp_sizes[i]);
		}
	}
}

void* memp_alloc(memp_t type)
{
	struct memp* memp;
	SYS_ARCH_DECL_PROTECT(old_level);//系统保护声明

	SYS_ARCH_PROTECT(old_level);//加锁

	memp=memp_tab[type];
	if(memp!=NULL)
	{
		memp_tab[type]=memp->next;//当最后一个内存块分配出去后，memp_tab[type]会指向空
		memp=(struct memp*)(void*)((u8t*)memp+MEMP_SIZE);//在分配出去后，next域也被分配出去使用了。
	}

	SYS_ARCH_UNPROTECT(old_level);
	
	return memp;//如果没有可用内存块那么memp的值为NULL
}

void memp_free(memp_t type,void* mem)
{	
	struct memp* memp;
	SYS_ARCH_DECL_PROTECT(old_level);//系统保护声明
	//SSNP_DEBUG_PRINT("memp_free():free pool memory\r\n");
	if(mem==NULL)
		return ;

	memp=(struct memp*)(void*)((u8t*)mem-MEMP_SIZE);

	SYS_ARCH_PROTECT(old_level);//加锁
	//SSNP_DEBUG_PRINT("memp_free():deleting memory\r\n");
	memp->next=memp_tab[type];//memp_tab[type]保存的是所有可以可以使用的内存块链表的表头
	memp_tab[type]=memp;      //回收内存时简单的修改链表的表头就可以了，不需要保存已经分配出去的内存的地址
                              //这一部分的信息是使用者来管理的
	SYS_ARCH_UNPROTECT(old_level);
//	SSNP_DEBUG_PRINT("memp_free():delete memory done.\r\n");
}
