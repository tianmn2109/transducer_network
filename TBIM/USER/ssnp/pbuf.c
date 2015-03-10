#include "sys.h"
#include "opt.h"
#include "pbuf.h"
#include "mem.h"
#include "memp.h"
#include "def.h"
#include "debug.h"

#define SIZEOF_STRUCT_PBUF SSNP_MEM_ALIGN_SIZE(sizeof(struct pbuf))
#define PBUF_POOL_BUFSIZE_ALIGNED SSNP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE)//网络中最大的数据大小


/***********************************************************************************
 *
 *  因为不需要考虑trigger，因此这里是考虑dp和sdp，而dp和sdp都属于网络和传输层
 *  
 *　　
 ***********************************************************************************/
struct pbuf* pbuf_alloc(pbuf_layer layer,u16t length,pbuf_type type)
{
	struct pbuf* p;
	struct pbuf* q;
	struct pbuf* r;
	u16t offset;
	s32t rem_len;

	switch(layer)//#未完成#：这里的协议层次和协议头部大小都有问题，到后面修改
	{
	case PBUF_TRANSPORT:
		{
			offset=PBUF_LINK_HLEN+PBUF_IP_AND_TRANSPORT_HLEN;//注意这里只有一种就是dp
			break;
		}
	case PBUF_IP:
		{
			offset=PBUF_LINK_HLEN+PBUF_IP_HLEN;
			break;
		}
	case PBUF_LINK:
		{
			offset=PBUF_LINK_HLEN;
			break;
		}
	case PBUF_RAW:
		{
			offset=0;
			break;
		}
	default: return NULL;
	}

	switch(type)
	{
	case PBUF_POOL:
		{
			p=(struct pbuf*)memp_alloc(MEMP_PBUF_POOL);//注意PBUF_POOL类型只在底层驱动收到数据包后使用目的是为了快速处理加快中断处理程序的执行速度，
			                                           //因此MEMP_PBUF_POOL类型的内存池一定要足够大，大到可以容纳传感器网络中一次发送接收最大的数据单元
			                                           //注意这里分配的大小包括了pbuf的大小
			if(p == NULL)
			{
			//	SSNP_DEBUG_PRINT("pbuf_alloc(): pbuf p is NULL.\r\n");
				return NULL;
			}

			/*
			 *  注意唯一使用pbuf_pool类型的内存分配是在底层驱动接收数据帧，这里的type参数是PBUF_RAW=0，因此下面的计算就正确了。
			 *  #为什么#:这里的MEMP_PBUF_POOL的含义到底是怎样的？
			 */
			p->type=type;
			p->next=NULL;
			p->data=SSNP_MEM_ALIGN((void*)((u8t*)p + SIZEOF_STRUCT_PBUF + offset));//#为什么#：为什么这里要跳过offset？为什么这里不是LWIP_MEM_ALIGN_SIZE(offset)
			p->tot_len=length;
			p->len=SSNP_MIN(length,PBUF_POOL_BUFSIZE_ALIGNED-SSNP_MEM_ALIGN_SIZE(offset));//注意PBUF_POOL_BUFSIZE_ALIGNED只是最大网络数据的大小没有包括pbuf，因此减去offset也就表示可以携带的数据
			 //#为什么#：这里的最大数据包是什么含义？如果是最大那么直接使用一个内存池不就可以了为什么还要分段？

			p->ref=1;//#为什么#：为什么p没有设置flag位。

			r=p;
			rem_len=length-p->len;
			while(rem_len>0)
			{
				q=(struct pbuf*)memp_alloc(MEMP_PBUF_POOL);
				if(q==NULL)
				{
					SSNP_DEBUG_PRINT("pbuf_alloc(): pbuf q is NULL.\r\n");
					pbuf_free(q);
					return NULL;
				}
				q->type=type;
				q->flags=0;
				q->next=NULL;
				r->next=q;
				q->tot_len=(u16t)rem_len;//#为什么#：这里要转换为u16t
				q->len=SSNP_MIN((u16t)rem_len,PBUF_POOL_BUFSIZE_ALIGNED);
				q->data=(void*)((u8t*)q+SIZEOF_STRUCT_PBUF);
				q->ref=1;

				rem_len-=q->len;
				r=q;
			}
			break;
		}
	case PBUF_RAM:
		{
			p=(struct pbuf*)mem_alloc(SSNP_MEM_ALIGN_SIZE(SIZEOF_STRUCT_PBUF + offset) + SSNP_MEM_ALIGN_SIZE(length));//操作系统分配的内存由于对齐原因总是多分配一点
			if(p==NULL)
				return NULL;

			p->data=SSNP_MEM_ALIGN((void*)((u8t*)p+SIZEOF_STRUCT_PBUF+offset));
			p->tot_len=length;
			p->len=length;
			p->next=NULL;
			p->type=type;

			break;
		}
	case PBUF_ROM:
	case PBUF_REF:
		{
			p=(struct pbuf*)memp_alloc(MEMP_PBUF);
			if(p==NULL)
				return NULL;

			p->data=NULL;
			p->tot_len=length;
			p->len=length;
			p->next=NULL;
			p->type=type;

			break;
		}
	default:return NULL;
	}

