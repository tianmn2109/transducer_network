#include "sdp.h"
#include "debug.h"
#include "def.h"
#include "inet_chksum.h"
#include "memp.h"
#include "netif.h"

#include <string.h>

#define PAD_LEN    1
#define CHK_LEN    2

static struct sdp_pcb *sdp_pcbs;//dp_pcb����
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
static u8t addr_check(struct sdp_addr* src,struct sdp_addr* dst)
{
	//#δ���#����ַ�����˲���
	return 1;
}

static u16t checksum(void* dataptr,u16t len)
{
	//#δ���#������У��ͣ����ȷ���˷��͵�������ô����ֱ���ں����м���ӿ��ٶ�
	return 0xffff;
}

static err_t dp_frag_output(struct pbuf* p,struct sdp_addr* src,struct sdp_addr* dst,struct netif* netif)
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
	
	if(sdp_pcbs == NULL)//�����ʱû�п��ƿ���Ϣ����ôֱ��ɾ��������
	{
		pbuf_free(p);
		return ERR_ARGUMENT;
	}

	sdphdr=(struct sdp_hdr*)p->data;

	pid=SDPH_PID(sdphdr);//#δ���#��������Ҫ�ж����������͵�Э����
	if(pid !=(u8t)PROTO_DGP && pid!=(u8t)PROTO_SDP && pid!=(u8t)PROTO_CSP && pid!=(u8t)PROTO_RP)//���Э���ʾ���������������򶪵�������
	{
		SSNP_DEBUG_PRINT("sdp_input(): sdp packet dropped due to bad protocol identifier.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}

	if(SDPH_V(sdphdr)!=1)//�汾�Ų���1�򶪵�
	{
		SSNP_DEBUG_PRINT("sdp_input():sdp packet dropped due to bad protocol version.\r\n");
		pbuf_free(p);
		set_status_word_protocol_error();
		return ERR_VALUE;
	}

	sdp_data_len=NTOHS(sdphdr->len);//�����ж�
	//sdp_data_len=sdphdr->len;//�����ж�
	if(sdp_data_len % 2 == 0)
		sdp_data_pad_len=sdp_data_len;
	else
		sdp_data_pad_len=sdp_data_len + 1;//���λ

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
		if(inet_chksum(sdphdr,sdp_data_pad_len) != 0)  //У���
		{
			SSNP_DEBUG_PRINT("sdp_input():checksum failed.\n");
			pbuf_free(p);
			return ERR_VALUE;
		}
	}
  //#δ���#�����ﻹ��Ҫȥ��dp�����pad��checksum���ݣ�����ĺ�����ɴ˹�������
	pbuf_realloc(p,sdp_data_pad_len);//ȥ������Ĳ�����dp������


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
	if(!addr_check(&sdphdr->src,&sdphdr->dst)) //��ַ���
	{
		SSNP_DEBUG_PRINT("sdp_input():sdp address error.\n");
		pbuf_free(p);
		return ERR_VALUE;
	}

	/*
	 *  #δ���#���ǲ��ǹ㲥��ַ
	 *
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
	 src_alias=SDPH_SRC_ALIAS(sdphdr);
	 src_tdcn_num=SDPH_SRC_TDCN_NUM(sdphdr);
	 dst_alias=SDPH_DST_ALIAS(sdphdr);
	 dst_tdcn_num=SDPH_DST_TDCN_NUM(sdphdr);
	 if(pbuf_header(p,-SDP_HLEN))//#δ���#��������Ȼ������ͷ��ָ�룬ʹ��dataָ��Э�����ݣ��������ݳ���û��ȥ����������λ��У��λ
	 {                                      //������Ҫ����һ�£�ʹ�����ݳ���λ�������ݵĳ��ȣ������ͨ��dpЭ���Lengthλ����
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


//ע����ô˺���֮ǰpcbһ����bind��ɵ�
err_t sdp_connect(struct sdp_pcb* pcb,u8t r_alias,u8t r_tdcn_num)
{
	struct sdp_pcb* i;

	pcb->remote_alias=r_alias;
	pcb->remote_tdcn_num=r_tdcn_num;
	pcb->flag |= SDP_FLAG_CONNECT; //ÿһλ�����岻ͬ

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
	netif=ip_route(pcb->local_alias);//#δ���#�����ﻹ�����⣬Ҳ���������ip�����ȷ��ʹ����һ��netif�������ݴ��䡣
	if(netif == NULL)
		return ERR_ROUTE;
	//SSNP_DEBUG_PRINT("sdp_send():find netif.\r\n");
	return sdp_sendto_if(pcb,p,dst_alias,dst_tdcn_num,netif);
}


//����ĺ����������dp_output��һ���ģ�ȥ��һ����
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

	if(pbuf_header(p,SDP_HLEN))//#δ���#���Ƚ��������dp�����������м�ģ�����ͷ�����滹�����λ��У��ͣ�����������Ҫר��Ϊdp�޸�һ��
	{	//SSNP_DEBUG_PRINT("sdp_sendto_if():could not allocate header.\r\n");
		q=pbuf_alloc(PBUF_RAW,SDP_HLEN,PBUF_RAM);//���¼���һ����������dpЭ��ͷ��������
		if(q == NULL)
		{
			SSNP_DEBUG_PRINT("sdp_sendto_if():could not allocate header.\r\n");
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
			pbuf_free(q);//���������p��Ӧ�����ݲ�������Э����ɾ����Э����ֻ��ɾ����Э���ڲ����������
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
	sdphdr=(struct sdp_hdr*)q->data;//ע������Ӧ��ʹ��q������p
	
	SDPH_PAA_SET(sdphdr,protocol_id,ake,ack);
	SDPH_VPP_SET(sdphdr,version,psn,priority);
	sdphdr->len=HTONS(len);
	src=LINK(pcb->local_alias,pcb->local_tdcn_num);
	sdphdr->src.addr=HTONS(src);
	dst=LINK(dst_alias,dst_tdcn_num);
	sdphdr->dst.addr=HTONS(dst);
	sdphdr->sqn=seq_num;

	/*
	 *  #δ���#��������Ҫ����У��ͣ��������λ�ļ��㡣
	 *           ��dp_output������д�����
	 *           �����ع���ʱ����dp���ݲ���Ҫ������ô��ĺ���������д��һ�������С�
	 *
	 */
//	SSNP_DEBUG_PRINT("sdp_send():pass sdp data to data link.\r\n");
	err=netif->output(netif,q,dst_alias,ETHTYPE_SDP);//#δ���#�����������Ҳ��Ҫȷ��һ��
	
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