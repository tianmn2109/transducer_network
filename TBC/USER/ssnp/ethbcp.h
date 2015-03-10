#ifndef ETHBCP_H
#define ETHBCP_H


#include "err.h"
#include "pbuf.h"
#include "config.h"

#ifndef ETHBCP_HWADDR_LEN
#define ETHBCP_HWADDR_LEN     6   //Ӳ����ַ����
#endif

#ifndef ETH_PAD_SIZE
#define ETH_PAD_SIZE          0  //���λ����
#endif


#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct eth_addr
{
	u8t addr[ETHBCP_HWADDR_LEN];  //mac��ַ����
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif

#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct eth_hdr                 //��̫��֡����
{
#if ETH_PAD_SIZE
	u8t padding[ETH_PAD_SIZE];
#endif
	struct eth_addr dest;
	struct eth_addr src;
	u16t type;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif


#define SIZEOF_ETH_HDR (14+ETH_PAD_SIZE) //��̫��֡ͷ����



#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct ethbcp_hdr                      //short format link control frame:���ڳ�ʼ���ý׶ε���Ϣ����
{
	struct eth_addr eth_dest_addr;
	struct eth_addr eth_src_addr;
	u16t   eth_type;
	u8t    SSType;
	u8t    SSLength;
	u8t    SSVersion;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif

#define SIZEOF_EHTBCP_HDR    17   //short format link control frameͷ������

//ע������ETHTYPE_BCP����ȷ�����ͣ����������ֱ���arp��ip��vlan������
#define ETHTYPE_BCP       0x0802U   //ethertype ���� ע�������������Ϊ��̫��Э������
#define ETHTYPE_DP        0x0806U   //arpЭ�� -> ���ݰ�Э��  
#define ETHTYPE_SDP       0x0800U   //ipЭ��  ->������Э��
#define ETHTYPE_TP        0x8100U   //vlanЭ��->trigger


struct netif;//��ֹ�ظ�����

//����������Ϣreply����,����������Ĵ���Ƚϼ򵥡�����Ҳ���Ǻ�arp�����й̶�������(����Ҳ��)
//�������ʹ���Ǻ���Ĺ�����
err_t ethbcp_output(struct netif* netif,struct pbuf* p,u8t dst_alias,u16t type);


//ssnpЭ��ջ��ȫ�����ݽ��մ�����,Ҳ������̫��������·��Ľ��պ���
err_t ethernet_input(struct pbuf* p,struct netif* netif);


//******************************************TBIM���д���************************************************************
#if NODE == TBIM
void TBIM_DLL_init(void);
u8t getalias(void);
u8t is_streaming_ok(void);
u8t is_assign_time_slot_done(void);
void get_syn_asyn_begin_timeslot(u16t* syn,u16t* asyn,u16t* begin_timeslot);
u8t is_reflect_ok(void);
err_t send_reflect_reply_msg(struct netif* netif);
void set_reflect_ok_unfinished_TEST(void);
#endif
//******************************************************************************************************************



//******************************************TBC���д���************************************************************
#if NODE == TBC
void TBC_DLL_init(void);
u8t get_alias_alloc_state(void);
void* get_done_list(void);
void set_alias_alloc_done(void);
err_t InitiateDiscovery(struct netif* netif);
err_t send_assign_time_slot_msg(struct netif* netif,u8t alias,u8t tbim_tdcn_num,u16t beign_time_slot,u8t time_slot_num);
err_t send_define_epoch_msg(struct netif* netif,u16t syn_timeslot_num,u16t asyn_timeslot_num);
err_t send_begin_of_epoch_msg(struct netif* netif);
err_t send_reflect_msg(struct netif* netif,u8t alias);
u8t is_reflect_reply_ok(void);
void set_reflect_reply_unfinished(void);
#endif
//******************************************************************************************************************

#endif