	p->ref=1;
	p->flags=0;

	return p;
}

//将pbuf减少到new_len
void pbuf_realloc(struct pbuf* p,u16t new_len)
{
	struct pbuf* q;
	u16t rem_len;
	s32t grow;

	if(new_len>=p->tot_len)
		return ;

	grow=new_len-p->tot_len;
	rem_len=new_len;
	q=p;

	while(rem_len>q->len)
	{
		rem_len-=q->len;
		q->tot_len+=(u16t)grow;//注意这里的加法实际上是减法的效果
		q=q->next;
	}

	if((q->type==PBUF_RAM)&&(rem_len!=q->len))
	{
		q=(struct pbuf*)mem_trim(q,(u16t)((u8t*)q->data-(u8t*)q)+rem_len);
	}
	q->len=rem_len;
	q->tot_len=q->len;//最后一个节点

	if(q->next!=NULL)
		pbuf_free(q->next);//释放掉后面所有的内存
	q->next=NULL;
}

//#为什么#：这里到使用时在看
u8t pbuf_header(struct pbuf* p,s16t header_size_increment)
{
	u16t type;
	void* data;
	u16t increment_magnitude;

	if(p==NULL || header_size_increment==0)
		return 0;

	if(header_size_increment<0)
		increment_magnitude=-header_size_increment;
	else
		increment_magnitude=header_size_increment;

	type=p->type;
	data=p->data;

	if(type==PBUF_RAM || type==PBUF_POOL)//这两种类型是将data指针向上移动
	{
		p->data=(u8t*)p->data - header_size_increment;//#为什么#：
		if((u8t*)p->data < (u8t*)p+SIZEOF_STRUCT_PBUF)
		{
			p->data=data;
			return 1;
		}
	}
	else if(type==PBUF_REF || type==PBUF_ROM)//这两种类型是将指针向下移动
	{
		if((header_size_increment<0)&&(increment_magnitude<=p->len))
			p->data=(u8t*)p->data - header_size_increment;
		else
			return 1;
	}
	else
	{
		return 1;
	}
	
	p->len+=header_size_increment;
	p->tot_len+=header_size_increment;

	return 0;
}

u8t pbuf_free(struct pbuf* p)
{
	u16t type;
	struct pbuf* q;
	u8t count;

	if(p==NULL)
		return 0;
	//printf("pbuf_free():here\r\n");
	count=0;
	while(p!=NULL)
	{
		u16t ref;
		SYS_ARCH_DECL_PROTECT(old_level);//系统保护声明
	//	printf("pbuf_free():minuss ref num.\r\n");
		SYS_ARCH_PROTECT(old_level);//加锁保护
		ref= --(p->ref);   //printf("pbuf_free():minusing ref num.\r\n");
		SYS_ARCH_UNPROTECT(old_level);//解锁
	///	printf("pbuf_free():minus ref num done.\r\n");
		if(ref == 0)
		{
			q=p->next;
			type=p->type;
			//printf("pbuf_free():there is free mem.\r\n");
			if(type == PBUF_POOL)
				memp_free(MEMP_PBUF_POOL,p);//这里没有必要保护，这在memp_free()中实现，只负责最重要的部分
			else if(type == PBUF_ROM || type == PBUF_REF)
				memp_free(MEMP_PBUF,p);
			else
				mem_free(p);

			count++;
			p=q;
		}
		else
			p=NULL;//此内存还有其他引用
	}

	return count;
}

