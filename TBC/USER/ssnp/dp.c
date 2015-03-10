
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

static struct dp_pcb *dp_pcbs;//dp_pcb����
/*
 *  A ���ﻹû��ʵ�ַ�Ƭ����
 *  B TBC��TBIM�������ʵ�ַ�ʽ�ǲ�ͬ��
 *
 */

static void set_status_word_protocol_error()
{
	//#δ���#��Ӳ������
}

   
    /*
	 *  ����TBIM��˵���Ƿ������Լ���һ�ɶ�����Դ��ַ����TBC��һ�ɶ���
	 *  ����TBC��˵���Ƿ������Լ���һ�ɶ�����Դ��ַ���ǵ�ַ���е�һ�ɶ���   
	 *
	 *
	 */
static u8t addr_check(struct dp_addr* src,struct dp_addr* dst)
{
	//#δ���#����ַ�����˲���
	return 1;
}

static u16t checksum(void* dataptr,u16t len)
{
	//#δ���#������У��ͣ����ȷ���˷��͵�������ô����ֱ���ں����м���ӿ��ٶ�
	return 0xffff;
}

static err_t dp_frag_output(struct pbuf* p,struct dp_addr* src,struct dp_addr* dst,struct netif* netif)
{
	//#δ���#�������Ҫ��Ƭ��װ���ܣ���ô������ʵ��
	return ERR_OK;
}
static u8t isbroadcast(u8t alias)
{
	return alias == 0;
}
/*
 *
 *  �������ݲ���pbuf�����Ǳ���Э������ݣ�Ҳ���ǵײ�Э���ύ���������ݱ����Ǵ���õ�
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
	if(dp_pcbs == NULL)//�����ʱû�п��ƿ���Ϣ����ôֱ��ɾ��������
	{ 
		SSNP_DEBUG_PRINT("dp_input(): delete dp data because the dp_pcb is NULL.\r\n");
		pbuf_free(p);
		return ERR_ARGUMENT;
	}

	dphdr=(struct dp_hdr*)p->data;

	pid=DPH_PID(dphdr);//#δ���#��������Ҫ�ж����������͵�Э����
	if(pid !=(u8t)PROTO_DGP && pid!=(u8t)PROTO_SDP && pid!=(u8t)PROTO_CSP && pid!=(u8t)PROTO_RP)//���Э���ʾ���������������򶪵�������
	{
		SSNP_DEBUG_PRINT("dp_input(): dp packet dropped due to bad protocol identifier.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}
	//SSNP_DEBUG_PRINT("dp_input(): here2.\r\n");
	if(DPH_V(dphdr)!=1)//�汾�Ų���1�򶪵�
	{
		SSNP_DEBUG_PRINT("dp_input():dp packet dropped due to bad protocol version.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}
//	SSNP_DEBUG_PRINT("dp_input(): here3.\r\n");
	dp_data_len=NTOHS(dphdr->len);//�����ж�
	//dp_data_len=dphdr->len;//�����ж�
	if(dp_data_len % 2 == 0)
		dp_data_pad_len=dp_data_len;
	else
		dp_data_pad_len=dp_data_len + 1;//���λ

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
		if(inet_chksum(dphdr,dp_data_pad_len) != 0)  //У���
		{
			SSNP_DEBUG_PRINT("dp_input():checksum failed.\n");
			pbuf_free(p);
			return ERR_VALUE;
		}
	}
  //#δ���#�����ﻹ��Ҫȥ��dp�����pad��checksum���ݣ�����ĺ�����ɴ˹�������
	pbuf_realloc(p,dp_data_pad_len);//ȥ������Ĳ�����dp������
  // SSNP_DEBUG_PRINT("dp_input(): here5.\r\n");

	/*
	 *  ����Ҫ���Ŀ�ĵ�ַ�Ƿ�ָ�����ǣ���������ĵ�ֱַ��ʹ�õ���mac��ַ����������ַ����ָ������
	 *  ��ô���Ѿ������˵��ˣ����������TBC��TBIM���ǲ�һ���ģ�TBIM����Ҫ�����Դ����TBC����Ҫ���
	 *  ��Դ�ģ���Ȼ�ײ�ʹ�õ�����̫�������������Э����ʵ�ֵĹ�����һ�������Լ���ײ�ʹ�õ���ʲô���硣
	 *  �ϲ�����Э���ʵ��һ��Ҫ�͵ײ��޹ء�
	 *
	 *  #δ���#����������Ӧ��ʹ��ʲô���ĵ�ַ�����˻����أ�
	 *           ����TBIM��˵���Ƿ������Լ���һ�ɶ�����Դ��ַ����TBC��һ�ɶ���
	 *           ����TBC��˵���Ƿ������Լ���һ�ɶ�����Դ��ַ���ǵ�ַ���е�һ�ɶ���   
	 */           
	if(!addr_check(&dphdr->src,&dphdr->dst)) //��ַ���
	{
		SSNP_DEBUG_PRINT("dp_input():dp address error.\n");
		pbuf_free(p);
		return ERR_VALUE;
	}
