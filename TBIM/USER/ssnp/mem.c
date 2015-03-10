
#include "mem.h"
#include "sys.h"

#define MIN_SIZE   12    //�ڴ�������Сֵ
#define MIN_SIZE_ALIGNED     SSNP_MEM_ALIGN_SIZE(MIN_SIZE)//�������ڴ������Сֵ
#define SIZEOF_STRUCT_MEM    SSNP_MEM_ALIGN_SIZE(sizeof(struct mem))//�����Ľṹ����Ϣ��С
#define MEM_SIZE_ALIGNED     SSNP_MEM_ALIGN_SIZE(MEM_SIZE)

struct mem
{
	mem_size_t next;//ע�������nextָ��ֻ�������е�λ��
	mem_size_t prev;
	u8t used;
};

u8t RAM[MEM_SIZE_ALIGNED + (2*SIZEOF_STRUCT_MEM) + MEM_ALIGNMENT];//����һ����ĩβ�Ľṹ����end������һ��MEM_ALIGNMENTΪ��ͷָ��������ÿռ䲻�����
#define SSNP_RAM_HEAP_POINTER RAM

static u8t* ram;   //������ͷָ��
static struct mem* ram_end;//ĩβ���
static struct mem* ffree;  //��һ�������ڴ��ָ��

static sys_sem_t mem_sem; //�������ʱ���


#define SSNP_MEM_FREE_DECL_PROTECT()//�ڴ��ͷ�ʱ���ٽ���Դ��������
#define SSNP_MEM_FREE_PROTECT()    sys_arch_sem_wait(mem_sem, 0)
#define SSNP_MEM_FREE_UNPROTECT()  sys_sem_signal(mem_sem)

#define SSNP_MEM_ALLOC_DECL_PROTECT()//�ڴ����ʱ���ٽ���Դ��������
#define SSNP_MEM_ALLOC_PROTECT()
#define SSNP_MEM_ALLOC_UNPROTECT()


static void plug_holes(struct mem* mem)//�ϲ�memǰ����õ��ڴ��
{
	struct mem* nmem;
	struct mem* pmem;

	nmem=(struct mem*)(void*)&ram[mem->next];//��ǰ�ڴ�����һ��
	if(mem != nmem && nmem->used == 0 && (u8t *)nmem != (u8t *)ram_end)//���mem����һ���ڴ��û��ʹ�ò��Ҳ���ĩβ
	{
		if(ffree==nmem)
			ffree=mem;

		mem->next=nmem->next;//����һ�������ڴ��ϲ�����ǰ�ڴ����
		((struct mem*)(void*)&ram[nmem->next])->prev=(mem_size_t)((u8t*)mem-ram);//�������memָ��תΪ��С��λλ��
	}

	pmem=(struct mem*)(void*)&ram[mem->prev];//��ǰ�ڴ�����һ��
	if(mem != pmem && pmem->used == 0)//���mem������ǵ�һ���ڴ���أ�ע���ڳ�ʼ�������е�һ���ڴ���prevָ��ָ�����Լ�
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

	ram=(u8t*)SSNP_MEM_ALIGN(SSNP_RAM_HEAP_POINTER);//ramָ������Ķ��׵�ַ

	mem=(struct mem*)(void*)ram;
	mem->next=MEM_SIZE_ALIGNED;
	mem->prev=0;//��һ���ڵ�ָ���Լ�
	mem->used=0;

	ram_end=(struct mem*)(void*)&ram[MEM_SIZE_ALIGNED];
	ram_end->used=1;
	ram_end->next=MEM_SIZE_ALIGNED;//���һ���ڵ��next��prev��ָ���Լ�
	ram_end->prev=MEM_SIZE_ALIGNED;

	ffree=(struct mem*)(void*)ram;//���õ�һ������ָ����

	mem_sem=sys_sem_new(1);//������Դ����
}

void mem_free(void* rmem)
{
	struct mem* mem;
	SSNP_MEM_FREE_DECL_PROTECT();//ϵͳ������������

	if(rmem == NULL)
		return;
	if((u8t *)rmem < (u8t *)ram || (u8t *)rmem >= (u8t *)ram_end)//�߽���
		return;


	SSNP_MEM_FREE_PROTECT();//ϵͳ�ٽ���Դ������

	mem=(struct mem*)(void*)((u8t*)rmem - SIZEOF_STRUCT_MEM);
	mem->used=0;
	if(mem < ffree)
		ffree=mem;
	plug_holes(mem);//�ϲ������ڴ��

	SSNP_MEM_FREE_UNPROTECT();//�ͷ�
}

