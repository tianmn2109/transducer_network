#include "sdp.h"
#include "debug.h"
#include "def.h"
#include "inet_chksum.h"
#include "memp.h"
#include "netif.h"

#include <string.h>

#define PAD_LEN    1
#define CHK_LEN    2

static struct sdp_pcb *sdp_pcbs;//dp_pcb链表
/*
 *  A 这里还没有实现分片功能
 *  B TBC和TBIM在这里的实现方式是不同的
 *
 */

static void set_status_word_protocol_error()
{
	//#未完成#：硬件设置
}

   
    /*
	 *  对于TBIM来说不是发送向自己的一律丢掉，源地址不是TBC的一律丢掉
	 *  对于TBC来说不是发送向自己的一律丢掉，源地址不是地址池中的一律丢掉   
	 *
	 *
	 */
static u8t addr_check(struct sdp_addr* src,struct sdp_addr* dst)
{
	//#未完成#：地址检测过滤策略
	return 1;
}

static u16t checksum(void* dataptr,u16t len)
{
	//#未完成#：计算校验和，如果确定了发送的流程那么可以直接在函数中计算加快速度
	return 0xffff;
}

static err_t dp_frag_output(struct pbuf* p,struct sdp_addr* src,struct sdp_addr* dst,struct netif* netif)
{
	//#未完成#：如果需要分片重装功能，那么在这里实现
	return ERR_OK;
}
static u8t isbroadcast(u8t alias)
{
	return alias == 0;
}


/*
 *
 *  这里数据参数pbuf必须是本层协议的数据，也就是底层协议提交上来的数据必须是处理好的
 *
 *
 *
 */
