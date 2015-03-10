
#include "sys.h"
#include "opt.h"
#include "mem.h"
#include "memp.h"
#include "pbuf.h"//����չ��#include "memp_std.h"�����е�sizeof(struct)��Ӧ�ð���������
#include "dp.h"
#include "sdp.h"
#include "tp.h"
#include "ssnp.h"
#include "timer.h"
#include "netbuf.h"
//������linux��slab������

#define MEMP_SIZE   0
#define MEMP_ALIGN_SIZE(x) (SSNP_MEM_ALIGN_SIZE(x))


struct memp
{
	struct memp* next;
};

static struct memp* memp_tab[MEMP_MAX];//ÿһ���ڴ�صĵ�һ�����õ�ַ

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
			memp->next=memp_tab[i];//ע��ÿһ���ڴ�ض���ֻ��һ���ڴ��ָ��NULL����˵������ڴ�ʹ�����memp_tab[type]��ָ���
			memp_tab[i]=memp;
			memp=(struct memp*)(void*)((u8t *)memp + MEMP_SIZE + memp_sizes[i]);
		}
	}
}

void* memp_alloc(memp_t type)
{
	struct memp* memp;
	SYS_ARCH_DECL_PROTECT(old_level);//ϵͳ��������

	SYS_ARCH_PROTECT(old_level);//����

	memp=memp_tab[type];
	if(memp!=NULL)
	{
		memp_tab[type]=memp->next;//�����һ���ڴ������ȥ��memp_tab[type]��ָ���
		memp=(struct memp*)(void*)((u8t*)memp+MEMP_SIZE);//�ڷ����ȥ��next��Ҳ�������ȥʹ���ˡ�
	}

	SYS_ARCH_UNPROTECT(old_level);
	
	return memp;//���û�п����ڴ����ômemp��ֵΪNULL
}

void memp_free(memp_t type,void* mem)
{	
	struct memp* memp;
	SYS_ARCH_DECL_PROTECT(old_level);//ϵͳ��������
	//SSNP_DEBUG_PRINT("memp_free():free pool memory\r\n");
	if(mem==NULL)
		return ;

	memp=(struct memp*)(void*)((u8t*)mem-MEMP_SIZE);

	SYS_ARCH_PROTECT(old_level);//����
	//SSNP_DEBUG_PRINT("memp_free():deleting memory\r\n");
	memp->next=memp_tab[type];//memp_tab[type]����������п��Կ���ʹ�õ��ڴ������ı�ͷ
	memp_tab[type]=memp;      //�����ڴ�ʱ�򵥵��޸�����ı�ͷ�Ϳ����ˣ�����Ҫ�����Ѿ������ȥ���ڴ�ĵ�ַ
                              //��һ���ֵ���Ϣ��ʹ�����������
	SYS_ARCH_UNPROTECT(old_level);
//	SSNP_DEBUG_PRINT("memp_free():delete memory done.\r\n");
}