u8t pbuf_clen(struct pbuf* p)
{
	u8t len;
	len=0;
	while(p!=NULL)
	{
		++len;
		p=p->next;
	}

	return len;
}

void pbuf_ref(struct pbuf* p)
{	   
	SYS_ARCH_DECL_PROTECT(old_level);//系统保护声明
	 //SSNP_DEBUG_PRINT("pbuf_ref(): here\r\n");
	if(p!=NULL)//#为什么#：这里需要保护而后面的不需要？
	{		
		SYS_ARCH_PROTECT(old_level);//加锁保护
		 //SSNP_DEBUG_PRINT("pbuf_ref(): add ref\r\n");
		++(p->ref);	  //SSNP_DEBUG_PRINT("pbuf_ref(): add ref ok\r\n");
		SYS_ARCH_UNPROTECT(old_level);//解锁
	}
}

void pbuf_cat(struct pbuf* head,struct pbuf* tail)
{
	struct pbuf* p;
	 //SSNP_DEBUG_PRINT("pbuf_cat(): here\r\n");
	for(p=head; p->next!=NULL; p=p->next)
		p->tot_len+=tail->tot_len;
	 //SSNP_DEBUG_PRINT("pbuf_cat(): 1\r\n");
	p->tot_len+=tail->tot_len;//指向最后一个内存块
	//SSNP_DEBUG_PRINT("pbuf_cat(): 2\r\n");
	p->next=tail;
	//SSNP_DEBUG_PRINT("pbuf_cat(): 3\r\n");
	//p现在引用了tail，应该增加对tail的引用，但是使用此函数时调用者会释放掉对tail的引用，
	//因此这里不需要进行处理。
}

//#为什么#：如何使用
void pbuf_chain(struct pbuf* head,struct pbuf* tail)
{  //SSNP_DEBUG_PRINT("pbuf_chain(): here\r\n");
	pbuf_cat(head,tail);   //SSNP_DEBUG_PRINT("pbuf_chain(): cat\r\n");
	pbuf_ref(tail);	 //SSNP_DEBUG_PRINT("pbuf_chain(): ref\r\n");
}


struct pbuf* pbuf_dechain(struct pbuf* p)
{
	struct pbuf* q;
	u8t tail_gone=1;

	q=p->next;
	if(q!=NULL)
	{
		q->tot_len=p->tot_len-p->len;//这个本来就应该成立
		p->next=NULL;
		p->tot_len=p->len;

		tail_gone=pbuf_free(q);
	}

	return ((tail_gone>0)? NULL:q);//后面的如果没有引用被删除掉了就返回NULL，否则还有引用则返回
}

