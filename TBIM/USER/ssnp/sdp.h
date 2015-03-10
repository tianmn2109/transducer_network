#ifndef SDP_H
#define SDP_H

/*
 *  ע�������sdp����udp�������ڶ�Զ��������ҪҪ�˿ںţ�����Ҳ�����Ƶ�
 *  �˿ں�Ҳ����TBIM��Ѱַ��Ҳ���ǵ�ַ�ĺ�8λ��������Ƕ�����dp��sdp��
 *  �����ʹ����������ˣ�dp��sdp����������������ظ��Ѿ�����Ӧ�ò��ˣ�
 *  ���Э��ջ���˾�Ϊֹ�ˣ�����dp�����������������ظ��Ѿ�û�е�ַλ�ˡ�
 *  
 *  ���������command services �� reply protocol����Ӧ�ò�����ˡ�
 *
 *
 *  ���������Э��ǳ��򵥾�һ�㡣Ҳ����dp��sdp�ĵ�λ��һ���ģ�����ȫ��ʹ��dp�Ϳ�����.
 */


  /*
   *  ��һ�����sdp��ʵ������tcp����������ļ�ʱ��ʱ��Ƭ�ȵȾ���Ҫ����һ��ʵ���ˣ�����İ���
   *  ʱ��Ƭ���Ͳ����������ڵײ���̫������ʱ��Ƭ���ͣ���Ϊ������һ��Э������񣬶��ײ����̫��
   *  �ӿ�ֻ�ܰ����ݰ����ͳ�ȥ�������Ĳ���������������ӿڲ㡣���о���dp��sdp��tp����Ҫ����ʱ��Ƭ
   *  ���ͣ����Զ���Ҫ����ʱ��.
   *
   *  ֻ���������Ա������ȼ����������̫��û�еģ���ô������Ծ�ֻ��ʹ�������ģ���ˡ�����������
   *  һ��Ҫ�������һ�������
   */

   /*
    *  #δ���#���������ʱ��Ƭ����������һ�������Ҳ���ǰ���ʱ��Ƭ���͵�����˭��ʵ�ֵģ�����
	*           ����ʱ��Ƭ��������Ӧ�þ���������ӿڲ��������ģ�⣬�����Ӧ�����ϲ����������ӿڲ�
	*           �ǿ��Բ鿴���������͵ģ����������ϲ�Э�鷢�͵����ݻ�������ֱ�ӷ��ͣ�Ӧ��ʹ�ö��м��й���
	*           �������з���(������н��й���)��
	*  #δ���#�����г�ʱ�������˭������
	*
	*
    */

   /*
    *   #δ���#������һ������￴dp�������command services��reply protocol��ʵ����һ���(����������)��
	*            dp��ñ������ź���Ҫ�����ϲ�Ľ��պ������ϲ�Ľ��պ���һ��Ҫ�б�����һ�����Ϳ�����Ҳ����
	*            ����ʵ�ֵġ�
	*   
	*   #δ���#����һ����ʵ����Ҫ����ϲ�Э�飬��Ϊdp��input����һ����һ��������Ϣ����ô�ϲ�Ӧ��ֱ�ӻظ��Ϳ����ˡ�
	*            ��Ϊ�ϲ�Ӧ��֪����dp������������һ����һ������(���ԣ�Ҳ�п�������Ӧ��Ϣ)���ǲ��������Ķ���TBC
	*            ��˵��һ���ǻظ�������TBIM��˵��һ�������Ҳ������Ҫȷ���ǲ���ֻ��TBC�ſ��Է������
	*    ��׼��˵�ĺ�����ˣ��������command servicesЭ��ȷʵ����TBC������TBIM��������ġ�TBIM�����ܷ�������
	*    ����ֻ�ܻظ������ô�����ͼ��ˡ�
	*    ������Ƕ�����dp��sdp�Ľ��մ�����ȷʵ��ͬ
    */

   /*
    *   #δ���#������udp���������ϲ㴫�ݵ���ʲô���ݡ�
	*   
	*
	*   #δ���#��TBIMӦ�������дĿ��ip��ַ��Ҳ����TBIM�����֪��TBC�ı�����Ӧ����ͨ�������ķ������Ǿ���TBC��
	*            ������TBIM�����������TBIM�Ϳ���֪��TBC�ı�����
    */
   //http://www.iqiyi.com/dianshiju/lzxxdnzb.html
	 
	 
	 

#include "pbuf.h"
#include "err.h"


#define ETHTYPE_SDP       0x0800U  //arpЭ�� -> ���ݰ�Э��  

#define SDP_HLEN           10 
#define LINK_HLEN         14    

