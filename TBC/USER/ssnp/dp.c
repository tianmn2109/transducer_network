
#include "dp.h"
#include "debug.h"
#include "def.h"
#include "inet_chksum.h"
#include "memp.h"
#include "netif.h"
#include "ethbcp.h"
#include <string.h>


#define PAD_LEN    1
#define CHK_LEN    2

static struct dp_pcb *dp_pcbs;//dp_pcb链表
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
static u8t addr_check(struct dp_addr* src,struct dp_addr* dst)
{
	//#未完成#：地址检测过滤策略
	return 1;
}

static u16t checksum(void* dataptr,u16t len)
{
	//#未完成#：计算校验和，如果确定了发送的流程那么可以直接在函数中计算加快速度
	return 0xffff;
}

static err_t dp_frag_output(struct pbuf* p,struct dp_addr* src,struct dp_addr* dst,struct netif* netif)
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
err_t dp_input(struct pbuf* p,struct netif* inp)
{
	struct dp_hdr* dphdr;
	struct netif* netif;
	struct dp_pcb* pcb;
	u8t src_alias;
	u8t src_tdcn_num;
	u8t dst_alias;
	u8t dst_tdcn_num;
	u8t match;
	u8t allempty;
	u8t pid;
	u16t* cksum;
	u8t* test;

	u16t dp_data_pad_len;
	u16t dp_data_len;
	//SSNP_DEBUG_PRINT("dp_input(): here1.\r\n");
	if(dp_pcbs == NULL)//如果此时没有控制块信息，那么直接删除掉数据
	{ 
		SSNP_DEBUG_PRINT("dp_input(): delete dp data because the dp_pcb is NULL.\r\n");
		pbuf_free(p);
		return ERR_ARGUMENT;
	}

	dphdr=(struct dp_hdr*)p->data;

	pid=DPH_PID(dphdr);//#未完成#：这里需要判断这三种类型的协议吗？
	if(pid !=(u8t)PROTO_DGP && pid!=(u8t)PROTO_SDP && pid!=(u8t)PROTO_CSP && pid!=(u8t)PROTO_RP)//如果协议标示符不属于这三种则丢掉此数据
	{
		SSNP_DEBUG_PRINT("dp_input(): dp packet dropped due to bad protocol identifier.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}
	//SSNP_DEBUG_PRINT("dp_input(): here2.\r\n");
	if(DPH_V(dphdr)!=1)//版本号不是1则丢掉
	{
		SSNP_DEBUG_PRINT("dp_input():dp packet dropped due to bad protocol version.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}
//	SSNP_DEBUG_PRINT("dp_input(): here3.\r\n");
	dp_data_len=NTOHS(dphdr->len);//长度判断
	//dp_data_len=dphdr->len;//长度判断
	if(dp_data_len % 2 == 0)
		dp_data_pad_len=dp_data_len;
	else
		dp_data_pad_len=dp_data_len + 1;//填充位

	if(dp_data_pad_len > p->tot_len)
	{
		SSNP_DEBUG_PRINT("dp_input():dp packet is longer than pbuf.\r\n");
		printf("dp_input():dp_data_pad_len=%d,p->tot_len=%d.\r\n",dp_data_pad_len,p->tot_len);
		pbuf_free(p);
		return ERR_VALUE;
	}
//	SSNP_DEBUG_PRINT("dp_input(): here4.\r\n");
	cksum=(u16t*)((u8t*)dphdr + dp_data_pad_len - 2);
	if((*cksum) != 0xffff)
	{
		if(inet_chksum(dphdr,dp_data_pad_len) != 0)  //校验和
		{
			SSNP_DEBUG_PRINT("dp_input():checksum failed.\n");
			pbuf_free(p);
			return ERR_VALUE;
		}
	}
  //#未完成#：这里还需要去掉dp后面的pad和checksum数据，这里的函数完成此功能了吗？
	pbuf_realloc(p,dp_data_pad_len);//去掉后面的不属于dp的数据
  // SSNP_DEBUG_PRINT("dp_input(): here5.\r\n");

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
	if(!addr_check(&dphdr->src,&dphdr->dst)) //地址检测
	{
		SSNP_DEBUG_PRINT("dp_input():dp address error.\n");
		pbuf_free(p);
		return ERR_VALUE;
	}
//	SSNP_DEBUG_PRINT("dp_input(): here6.\r\n");
	/*
	 *  ：是不是广播地址
	 *  for(pcb=dp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
			 if(pcb->recv != NULL)
			 {	
			 //	 SSNP_DEBUG_PRINT("dp_input(): here9\r\n");
				 allempty=0;
				 pcb->recv(pcb->recv_arg,pcb,p,src_alias,src_tdcn_num,dst_tdcn_num);
			 }
		 }
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
	 src_alias=DPH_SRC_ALIAS(dphdr);
	 src_tdcn_num=DPH_SRC_TDCN_NUM(dphdr);
	 dst_alias=DPH_DST_ALIAS(dphdr);
	 dst_tdcn_num=DPH_DST_TDCN_NUM(dphdr);
	 /*
	 	if(dst_alias==0)
		{
			for(pcb=dp_pcbs;pcb!=NULL;pcb=pcb->next)
				pcb->recv(pcb->recv_arg,pcb,p,src_alias,src_tdcn_num,dst_tdcn_num);
			return ERR_OK;
		}
	 */
//----------------------------下面的代码是用来调试的 --------------------------------------------------------------
/*
     printf("\r\n-----------------------recv dp data--------------------------------------\r\n");
	 printf("pid %d, Version %d.\r\n",pid,DPH_V(dphdr));
	 printf("length %d\r\n.",dp_data_len);
	 printf("source address:alias %d,src tdcn num %d.\r\n",src_alias,src_tdcn_num);
	 printf("dst address:alias %d,src tdcn num %d.\r\n",dst_alias,dst_tdcn_num);
	 test=(u8t*)((u8t*)p->data+10);
	 printf("cmdclass %d,cmdfunc %d.\r\n",*test,*(test+1));
	  printf("-----------------------------------------------------------------------\r\n\r\n");
*/
//-----------------------------------------------------------------------------------------------------------------


	 if(pbuf_header(p,-DP_HLEN))//#未完成#：这里虽然调整了头部指针，使得data指向协议数据，但是数据长度没有去掉后面的填充位和校验位
	 {                                      //这里需要改正一下，使得数据长度位就是数据的长度，这可以通过dp协议的Length位做到
	 	SSNP_DEBUG_PRINT("dp_input():pbuf header failed.\r\n");
		pbuf_free(p);
		return ERR_VALUE;
	 }
//	 SSNP_DEBUG_PRINT("dp_input(): here7.\r\n");
	 if(isbroadcast(dst_alias))
	 {	
	 //	SSNP_DEBUG_PRINT("dp_input(): here8\r\n");
		 for(pcb=dp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
			 if(pcb->recv != NULL)
			 {	
			 //	 SSNP_DEBUG_PRINT("dp_input(): here9\r\n");
				 allempty=0;
				 pcb->recv(pcb->recv_arg,pcb,p,src_alias,src_tdcn_num,dst_tdcn_num);
			 }
		 }
		 if(allempty)
		 {	 
		 	 SSNP_DEBUG_PRINT("dp_input(): delete pbuf.\r\n");
			 pbuf_free(p);
			 return ERR_VALUE;
		 }
		 else
		 { 
		   //SSNP_DEBUG_PRINT("dp_input(): here11\r\n");
		   return ERR_OK;
		 }
	 }
	 else
	 {
	 	// SSNP_DEBUG_PRINT("dp_input(): here12\r\n");
	
		 for(pcb=dp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
		 //	 printf("local_alias is %d,dst_alias is %d,pcb->remote_alias is %d ,src_alias is %d.",pcb->local_alias,dst_alias,pcb->remote_alias,src_alias);
			 if(pcb->local_alias == dst_alias && pcb->remote_alias == src_alias)
			 {
				 match=1;
				 break;
			 }
		 }
		 if(!match)
		 {
		 	 SSNP_DEBUG_PRINT("dp_input(): here13\r\n");
			 pbuf_free(p);
			 return ERR_VALUE;
		 }
		 else
		 {
			 if(pcb->recv!=NULL)
			 {	 //SSNP_DEBUG_PRINT("dp_input(): send to app.\r\n");
				 pcb->recv(pcb->recv_arg,pcb,p,src_alias,src_tdcn_num,dst_tdcn_num);
				 return ERR_OK;
			 }
			 else
			 {	 SSNP_DEBUG_PRINT("dp_input(): here14.\r\n");
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


/*
 *  #未完成#：这里我们假设给出的参数src和dst是没有问题的，并且除了
 *  这两个参数其他的信息都在pbuf中配置好了，也为填充位和校验和未预留了
 *  空间，同时pbuf的data指针指向dp头部。
 *  
 *  注意这里的dp中的各项参数必须在这一层里完成，因为上层应用时看不到这里的数据格式的
 */

err_t dp_output(struct pbuf* p,struct dp_addr* src,struct dp_addr* dst,struct netif* netif)
{
	struct dp_hdr* dphdr;
	u16t dp_data_len;
	u16t dp_data_pad_len;
	u16t *chksum;
	u16t pid;

	dphdr=(struct dp_hdr*)p->data;
	pid=DPH_PID(dphdr);//由于这里的数据报和数据流使用相同的函数进行数据发送，因此这里要区分pid
	dp_data_len=dphdr->len;

	if(dp_data_len % 2 ==0)
		dp_data_pad_len=dp_data_len;
	else
		dp_data_pad_len=dp_data_len + 1;

	chksum=(u16t*)((u8t*)dphdr + dp_data_pad_len - 2);
	*chksum=checksum(dphdr,dp_data_pad_len - 2);

	/*
	 *
	 *  #未完成#：这里可能还需要其他的处理。如数据填充等等
	 *
	 *
	 */

	if(netif->mtu && p->tot_len>netif->mtu)
		return dp_frag_output(p,src,dst,netif);

	return netif->output(netif,p,((dst->addr)>>8),pid);//这里使用pid表示可以发送两种数据
}

struct dp_pcb* dp_new()
{
	struct dp_pcb* pcb;

	pcb=(struct dp_pcb*)memp_alloc(MEMP_DP_PCB);
	if(pcb != NULL)
	{
		memset(pcb,0,sizeof(struct dp_pcb));
	}

	return pcb;
}
/*
	问题在这里,TBC的同每个TBIM新建的连接中的pcb中的本地别名和变送器通道号是相同的，导致每次直接返回了
*/															   
err_t dp_bind(struct dp_pcb* pcb,u8t l_alias,u8t l_tdcn_num)
{
	struct dp_pcb* ipcb;
	u8t rebind;

	rebind=0;
	for(ipcb=dp_pcbs;ipcb!=NULL;ipcb=ipcb->next)
	{
		if(ipcb == pcb)
		{
			rebind=1;
			break;
		}
		else
		{
		//	if((l_alias == ipcb->local_alias) && (l_tdcn_num == ipcb->local_tdcn_num))
			//	return ERR_USE;
		}
	}

	pcb->local_alias=l_alias;
	pcb->local_tdcn_num=l_tdcn_num;

	if(rebind == 0)
	{
		pcb->next=dp_pcbs;
		dp_pcbs=pcb;
	}

	return ERR_OK;
}

void dp_remove(struct dp_pcb* pcb)
{
	struct dp_pcb* i;

	if(pcb == dp_pcbs)
	{
		dp_pcbs=pcb->next;
	}
	else
	{
		for(i=dp_pcbs; i!=NULL; i=i->next)
		{
			if(i->next!=NULL && i->next == pcb)
				i->next=pcb->next;
		}
	}
	memp_free(MEMP_DP_PCB,pcb);
}

//注意调用此函数之前pcb一定是bind完成的
err_t dp_connect(struct dp_pcb* pcb,u8t r_alias,u8t r_tdcn_num)
{
	struct dp_pcb* i;

	pcb->remote_alias=r_alias;
	pcb->remote_tdcn_num=r_tdcn_num;
	pcb->flag |= DP_FLAG_CONNECT; //每一位的意义不同

	for(i=dp_pcbs;i!=NULL;i=i->next)
	{
		if(i == pcb)
			return ERR_OK;
	}

	pcb->next=dp_pcbs;
	dp_pcbs=pcb;

	return ERR_OK;
}

void dp_disconnect(struct dp_pcb* pcb)
{
	pcb->remote_alias=0xff;
	pcb->remote_tdcn_num=0;
	pcb->flag &=~DP_FLAG_CONNECT;
}

void dp_recv(struct dp_pcb* pcb,dp_recv_fn recv,void* recv_arg)
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
err_t dp_send(struct dp_pcb* pcb,struct pbuf* p)
{
	return dp_sendto(pcb,p,pcb->remote_alias,pcb->remote_tdcn_num);
}

err_t dp_sendto(struct dp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num)
{
	struct netif* netif;
//	SSNP_DEBUG_PRINT("dp_send():dp find netif.\r\n");
	netif=ip_route(pcb->local_alias);//#未完成#：这里还有问题，也就是这里的ip层如何确定使用哪一个netif进行数据传输。
	if(netif == NULL)
		return ERR_ROUTE;
//	SSNP_DEBUG_PRINT("dp_send():find netif.\r\n");
	return dp_sendto_if(pcb,p,dst_alias,dst_tdcn_num,netif);
}
//这里的函数和上面的dp_output是一样的，去掉一个。
err_t dp_sendto_if(struct dp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num,struct netif* netif)
{
	struct dp_hdr* dphdr;
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
	u8t* test;
	
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

	if(pbuf_header(p,DP_HLEN))//#未完成#：比较奇葩的是dp的数据是在中间的，除了头部后面还有填充位和校验和，所以这里需要专门为dp修改一下
	{//	SSNP_DEBUG_PRINT("dp_sendto_if():could not allocate header.\r\n");
		q=pbuf_alloc(PBUF_RAW,DP_HLEN,PBUF_RAM);//重新加上一个可以容纳dp协议头部的容量
		if(q == NULL)
		{
			SSNP_DEBUG_PRINT("dp_sendto_if():could not allocate header.\r\n");
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
//	SSNP_ASSERT("the first pbuf can not hold struct dp_hdr.\n",(q->len >= sizeof(struct dp_hdr)));

	protocol_id=PROTO_DGP;
	ake=0;
	ack=0;
	version=1;
	psn=0;
	priority=0;
	seq_num=0;
//	printf("dp_send_if():pad pbuf.\r\n");
	pbuf_pad=NULL;
	if(add_pad_pbuf(p,&pbuf_pad) != ERR_OK)
	{
		SSNP_DEBUG_PRINT("dp_sendto_if():add pad_pbuf failed\r\n");
		if(p!=q)
			pbuf_free(q);//如意这里的p是应用数据不可以在协议中删除，协议中只能删除在协议内部分配的数据
		return ERR_VALUE;
	}	
//	printf("dp_send_if():pad pbuf done ok.\r\n");
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
//	SSNP_DEBUG_PRINT("dp_send_if():prepare to chain q and pbuf_pad.\r\n");
	pbuf_chain(q,pbuf_pad);
//	SSNP_DEBUG_PRINT("dp_send_if():write pbuf.\r\n");
	dphdr=(struct dp_hdr*)q->data;//注意这里应该使用q而不是p
	
	DPH_PAA_SET(dphdr,protocol_id,ake,ack);
	DPH_VPP_SET(dphdr,version,psn,priority);
	dphdr->len=HTONS(len);
	src=LINK(pcb->local_alias,pcb->local_tdcn_num);
	dphdr->src.addr=HTONS(src);
	dst=LINK(dst_alias,dst_tdcn_num);
	dphdr->dst.addr=HTONS(dst);
	dphdr->sqn=seq_num;
 //----------------------------下面的代码是用来调试的 --------------------------------------------------------------
 /*
     printf("\r\n-----------------------send dp data--------------------------------------\r\n");
	 printf("pid %d, Version %d.\r\n",protocol_id,version);
	 printf("length %d\r\n.",len);
	 printf("source address:alias %d,src tdcn num %d.\r\n",pcb->local_alias,pcb->local_tdcn_num);
	 printf("dst address:alias %d,dst tdcn num %d.\r\n",dst_alias,dst_tdcn_num);
	 test=(u8t*)((u8t*)p->data);
	 printf("cmdclass %d,cmdfunc %d.\r\n",*test,*(test+1));
	  printf("-----------------------------------------------------------------------\r\n\r\n");
*/
//-----------------------------------------------------------------------------------------------------------------
	/*
	 *  #未完成#：这里需要计算校验和，数据填充位的计算。
	 *           将dp_output的内容写到这里。
	 *           后面重构的时候发送dp数据不需要调用这么多的函数，可以写到一个函数中。
	 *
	 */
//	SSNP_DEBUG_PRINT("dp_send():pass dp data to data link.\r\n");
	err=netif->output(netif,q,dst_alias,ETHTYPE_DP);//#未完成#：这里的类型也需要确认一下
	
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