err_t sdp_input(struct pbuf* p,struct netif* inp)
{
	struct sdp_hdr* sdphdr;
	struct netif* netif;
	struct sdp_pcb* pcb;
	u8t src_alias;
	u8t src_tdcn_num;
	u8t dst_alias;
	u8t dst_tdcn_num;
	u8t match;
	u8t allempty;
	u8t pid;
	u16t* cksum;

	u16t sdp_data_pad_len;
	u16t sdp_data_len;
	
	if(sdp_pcbs == NULL)//如果此时没有控制块信息，那么直接删除掉数据
	{
		pbuf_free(p);
		return ERR_ARGUMENT;
	}

	sdphdr=(struct sdp_hdr*)p->data;

	pid=SDPH_PID(sdphdr);//#未完成#：这里需要判断这三种类型的协议吗？
	if(pid !=(u8t)PROTO_DGP && pid!=(u8t)PROTO_SDP && pid!=(u8t)PROTO_CSP && pid!=(u8t)PROTO_RP)//如果协议标示符不属于这三种则丢掉此数据
	{
		SSNP_DEBUG_PRINT("sdp_input(): sdp packet dropped due to bad protocol identifier.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}

	if(SDPH_V(sdphdr)!=1)//版本号不是1则丢掉
	{
		SSNP_DEBUG_PRINT("sdp_input():sdp packet dropped due to bad protocol version.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}

	sdp_data_len=NTOHS(sdphdr->len);//长度判断
	//sdp_data_len=sdphdr->len;//长度判断
	if(sdp_data_len % 2 == 0)
		sdp_data_pad_len=sdp_data_len;
	else
		sdp_data_pad_len=sdp_data_len + 1;//填充位

	if(sdp_data_pad_len > p->tot_len)
	{
		SSNP_DEBUG_PRINT("sdp_input():sdp packet is longer than pbuf.\r\n");
		printf("sdp_input():sdp_data_pad_len=%d p->tot_len=%d.\r\n",sdp_data_pad_len,p->tot_len);
		pbuf_free(p);
		return ERR_VALUE; 
	}

	cksum=(u16t*)((u8t*)sdphdr + sdp_data_pad_len - 2);
	if((*cksum) != 0xffff)
	{
		if(inet_chksum(sdphdr,sdp_data_pad_len) != 0)  //校验和
		{
			SSNP_DEBUG_PRINT("sdp_input():checksum failed.\n");
			pbuf_free(p);
			return ERR_VALUE;
		}
	}
  //#未完成#：这里还需要去掉dp后面的pad和checksum数据，这里的函数完成此功能了吗？
	pbuf_realloc(p,sdp_data_pad_len);//去掉后面的不属于dp的数据


	/*
	 *  后面要检测目的地址是否指向我们，但是这里的地址直接使用的是mac地址，如果这个地址不是指向我们
	 *  那么就已经被过滤掉了，但是这里的TBC和TBIM还是不一样的，TBIM不需要检测来源，而TBC是需要检测
	 *  来源的，虽然底层使用的是以太网，但是上面的协议在实现的过程中一定不可以假设底层使用的是什么网络。
	 *  上层网络协议的实现一定要和底层无关。
	 *
	 *  #未完成#：但是这里应该使用什么样的地址检测过滤机制呢？
	 *           对于TBIM来说不是发送向自己的一律丢掉，源地址不是TBC的一律丢掉
	 *           对于TBC来说不是发送向自己的一律丢掉，源地址不是地址池中的一律丢掉   
	 */           
	if(!addr_check(&sdphdr->src,&sdphdr->dst)) //地址检测
	{
		SSNP_DEBUG_PRINT("sdp_input():sdp address error.\n");
		pbuf_free(p);
		return ERR_VALUE;
	}

	/*
	 *  #未完成#：是不是广播地址
	 *
	 *  #未完成#：如果有分片重装功能也要在这里实现
	 *
	 *  #未完成#：raw.h的作用是什么？
	 */


	/*
	 *  #未完成#：对AKE，ACK，PSN，priority，序列号的处理
	 *
	 *
	 *
	 */

	/*
	 *   #未完成#：遍历pcb链表找到正确的pcb，调用相应的recv处理函数
	 *
	 *
	 *
	 */
	 match=0;
	 allempty=1;
	 src_alias=SDPH_SRC_ALIAS(sdphdr);
	 src_tdcn_num=SDPH_SRC_TDCN_NUM(sdphdr);
	 dst_alias=SDPH_DST_ALIAS(sdphdr);
	 dst_tdcn_num=SDPH_DST_TDCN_NUM(sdphdr);
	 if(pbuf_header(p,-SDP_HLEN))//#未完成#：这里虽然调整了头部指针，使得data指向协议数据，但是数据长度没有去掉后面的填充位和校验位
	 {                                      //这里需要改正一下，使得数据长度位就是数据的长度，这可以通过dp协议的Length位做到
	 	SSNP_DEBUG_PRINT("sdp_input():pbuf header failed.\r\n");
		pbuf_free(p);
		return ERR_VALUE;
	 }
	 if(isbroadcast(dst_alias))
	 {
		 for(pcb=sdp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
			 if(pcb->recv != NULL)
			 {
				 allempty=0;
				 pcb->recv(pcb->recv_arg,pcb,p,src_alias,src_tdcn_num,dst_tdcn_num);
			 }
		 }
		 if(allempty)
		 {
			 pbuf_free(p);
			 return ERR_VALUE;
		 }
		 else
		 {
		   return ERR_OK;
		 }
	 }
	 else
	 {
		 for(pcb=sdp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
			 if(pcb->local_alias == dst_alias && pcb->remote_alias == src_alias)
			 {
				 match=1;
				 break;
			 }
		 }
		 if(!match)
		 {
			 pbuf_free(p);
			 return ERR_VALUE;
		 }
		 else
		 {
			 if(pcb->recv!=NULL)
			 {
				 pcb->recv(pcb->recv_arg,pcb,p,src_alias,src_tdcn_num,dst_tdcn_num);
				 return ERR_OK;
			 }
			 else
			 {
				 pbuf_free(p);
				 return ERR_VALUE;
			 }
		 }
	 }


//	switch(DPH_PID(dphdr))
//	{
//	case PROTO_CSP:
//		{
//			//其实命令和回复协议完全可以在这里实现，但是如果协议分层比较明晰的化那就使用dp_pcb的回调函数来处理
//			//注意作为TBC如果接收到命令消息则直接丢弃
//			//#未完成#：command service protocol的input函数
//		}
//	case PROTO_RP:
//		{
//			//注意作为TBIM如果接收到回复消息则直接丢弃
//			//#未完成#：reply protocol的input函数
//		}
//	default:
//		{
//			//sdp_pcb的回调接收函数来处理
//			//#未完成#：本身是streaming data protocol的处理
//		}
//	}

//	return ERR_OK;
}


struct sdp_pcb* sdp_new()
{
	struct sdp_pcb* pcb;

	pcb=(struct sdp_pcb*)memp_alloc(MEMP_SDP_PCB);
	if(pcb != NULL)
	{
		memset(pcb,0,sizeof(struct sdp_pcb));
	}

	return pcb;
}


err_t sdp_bind(struct sdp_pcb* pcb,u8t l_alias,u8t l_tdcn_num)
{
	struct sdp_pcb* ipcb;
	u8t rebind;

	rebind=0;
	for(ipcb=sdp_pcbs;ipcb!=NULL;ipcb=ipcb->next)
	{
		if(ipcb == pcb)
		{
			rebind=1;
			break;
		}
		else
		{
		//	if((l_alias == ipcb->local_alias) && (l_tdcn_num == ipcb->local_tdcn_num))
		//		return ERR_USE;
		}
	}

	pcb->local_alias=l_alias;
	pcb->local_tdcn_num=l_tdcn_num;

	if(rebind == 0)
	{
		pcb->next=sdp_pcbs;
		sdp_pcbs=pcb;
	}

	return ERR_OK;
}


void sdp_remove(struct sdp_pcb* pcb)
{
	struct sdp_pcb* i;

	if(pcb == sdp_pcbs)
	{
		sdp_pcbs=pcb->next;
	}
	else
	{
		for(i=sdp_pcbs; i!=NULL; i=i->next)
		{
			if(i->next!=NULL && i->next == pcb)
				i->next=pcb->next;
		}
	}
	memp_free(MEMP_SDP_PCB,pcb);
}


//注意调用此函数之前pcb一定是bind完成的
err_t sdp_connect(struct sdp_pcb* pcb,u8t r_alias,u8t r_tdcn_num)
{
	struct sdp_pcb* i;

	pcb->remote_alias=r_alias;
	pcb->remote_tdcn_num=r_tdcn_num;
	pcb->flag |= SDP_FLAG_CONNECT; //每一位的意义不同

	for(i=sdp_pcbs;i!=NULL;i=i->next)
	{
		if(i == pcb)
			return ERR_OK;
	}

	pcb->next=sdp_pcbs;
	sdp_pcbs=pcb;

	return ERR_OK;
}

void sdp_disconnect(struct sdp_pcb* pcb)
{
	pcb->remote_alias=0xff;
	pcb->remote_tdcn_num=0;
	pcb->flag &=~SDP_FLAG_CONNECT;
}

void sdp_recv(struct sdp_pcb* pcb,sdp_recv_fn recv,void* recv_arg)
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


static err_t add_pad_pbuf(struct pbuf* p,struct pbuf** pad)
{
	u16t real_size;
	struct pbuf* buf_pad;
	real_size=(p->len & 0x01)? (PAD_LEN + CHK_LEN) : (CHK_LEN);

//	SSNP_DEBUG_PRINT("add_pad_pbuf():prepare to alloc memp_netpbuf_pad\r\n");
	buf_pad=memp_alloc(MEMP_NETPBUF_PAD);
	if(buf_pad == NULL)
	{
	    SSNP_DEBUG_PRINT("add_pad_pbuf():alloc buf_pad failed\r\n");
		return ERR_MEMORY;
	}
	
	buf_pad->tot_len=real_size;
	buf_pad->len=buf_pad->tot_len;
	buf_pad->next=NULL;
	buf_pad->data=(void*)((u8t*)buf_pad + sizeof(struct pbuf));
	
	*pad=buf_pad;
	
	return ERR_OK;
}


err_t sdp_send(struct sdp_pcb* pcb,struct pbuf* p)
{
	return sdp_sendto(pcb,p,pcb->remote_alias,pcb->remote_tdcn_num);
}


err_t sdp_sendto(struct sdp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num)
{
	struct netif* netif;
	//SSNP_DEBUG_PRINT("sdp_send():sdp find netif.\r\n");
	netif=ip_route(pcb->local_alias);//#未完成#：这里还有问题，也就是这里的ip层如何确定使用哪一个netif进行数据传输。
	if(netif == NULL)
		return ERR_ROUTE;
	//SSNP_DEBUG_PRINT("sdp_send():find netif.\r\n");
	return sdp_sendto_if(pcb,p,dst_alias,dst_tdcn_num,netif);
}


//这里的函数和上面的dp_output是一样的，去掉一个。
err_t sdp_sendto_if(struct sdp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num,struct netif* netif)
{
	struct sdp_hdr* sdphdr;
	struct pbuf* q;
	struct pbuf* pbuf_pad;
	err_t err;
	
	u8t protocol_id;
	u8t ake;
	u8t ack;
	u8t version;
	u8t psn;
	u8t priority;
	u16t seq_num;
	u16t src;
	u16t dst;
	u16t len;
	
	u8t* pad;
	u16t* chk;
  
	
//	if(pcb->local_tdcn_num == 0)//#未完成#：没有很好的弄清楚binding和connect函数的作用，再看一下。
//	{
//		SSNP_DEBUG_PRINT("dp_sendto_if():not yet bound to a port,binding now.\n");
//		err=dp_bind(pcb,pcb->local_alias,pcb->local_tdcn_num);
//		if(err != ERR_OK)
//		{
//			SSNP_DEBUG_PRINT("dp_sendto_if():forced port bind failed.\n");
//			return err;
//		}
//	}

	if(pbuf_header(p,SDP_HLEN))//#未完成#：比较奇葩的是dp的数据是在中间的，除了头部后面还有填充位和校验和，所以这里需要专门为dp修改一下
	{	//SSNP_DEBUG_PRINT("sdp_sendto_if():could not allocate header.\r\n");
		q=pbuf_alloc(PBUF_RAW,SDP_HLEN,PBUF_RAM);//重新加上一个可以容纳dp协议头部的容量
		if(q == NULL)
		{
			SSNP_DEBUG_PRINT("sdp_sendto_if():could not allocate header.\r\n");
			return ERR_MEMORY;
		}
		if(p->tot_len!=0)
			pbuf_chain(q,p);//#未完成#：这里的具体操作过程还不太清楚，在看一下
	}
	else
	{
		q=p;
	}

	//q就成为了要发送的数据
	SSNP_ASSERT("the first pbuf can not hold struct sdp_hdr.\n",(q->len >= sizeof(struct sdp_hdr)));

	protocol_id=PROTO_DGP;
	ake=0;
	ack=0;
	version=1;
	psn=0;
	priority=0;
	seq_num=0;
//	printf("sdp_send_if():pad pbuf.\r\n");
	pbuf_pad=NULL;
	if(add_pad_pbuf(p,&pbuf_pad) != ERR_OK)
	{
	//	SSNP_DEBUG_PRINT("sdp_sendto_if():add pad_pbuf failed\r\n");
		if(p!=q)
			pbuf_free(q);//如意这里的p是应用数据不可以在协议中删除，协议中只能删除在协议内部分配的数据
		return ERR_VALUE;
	}	
	//printf("sdp_send_if():pad pbuf done ok.\r\n");
	if(pbuf_pad->len & 0x01)
	{
		pad=(u8t*)pbuf_pad->data;
		*pad=0;
		chk=(u16t*)(pad + 1);
	}
	else
	{
		chk=(u16t*)pbuf_pad->data;
	}
	
	if(p == q)
		len=p->len + pbuf_pad->len;
	else
	    len=q->len + p->len + pbuf_pad->len;
	*chk=0xffff;
	//SSNP_DEBUG_PRINT("sdp_send_if():prepare to chain q and pbuf_pad.\r\n");
	pbuf_chain(q,pbuf_pad);
	//SSNP_DEBUG_PRINT("sdp_send_if():write pbuf.\r\n");
	sdphdr=(struct sdp_hdr*)q->data;//注意这里应该使用q而不是p
	
	SDPH_PAA_SET(sdphdr,protocol_id,ake,ack);
	SDPH_VPP_SET(sdphdr,version,psn,priority);
	sdphdr->len=HTONS(len);
	src=LINK(pcb->local_alias,pcb->local_tdcn_num);
	sdphdr->src.addr=HTONS(src);
	dst=LINK(dst_alias,dst_tdcn_num);
	sdphdr->dst.addr=HTONS(dst);
	sdphdr->sqn=seq_num;

	/*
	 *  #未完成#：这里需要计算校验和，数据填充位的计算。
	 *           将dp_output的内容写到这里。
	 *           后面重构的时候发送dp数据不需要调用这么多的函数，可以写到一个函数中。
	 *
	 */
//	SSNP_DEBUG_PRINT("sdp_send():pass sdp data to data link.\r\n");
	err=netif->output(netif,q,dst_alias,ETHTYPE_SDP);//#未完成#：这里的类型也需要确认一下
	
	memp_free(MEMP_NETPBUF_PAD,(void*)pbuf_pad);//删除掉填充的pbuf,并且一定注意使用memp_free()回收，因为这里通过memp_alloc()分配的
	pbuf_pad=NULL;
	p->next=NULL;
	if(q!=p)
	{
		pbuf_free(q);
		q=NULL;
	}

	return err;
}