#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct sdp_addr//dp�ĵ�ַ��ʽ
{
	u16t addr;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif



#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct sdp_hdr
{
	u8t pid_ake_ack;
	u8t v_psn_prt;
	u16t len;
	struct sdp_addr src;
	struct sdp_addr dst;
	u16t sqn ;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif

#define SDPH_PID(hdr) 						((hdr)->pid_ake_ack >> 2)
#define SDPH_AKE(hdr) 						(((hdr)->pid_ake_ack >> 1) & 0x01)
#define SDPH_ACK(hdr) 						((hdr)->pid_ake_ack & 0x01)
#define SDPH_V(hdr)   						((hdr)->v_psn_prt >> 4 )
#define SDPH_PSN(hdr) 						(((hdr)->v_psn_prt >> 3) & 0x01)
#define SDPH_PRT(hdr) 						((hdr)->v_psn_prt & 0x07)
#define SDPH_LEN(hdr) 						((hdr)->len)
#define SDPH_SRC_ALIAS(hdr)  		 (NTOHS((hdr)->src.addr) >> 8)
#define SDPH_SRC_TDCN_NUM(hdr)   (NTOHS((hdr)->src.addr) & 0x00ff)
#define SDPH_DST_ALIAS(hdr)      (NTOHS((hdr)->dst.addr) >> 8)
#define SDPH_DST_TDCN_NUM(hdr)   (NTOHS((hdr)->dst.addr) & 0x00ff)
#define SDPH_PAA_SET(hdr,pid,ake,ack)  (hdr)->pid_ake_ack = (((pid) << 2) | ((ake) << 1) | (ack))
#define SDPH_VPP_SET(hdr,v,psn,prt)  (hdr)->v_psn_prt = (((v) << 4) | ((psn) << 3) | (prt))



#define PROTO_DGP   0x0806 //����Э��֧�ֵ�Э���ʾ��
#define PROTO_SDP   0x0800U
#define PROTO_CSP   0x04
#define PROTO_RP    0x05

//�������ͱ�������ƴ�ӳ�ip��ַ
#define LINK(alias,tdcn_num) (((alias) << 8) | (tdcn_num))


struct sdp_pcb;


  /*
   *  ����Ļص���������pbuf���ͷ�.
   *  arg���û�����Ĳ���
   *  pbc��ʹ����һ��pcb����������
   *  addr��Զ��ip��ַ���Ӵ˵�ַ���յ�������
   *  tdcn_num:�����ڶ˿ںţ��ĸ�tbim�е��ĸ������� 
   */ 
typedef void (*sdp_recv_fn)(void* arg,struct sdp_pcb* pcb,struct pbuf* p,u8t src_alias,u8t src_tdcn_num,u8t tbim_tdcn_num);


   //#δ���#��ʱ��Ƭ��������ʱ��Ƭ�������ݴ���
  /*
   *  ��������ֱ��ʹ��255�������ʾ255���ڲ��������Ϳ�����
   *  �����������˷ѽ϶���ڴ棬���ҿ��ǵ�ÿ��TBIM�п��ܲ���
   *  ��̫��Ĵ������������ʹ������Ϳ����ˡ�
   *
   */
#define SDP_FLAG_CONNECT  0x01

struct sdp_pcb
{
	struct sdp_pcb* next;

	u8t flag;

	u8t local_alias;//�൱�ڱ���ip(����TBIM����)
	u8t remote_alias;//Զ��ip(Զ��TBIM����)

	u8t local_tdcn_num;//�൱�ڱ��ض˿�(TBIM�ڴ��������)
	u8t remote_tdcn_num;//Զ�̶˿�

	sdp_recv_fn recv;//������
	void* recv_arg;
};

//Ӧ�ò���Ҫʹ�õĺ���
struct netif;//��ֹ�໥����

#define sdp_init()

struct sdp_pcb*   sdp_new(void);
err_t            sdp_bind(struct sdp_pcb* pcb,u8t l_alias,u8t l_tdcn_num);
void             sdp_remove(struct sdp_pcb* pcb);
err_t            sdp_connect(struct sdp_pcb* pcb,u8t r_alias,u8t r_tdcn_num);
void             sdp_disconnect(struct sdp_pcb* pcb);
void             sdp_recv(struct sdp_pcb* pcb,sdp_recv_fn recv,void* recv_arg);
err_t            sdp_send(struct sdp_pcb* pcb,struct pbuf* p);
err_t            sdp_sendto(struct sdp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num);
err_t            sdp_sendto_if(struct sdp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num,struct netif* netif);
/*
 *  #δ���#�������dp��sdpӦ����һ����(��������һ��������ͬ�ģ�����) ֻ������pid�ͽ��պ������������
 *
 *
 *
 */
//ע������Ͳ�������ʹ�õײ�(������·��)�ĵ�ַ��ʽ��
err_t sdp_output(struct pbuf* p,struct sdp_addr* src,struct sdp_addr* dst,struct netif* netif);

//�ײ�Э����Ҫʹ�õĺ���
err_t sdp_input(struct pbuf* p,struct netif* inp);
	 
	 
#endif
