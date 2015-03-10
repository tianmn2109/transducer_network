
#include "mem.h"
#include "sys.h"

#define MIN_SIZE   12    //内存分配的最小值
#define MIN_SIZE_ALIGNED     SSNP_MEM_ALIGN_SIZE(MIN_SIZE)//对齐后的内存分配最小值
#define SIZEOF_STRUCT_MEM    SSNP_MEM_ALIGN_SIZE(sizeof(struct mem))//对齐后的结构体信息大小
#define MEM_SIZE_ALIGNED     SSNP_MEM_ALIGN_SIZE(MEM_SIZE)

struct mem
{
	mem_size_t next;//注意这里的next指针只是数组中的位置
	mem_size_t prev;
	u8t used;
};

u8t RAM[MEM_SIZE_ALIGNED + (2*SIZEOF_STRUCT_MEM) + MEM_ALIGNMENT];//其中一个在末尾的结构体标记end，还有一个MEM_ALIGNMENT为了头指针对齐后可用空间不会减少
#define SSNP_RAM_HEAP_POINTER RAM

static u8t* ram;   //对齐后的头指针
static struct mem* ram_end;//末尾标记
static struct mem* ffree;  //第一个可用内存块指针

static sys_sem_t mem_sem; //并发访问保护


#define SSNP_MEM_FREE_DECL_PROTECT()//内存释放时的临界资源保护函数
#define SSNP_MEM_FREE_PROTECT()    sys_arch_sem_wait(mem_sem, 0)
#define SSNP_MEM_FREE_UNPROTECT()  sys_sem_signal(mem_sem)

#define SSNP_MEM_ALLOC_DECL_PROTECT()//内存分配时的临界资源保护函数
#define SSNP_MEM_ALLOC_PROTECT()
#define SSNP_MEM_ALLOC_UNPROTECT()


static void plug_holes(struct mem* mem)//合并mem前后可用的内存块
{
	struct mem* nmem;
	struct mem* pmem;

	nmem=(struct mem*)(void*)&ram[mem->next];//当前内存块的下一块
	if(mem != nmem && nmem->used == 0 && (u8t *)nmem != (u8t *)ram_end)//如果mem的下一个内存块没有使用并且不是末尾
	{
		if(ffree==nmem)
			ffree=mem;

		mem->next=nmem->next;//将下一个可用内存块合并到当前内存块上
		((struct mem*)(void*)&ram[nmem->next])->prev=(mem_size_t)((u8t*)mem-ram);//将这里的mem指针转为最小单位位置
	}

	pmem=(struct mem*)(void*)&ram[mem->prev];//当前内存块的上一块
	if(mem != pmem && pmem->used == 0)//如果mem本身就是第一个内存块呢？注意在初始化函数中第一个内存块的prev指针指向了自己
	{
		if(ffree == mem)
			ffree=pmem;

		pmem->next = mem->next;
		((struct mem *)(void *)&ram[mem->next])->prev = (mem_size_t)((u8t *)pmem - ram);
	}
}

void mem_init()
{
	struct mem* mem;

	ram=(u8t*)SSNP_MEM_ALIGN(SSNP_RAM_HEAP_POINTER);//ram指向对齐后的堆首地址

	mem=(struct mem*)(void*)ram;
	mem->next=MEM_SIZE_ALIGNED;
	mem->prev=0;//第一个节点指向自己
	mem->used=0;

	ram_end=(struct mem*)(void*)&ram[MEM_SIZE_ALIGNED];
	ram_end->used=1;
	ram_end->next=MEM_SIZE_ALIGNED;//最后一个节点的next和prev都指向自己
	ram_end->prev=MEM_SIZE_ALIGNED;

	ffree=(struct mem*)(void*)ram;//设置第一个可用指针标记

	mem_sem=sys_sem_new(1);//共享资源保护
}

void mem_free(void* rmem)
{
	struct mem* mem;
	SSNP_MEM_FREE_DECL_PROTECT();//系统保护函数声明

	if(rmem == NULL)
		return;
	if((u8t *)rmem < (u8t *)ram || (u8t *)rmem >= (u8t *)ram_end)//边界检测
		return;


	SSNP_MEM_FREE_PROTECT();//系统临界资源，加锁

	mem=(struct mem*)(void*)((u8t*)rmem - SIZEOF_STRUCT_MEM);
	mem->used=0;
	if(mem < ffree)
		ffree=mem;
	plug_holes(mem);//合并空闲内存块

	SSNP_MEM_FREE_UNPROTECT();//释放
}