//#为什么#：
err_t pbuf_copy(struct pbuf* p_to,struct pbuf* p_from)
{
	u16t offset_to;
	u16t offset_from;
	u16t len;

	offset_to=0;//已经copy的
	offset_from=0;//已经copy的

	SSNP_ERROR("pbuf_copy:target not big enough to hold source\n",((p_to!=NULL)&&(p_from!=NULL)&&(p_to->tot_len>=p_from->tot_len)),return ERR_ARGUMENT;);

	do
	{
		if((p_to->len - offset_to)>=(p_from->len - offset_to))
		{
			len=p_from->len - offset_from;
		}
		else
		{
			len=p_to->len - offset_to;
		}

		MEMCPY((u8t*)p_to->data + offset_to,(u8t*)p_from->data + offset_from, len);
		offset_to+=len;
		offset_from+=len;
		SSNP_ASSERT("offset_to<=p_to->len\n",offset_to<=p_to->len); 
		SSNP_ASSERT("offset_from<=p_from->len\n",offset_from<=p_from->len);

		if(offset_from >= p_from->len)
		{
			offset_from=0;
			p_from=p_from->next;
		}
		if(offset_to == p_to->len)
		{
			offset_to=0;
			p_to=p_to->next;
			SSNP_ERROR("p_to != NULL\n", (p_to != NULL) || (p_from == NULL) , return ERR_ARGUMENT;);
		}

		if((p_from!=NULL) && (p_from->len == p_from->tot_len))
			SSNP_ERROR("pbuf_copy():does not allow packet queues!\n",(p_from->next==NULL),return ERR_VALUE);
		if((p_to!=NULL)&& (p_to->len ==  p_to->tot_len))
			SSNP_ERROR("pbuf_copy():does not allow packet queues!\n",(p_to->next==NULL),return ERR_VALUE);

	}while(p_from);

	return ERR_OK;//删掉
}

//向应用层提供的缓冲区copy数据，参数offset是开始的位置
u16t pbuf_copy_partical(struct pbuf* buf,void* dataptr,u16t len,u16t offset)
{
	struct pbuf* p;
	u16t left;
	u16t buf_copy_len;
	u16t copied_total;

	copied_total=0;
	left=0;

	SSNP_ERROR("pbuf_copy_partial(): invalid buf\n", (buf != NULL), return 0;);
    SSNP_ERROR("pbuf_copy_partial(): invalid dataptr\n", (dataptr != NULL), return 0;);

	if((buf==NULL)||(dataptr==NULL))
		return 0;

	for(p=buf; len!=0&&p!=NULL;p=p->next)
	{
		if((offset!=0)&&(offset>=p->len))
		{
			offset-=p->len;
		}
		else
		{
			buf_copy_len=p->len-offset;
			if(buf_copy_len>len)//如果过多那么只copy要求的部分就可以了
				buf_copy_len=len;
			MEMCPY(&((char*)dataptr)[left],&((char*)p->data)[offset],buf_copy_len);
			copied_total+=buf_copy_len;
			left+=buf_copy_len;
			len-=buf_copy_len;
			offset=0;
		}
	}
	return copied_total;
}

//将应用层数据copy到puf中。
err_t pbuf_take(struct pbuf* buf,const void* dataptr,u16t len)
{
	struct pbuf* p;
	u16t buf_copy_len;
	u16t total_copy_len;
	u16t copied_total;

	total_copy_len=len;
	copied_total=0;

    SSNP_ERROR("pbuf_take(): invalid buf\n", (buf != NULL), return 0;);
    SSNP_ERROR("pbuf_take(): invalid dataptr\n", (dataptr != NULL), return 0;);

    if ((buf == NULL) || (dataptr == NULL) || (buf->tot_len < len)) 
		return ERR_ARGUMENT;

	for(p=buf; total_copy_len!=0;p=p->next)
	{
		SSNP_ASSERT("pbuf_take():invalid pbuf\n",p!=NULL);
		buf_copy_len=total_copy_len;
		if(buf_copy_len > p->len)
			buf_copy_len=p->len;
		MEMCPY(p->data,&((char*)dataptr)[copied_total],buf_copy_len);
		total_copy_len-=buf_copy_len;
		copied_total+=buf_copy_len;
	}
	SSNP_ASSERT("did not copy all data\n",(total_copy_len==0&&copied_total==len));
	return ERR_OK;
}


struct pbuf* pbuf_coalesce(struct pbuf* p,pbuf_layer layer)
{
	struct pbuf* q;
	err_t err;

	if(p->next == NULL)
		return p;

	q=pbuf_alloc(layer,p->tot_len,PBUF_RAM);
	if(q==NULL)
		return p;

	err=pbuf_copy(q,p);
	SSNP_ASSERT("bpuf_copy():failed\n",err==ERR_OK);
	pbuf_free(p);
	return q;
}

//void pbuf_ref_chain(struct pbuf* p)