//	SSNP_DEBUG_PRINT("dp_input(): here6.\r\n");
	/*
	 *  ���ǲ��ǹ㲥��ַ
	 *  for(pcb=dp_pcbs;pcb!=NULL;pcb=pcb->next)
		 {
			 if(pcb->recv != NULL)
			 {	
			 //	 SSNP_DEBUG_PRINT("dp_input(): here9\r\n");
				 allempty=0;
				 pcb->recv(pcb->recv_arg,pcb,p,src_alias,src_tdcn_num,dst_tdcn_num);
			 }
		 }
	 *  #δ���#������з�Ƭ��װ����ҲҪ������ʵ��
	 *
	 *  #δ���#��raw.h��������ʲô��
	 */


	/*
	 *  #δ���#����AKE��ACK��PSN��priority�����кŵĴ���
	 *
	 *
	 *
	 */

	/*
	 *   #δ���#������pcb�����ҵ���ȷ��pcb��������Ӧ��recv������
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
//----------------------------����Ĵ������������Ե� --------------------------------------------------------------
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


	 if(pbuf_header(p,-DP_HLEN))//#δ���#��������Ȼ������ͷ��ָ�룬ʹ��dataָ��Э�����ݣ��������ݳ���û��ȥ����������λ��У��λ
	 {                                      //������Ҫ����һ�£�ʹ�����ݳ���λ�������ݵĳ��ȣ������ͨ��dpЭ���Lengthλ����
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
//			//��ʵ����ͻظ�Э����ȫ����������ʵ�֣��������Э��ֲ�Ƚ������Ļ��Ǿ�ʹ��dp_pcb�Ļص�����������
//			//ע����ΪTBC������յ�������Ϣ��ֱ�Ӷ���
//			//#δ���#��command service protocol��input����
//		}
//	case PROTO_RP:
//		{
//			//ע����ΪTBIM������յ��ظ���Ϣ��ֱ�Ӷ���
//			//#δ���#��reply protocol��input����
//		}
//	default:
//		{
//			//sdp_pcb�Ļص����պ���������
//			//#δ���#��������streaming data protocol�Ĵ���
//		}
//	}

//	return ERR_OK;
}


/*
 *  #δ���#���������Ǽ�������Ĳ���src��dst��û������ģ����ҳ���
 *  ������������������Ϣ����pbuf�����ú��ˣ�ҲΪ���λ��У���δԤ����
 *  �ռ䣬ͬʱpbuf��dataָ��ָ��dpͷ����
 *  
 *  ע�������dp�еĸ��������������һ������ɣ���Ϊ�ϲ�Ӧ��ʱ��������������ݸ�ʽ��
 */