void* mem_trim(void* rmem,mem_size_t newsize)
{
	mem_size_t size;
	mem_size_t ptr;
	mem_size_t ptr2;
	struct mem* mem;
	struct mem* mem2;

	SSNP_MEM_FREE_DECL_PROTECT();//系统保护声明

	newsize=SSNP_MEM_ALIGN_SIZE(newsize);
	if(newsize < MIN_SIZE_ALIGNED)
		newsize=MIN_SIZE_ALIGNED;

	if(newsize > MEM_SIZE_ALIGNED)// 最大大小内存可用吗，实验一下
		return NULL;

	if((u8t *)rmem < (u8t *)ram || (u8t *)rmem >= (u8t *)ram_end)
		return rmem;//超出范围不作处理

	mem=(struct mem*)(void*)((u8t*)rmem - SIZEOF_STRUCT_MEM);//得到此内存块的控制信息
	ptr=(mem_size_t)((u8t*)mem - ram);//此内存块与开头的偏移量
	size=mem->next - ptr - SIZEOF_STRUCT_MEM;//当前块的可用内存大小

	if(newsize > size)
		return NULL;
	if(newsize == size)
		return rmem;

	SSNP_MEM_FREE_PROTECT();//系统临界资源，加锁
	mem2=(struct mem*)(void*)&ram[mem->next];//当前内存块的下一块
	if(mem2->used == 0)//下一块没有使用可以合并
	{
		mem_size_t next;
		next=mem2->next;//被合并的内存的下一个内存块的位置
		ptr2= ptr + SIZEOF_STRUCT_MEM +newsize;//当前内存块的新位置

		if(ffree == mem2)
			ffree=(struct mem*)(void*)&ram[ptr2];//记录新的位置

		mem2=(struct mem*)(void*)&ram[ptr2];//新的位置
		mem2->used=0;
		mem2->next=next;//合并剩余的内存
		mem2->prev=ptr;

		mem->next=ptr2;//指向新的位置
		if(mem2->next != MEM_SIZE_ALIGNED)//下一个位置不是末尾
			((struct mem*)(void*)&ram[mem2->next])->prev=ptr2;
	}
	else if(newsize + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED <= size)//如果当前内存块的下一个内存块正在使用并且剩余的内存块超过了最小可分配内存块那么划分出来便于后面的合并
	{
		ptr2=ptr + SIZEOF_STRUCT_MEM + newsize;//当前小空间的位置
		mem2= (struct mem*)(void*)&ram[ptr2];
		if(mem2 < ffree)
			ffree=mem2;
		mem2->used=0;
		mem2->next=mem->next;
		mem2->prev=ptr;

		mem->next=ptr2;
		if(mem2->next != MEM_SIZE_ALIGNED)
			((struct mem *)(void *)&ram[mem2->next])->prev = ptr2;
	}
	else//剩余空间太小不作处理
	{
	}
	SSNP_MEM_FREE_UNPROTECT();//释放

	return rmem;
}

void* mem_alloc(mem_size_t size)
{
	mem_size_t ptr;
	mem_size_t ptr2;
	struct mem* mem;
	struct mem* mem2;
	struct mem* i;

	SSNP_MEM_ALLOC_DECL_PROTECT();//内存分配系统保护声明

	if(size == 0)
		return NULL;

	size=SSNP_MEM_ALIGN_SIZE(size);//计算实际对齐后的分配大小
	if(size < MIN_SIZE_ALIGNED)
		size=MIN_SIZE_ALIGNED;
	if(size > MEM_SIZE_ALIGNED)
		return NULL;

	sys_arch_sem_wait(mem_sem,0);
	SSNP_MEM_ALLOC_PROTECT();//加锁保护
	for(ptr = (mem_size_t)((u8t*)ffree - ram); ptr < MEM_SIZE_ALIGNED - size; ptr = ((struct mem*)(void*)&ram[ptr])->next)
	{
		mem=(struct mem*)(void*)&ram[ptr];
		if((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size)//内存未用并且空间满足需求
		{
			if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED))//如果当前空间不但满足要求并且还可以容纳一个最小可分配内存单元
			{
				ptr2=ptr + SIZEOF_STRUCT_MEM + size;
				mem2=(struct mem*)(void*)&ram[ptr2];
				mem2->used=0;
				mem2->next=mem->next;
				mem2->prev=ptr;

				mem->next=ptr2;
				mem->used=1;

				if(mem2->next != MEM_SIZE_ALIGNED)
					((struct mem *)(void *)&ram[mem2->next])->prev = ptr2;
			}
			else//分配完内存后的剩余空间不足以容纳最小的内存单元
			{
				mem->used=1;
			}

			if(mem == ffree)
			{
				i=ffree;
				while(i->used && i!= ram_end)
					i=(struct mem*)(void*)&ram[i->next];
				ffree=i;//这里有冗余，没有必要使用i
			}

			SSNP_MEM_ALLOC_UNPROTECT();//返回前要解锁，否则这里出错
			sys_sem_signal(mem_sem);
			return (u8t*)mem + SIZEOF_STRUCT_MEM;
		}
	}

    SSNP_MEM_ALLOC_UNPROTECT();//解锁
	sys_sem_signal(mem_sem);
	return NULL;
}