void* mem_trim(void* rmem,mem_size_t newsize)
{
	mem_size_t size;
	mem_size_t ptr;
	mem_size_t ptr2;
	struct mem* mem;
	struct mem* mem2;

	SSNP_MEM_FREE_DECL_PROTECT();//ϵͳ��������

	newsize=SSNP_MEM_ALIGN_SIZE(newsize);
	if(newsize < MIN_SIZE_ALIGNED)
		newsize=MIN_SIZE_ALIGNED;

	if(newsize > MEM_SIZE_ALIGNED)// ����С�ڴ������ʵ��һ��
		return NULL;

	if((u8t *)rmem < (u8t *)ram || (u8t *)rmem >= (u8t *)ram_end)
		return rmem;//������Χ��������

	mem=(struct mem*)(void*)((u8t*)rmem - SIZEOF_STRUCT_MEM);//�õ����ڴ��Ŀ�����Ϣ
	ptr=(mem_size_t)((u8t*)mem - ram);//���ڴ���뿪ͷ��ƫ����
	size=mem->next - ptr - SIZEOF_STRUCT_MEM;//��ǰ��Ŀ����ڴ��С

	if(newsize > size)
		return NULL;
	if(newsize == size)
		return rmem;

	SSNP_MEM_FREE_PROTECT();//ϵͳ�ٽ���Դ������
	mem2=(struct mem*)(void*)&ram[mem->next];//��ǰ�ڴ�����һ��
	if(mem2->used == 0)//��һ��û��ʹ�ÿ��Ժϲ�
	{
		mem_size_t next;
		next=mem2->next;//���ϲ����ڴ����һ���ڴ���λ��
		ptr2= ptr + SIZEOF_STRUCT_MEM +newsize;//��ǰ�ڴ�����λ��

		if(ffree == mem2)
			ffree=(struct mem*)(void*)&ram[ptr2];//��¼�µ�λ��

		mem2=(struct mem*)(void*)&ram[ptr2];//�µ�λ��
		mem2->used=0;
		mem2->next=next;//�ϲ�ʣ����ڴ�
		mem2->prev=ptr;

		mem->next=ptr2;//ָ���µ�λ��
		if(mem2->next != MEM_SIZE_ALIGNED)//��һ��λ�ò���ĩβ
			((struct mem*)(void*)&ram[mem2->next])->prev=ptr2;
	}
	else if(newsize + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED <= size)//�����ǰ�ڴ�����һ���ڴ������ʹ�ò���ʣ����ڴ�鳬������С�ɷ����ڴ����ô���ֳ������ں���ĺϲ�
	{
		ptr2=ptr + SIZEOF_STRUCT_MEM + newsize;//��ǰС�ռ��λ��
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
	else//ʣ��ռ�̫С��������
	{
	}
	SSNP_MEM_FREE_UNPROTECT();//�ͷ�

	return rmem;
}

void* mem_alloc(mem_size_t size)
{
	mem_size_t ptr;
	mem_size_t ptr2;
	struct mem* mem;
	struct mem* mem2;
	struct mem* i;

	SSNP_MEM_ALLOC_DECL_PROTECT();//�ڴ����ϵͳ��������

	if(size == 0)
		return NULL;

	size=SSNP_MEM_ALIGN_SIZE(size);//����ʵ�ʶ����ķ����С
	if(size < MIN_SIZE_ALIGNED)
		size=MIN_SIZE_ALIGNED;
	if(size > MEM_SIZE_ALIGNED)
		return NULL;

	sys_arch_sem_wait(mem_sem,0);
	SSNP_MEM_ALLOC_PROTECT();//��������
	for(ptr = (mem_size_t)((u8t*)ffree - ram); ptr < MEM_SIZE_ALIGNED - size; ptr = ((struct mem*)(void*)&ram[ptr])->next)
	{
		mem=(struct mem*)(void*)&ram[ptr];
		if((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size)//�ڴ�δ�ò��ҿռ���������
		{
			if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED))//�����ǰ�ռ䲻������Ҫ���һ���������һ����С�ɷ����ڴ浥Ԫ
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
			else//�������ڴ���ʣ��ռ䲻����������С���ڴ浥Ԫ
			{
				mem->used=1;
			}

			if(mem == ffree)
			{
				i=ffree;
				while(i->used && i!= ram_end)
					i=(struct mem*)(void*)&ram[i->next];
				ffree=i;//���������࣬û�б�Ҫʹ��i
			}

			SSNP_MEM_ALLOC_UNPROTECT();//����ǰҪ�����������������
			sys_sem_signal(mem_sem);
			return (u8t*)mem + SIZEOF_STRUCT_MEM;
		}
	}

    SSNP_MEM_ALLOC_UNPROTECT();//����
	sys_sem_signal(mem_sem);
	return NULL;
}
