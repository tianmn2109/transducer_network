#include "tp.h"
#include "debug.h"
#include "inet_chksum.h"
#include "memp.h"
#include "ethbcp.h"
#include "def.h"
#include <string.h>

static struct tp_pcb* tp_pcbs;

static void set_status_word_protocol_error()
{
	//#未完成#：硬件设置
}
static u8t isbroadcast(u8t alias)
{
	return alias == 0;
}
err_t tp_input(struct pbuf* p,struct netif* inp)
{
	struct tp_hdr* tphdr;
	struct tp_pcb* pcb;
	u8t src_alias;
	u8t src_tdcn_num;
	u8t dst_alias;
	u8t dst_tdcn_num;
	u8t match;
	u8t allempty;

	tphdr=(struct tp_hdr*)p->data;


  if(TPH_V(tphdr)!=1)
	{
		SSNP_DEBUG_PRINT("tp_input():incorrect version.\n");
		pbuf_free(p);
		return ERR_OK;
	}
	if(p->tot_len < TG_LEN)
	{
		SSNP_DEBUG_PRINT("tp_input():pbuf is too small.\n");
		pbuf_free(p);
		return ERR_OK;
	}
	if(tphdr->chksum != 0xffff)//#未完成#：反码校验不需要考虑校验和，注意这里确实也不需要网络字节到主机字节转换
	{
		if(inet_chksum(tphdr,TG_LEN) != 0)
		{
			SSNP_DEBUG_PRINT("tp_input():checksum failed.\n");
			pbuf_free(p);
			return ERR_OK;
		}
	}
//对于trigger协议丢不丢掉后面的数据都是无所谓的，因为本身就不需要后面的数据
	//pbuf_realloc(p,TG_LEN);

	/*
	 *   #未完成#：这里是和ake，ack priority等相关的代码
	 *            此后找到正确的pcb，然后调用其回调函数处理
	 *
	 *
	 */		   
	 match=0;
	 allempty=1;
	 src_alias=TPH_SRC_ALIAS(tphdr);
	 src_tdcn_num=TPH_SRC_TDCN_NUM(tphdr);
	 dst_alias=TPH_DST_ALIAS(tphdr);
	 dst_tdcn_num=TPH_DST_TDCN_NUM(tphdr);
	 if(isbroadcast(dst_alias))
	 {	
		 SSNP_DEBUG_PRINT("tp_input():alias is broadcast.\r\n");
		 for(pcb=tp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
			 if(pcb->recv != NULL)
			 {
				 pcb->recv(pcb->recv_arg,pcb,src_alias,src_tdcn_num,dst_tdcn_num);
				 allempty=0;
			 }
		 }
		 if(!allempty)
		 {
			 pbuf_free(p);//trigger协议不需要pbuf
			 return ERR_OK;
		 }
	 }
	 else
	 {	 
	   printf("tp_input():dst_alias=%d , src_alias=%d\r\n",dst_alias,src_alias);
		 for(pcb=tp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
			 if(pcb->local_alias == dst_alias && pcb->remote_alias == src_alias)
			 {
				 match=1;
				 break;
			 }
		 }
		 if(match)
		 {	 
			 SSNP_DEBUG_PRINT("tp_input():call the callback function to recv data.\r\n");
			 pcb->recv(pcb->recv_arg,pcb,src_alias,src_tdcn_num,dst_tdcn_num);
			 pbuf_free(p);
			 return ERR_OK;
		 }
	 }
	pbuf_free(p);	 
	return ERR_VALUE;
}


struct tp_pcb* tp_new()
{
	struct tp_pcb* pcb;

	pcb=(struct tp_pcb*)memp_alloc(MEMP_TP_PCB);
	if(pcb!=NULL)
		memset(pcb,0,sizeof(struct tp_pcb));

	return pcb;
}
 
err_t tp_bind(struct tp_pcb* pcb,u8t l_alias,u8t l_tdcn_num)
{
	struct tp_pcb* ipcb;
	u8t rebind;

	rebind=0;
	for(ipcb=tp_pcbs;ipcb!=NULL;ipcb=ipcb->next)
	{
		if(ipcb == pcb)
		{
			rebind=1;
			break;
		}
		else
		{
			if((l_alias == ipcb->local_alias) && (l_tdcn_num == ipcb->local_tdcn_num))
				return ERR_USE;
		}
	}

	pcb->local_alias=l_alias;
	pcb->local_tdcn_num=l_tdcn_num;

	if(rebind == 0)
	{
		pcb->next=tp_pcbs;
		tp_pcbs=pcb;
	}

	return ERR_OK;
}