err_t dp_output(struct pbuf* p,struct dp_addr* src,struct dp_addr* dst,struct netif* netif)
{
	struct dp_hdr* dphdr;
	u16t dp_data_len;
	u16t dp_data_pad_len;
	u16t *chksum;
	u16t pid;

	dphdr=(struct dp_hdr*)p->data;
	pid=DPH_PID(dphdr);//������������ݱ���������ʹ����ͬ�ĺ����������ݷ��ͣ��������Ҫ����pid
	dp_data_len=dphdr->len;

	if(dp_data_len % 2 ==0)
		dp_data_pad_len=dp_data_len;
	else
		dp_data_pad_len=dp_data_len + 1;

	chksum=(u16t*)((u8t*)dphdr + dp_data_pad_len - 2);
	*chksum=checksum(dphdr,dp_data_pad_len - 2);

	/*
	 *
	 *  #δ���#��������ܻ���Ҫ�����Ĵ������������ȵ�
	 *
	 *
	 */

	if(netif->mtu && p->tot_len>netif->mtu)
		return dp_frag_output(p,src,dst,netif);

	return netif->output(netif,p,((dst->addr)>>8),pid);//����ʹ��pid��ʾ���Է�����������
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
	����������,TBC��ͬÿ��TBIM�½��������е�pcb�еı��ر����ͱ�����ͨ��������ͬ�ģ�����ÿ��ֱ�ӷ�����
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

//ע����ô˺���֮ǰpcbһ����bind��ɵ�
err_t dp_connect(struct dp_pcb* pcb,u8t r_alias,u8t r_tdcn_num)
{
	struct dp_pcb* i;

	pcb->remote_alias=r_alias;
	pcb->remote_tdcn_num=r_tdcn_num;
	pcb->flag |= DP_FLAG_CONNECT; //ÿһλ�����岻ͬ

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
	netif=ip_route(pcb->local_alias);//#δ���#�����ﻹ�����⣬Ҳ���������ip�����ȷ��ʹ����һ��netif�������ݴ��䡣
	if(netif == NULL)
		return ERR_ROUTE;
//	SSNP_DEBUG_PRINT("dp_send():find netif.\r\n");
	return dp_sendto_if(pcb,p,dst_alias,dst_tdcn_num,netif);
}
//����ĺ����������dp_output��һ���ģ�ȥ��һ����
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
  
	
//	if(pcb->local_tdcn_num == 0)//#δ���#��û�кܺõ�Ū���binding��connect���������ã��ٿ�һ�¡�
//	{
//		SSNP_DEBUG_PRINT("dp_sendto_if():not yet bound to a port,binding now.\n");
//		err=dp_bind(pcb,pcb->local_alias,pcb->local_tdcn_num);
//		if(err != ERR_OK)
//		{
//			SSNP_DEBUG_PRINT("dp_sendto_if():forced port bind failed.\n");
//			return err;
//		}
//	}

	if(pbuf_header(p,DP_HLEN))//#δ���#���Ƚ��������dp�����������м�ģ�����ͷ�����滹�����λ��У��ͣ�����������Ҫר��Ϊdp�޸�һ��
	{//	SSNP_DEBUG_PRINT("dp_sendto_if():could not allocate header.\r\n");
		q=pbuf_alloc(PBUF_RAW,DP_HLEN,PBUF_RAM);//���¼���һ����������dpЭ��ͷ��������
		if(q == NULL)
		{
			SSNP_DEBUG_PRINT("dp_sendto_if():could not allocate header.\r\n");
			return ERR_MEMORY;
		}
		if(p->tot_len!=0)
			pbuf_chain(q,p);//#δ���#������ľ���������̻���̫������ڿ�һ��
	}
	else
	{
		q=p;
	}

	//q�ͳ�Ϊ��Ҫ���͵�����
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
			pbuf_free(q);//���������p��Ӧ�����ݲ�������Э����ɾ����Э����ֻ��ɾ����Э���ڲ����������
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
	dphdr=(struct dp_hdr*)q->data;//ע������Ӧ��ʹ��q������p
	
	DPH_PAA_SET(dphdr,protocol_id,ake,ack);
	DPH_VPP_SET(dphdr,version,psn,priority);
	dphdr->len=HTONS(len);
	src=LINK(pcb->local_alias,pcb->local_tdcn_num);
	dphdr->src.addr=HTONS(src);
	dst=LINK(dst_alias,dst_tdcn_num);
	dphdr->dst.addr=HTONS(dst);
	dphdr->sqn=seq_num;
 //----------------------------����Ĵ������������Ե� --------------------------------------------------------------
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
	 *  #δ���#��������Ҫ����У��ͣ��������λ�ļ��㡣
	 *           ��dp_output������д�����
	 *           �����ع���ʱ����dp���ݲ���Ҫ������ô��ĺ���������д��һ�������С�
	 *
	 */
//	SSNP_DEBUG_PRINT("dp_send():pass dp data to data link.\r\n");
	err=netif->output(netif,q,dst_alias,ETHTYPE_DP);//#δ���#�����������Ҳ��Ҫȷ��һ��
	
	memp_free(MEMP_NETPBUF_PAD,(void*)pbuf_pad);//ɾ��������pbuf,����һ��ע��ʹ��memp_free()���գ���Ϊ����ͨ��memp_alloc()�����
	pbuf_pad=NULL;
	p->next=NULL;
	if(q!=p)
	{
		pbuf_free(q);
		q=NULL;
	}

	return err;
}