void  tp_remove(struct tp_pcb* pcb)
{
	struct tp_pcb* i;

	if(pcb == tp_pcbs)
	{
		tp_pcbs=pcb->next;
	}
	else
	{
		for(i=tp_pcbs; i!=NULL; i=i->next)
		{
			if(i->next!=NULL && i->next == pcb)
				i->next=pcb->next;
		}
	}
	memp_free(MEMP_TP_PCB,pcb);
}

err_t tp_connect(struct tp_pcb* pcb,u8t r_alias,u8t r_tdcn_num)
{
	struct tp_pcb* i;

	pcb->remote_alias=r_alias;
	pcb->remote_tdcn_num=r_tdcn_num;
	pcb->flag |= TP_FLAG_CONNECT; //每一位的意义不同

	for(i=tp_pcbs;i!=NULL;i=i->next)
	{
		if(i == pcb)
			return ERR_OK;
	}

	pcb->next=tp_pcbs;
	tp_pcbs=pcb;

	return ERR_OK;
}

void tp_disconnect(struct tp_pcb* pcb)
{
	pcb->remote_alias=0xff;
	pcb->remote_tdcn_num=0;
	pcb->flag &=~TP_FLAG_CONNECT;
}

void tp_recv(struct tp_pcb* pcb,tp_recv_fn recv,void* recv_arg)
{
	pcb->recv=recv;
	pcb->recv_arg=recv_arg;
}
static struct netif* ip_route(u8t alias)
{
	struct netif* i;

	for(i=netif_list;i!=NULL;i=i->next)
	{
		if(i->alias == alias)
		{
			return i;
		}
	}

	if(netif_default == NULL)
		return NULL;

	return netif_default;
}
err_t tp_send(struct tp_pcb* pcb,struct pbuf* p)
{
	//#未完成#：这里还没有实现

	return ERR_OK;
}
err_t tp_sendto(struct tp_pcb* pcb,struct pbuf* p,u8t alias,u8t tdcn_num)
{
	struct netif* netif;

	netif=ip_route(pcb->local_alias);//#未完成#：这里还有问题，也就是这里的ip层如何确定使用哪一个netif进行数据传输。
	if(netif == NULL)
		return ERR_ROUTE;

	return tp_sendto_if(pcb,p,alias,tdcn_num,netif);
}

//注意这里的参数p是NULL,不要使用
err_t tp_sendto_if(struct tp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t tdcn_num,struct netif* netif)
{
	struct tp_hdr* tphdr;
	struct pbuf* buf;
	err_t err;
	u8t protocol_id;
	u8t ake;
	u8t ack;
	u8t version;
	u8t sum;
	u8t priority;
	u16t src;
	u16t dst;
	
	buf=memp_alloc(MEMP_TP);
	if(buf == NULL)
	{
		SSNP_DEBUG_PRINT("tp_sendto():trigger data alloc failed.\r\n");
		return ERR_MEMORY;
	}
	buf->ref=1;
	buf->len=sizeof(struct tp_hdr) + PBUF_LINK_HLEN;
	buf->tot_len=buf->len;
	buf->data=(void*)((u8t*)buf + sizeof(struct pbuf) + LINK_LEN);//这里其实破坏了分层的原则，上层应用应该不知道底层的结构
	buf->next=NULL;//其他的不需要设置，因为这不是通过pbuf函数来得到的
	
	tphdr=(struct tp_hdr*)buf->data;

	protocol_id=TRIGGER_PROTOCOL;
	ake=0;
	ack=0;
	version=1;
	sum=0;
	priority=0;
	src=LINK(pcb->local_alias,pcb->local_tdcn_num);
	dst=LINK(dst_alias,tdcn_num);
	
	TPH_PAA_SET(tphdr,protocol_id,ake,ack);
	TPH_VPP_SET(tphdr,version,sum,priority);
	tphdr->src.addr=HTONS(src);
	tphdr->dst.addr=HTONS(dst);
	tphdr->chksum=0xffff;
	
	err=netif->output(netif,buf,dst_alias,ETHTYPE_TP);
	memp_free(MEMP_TP,(void*)buf);
	
	return err;
}
