#ifndef ETHBCP_H
#define ETHBCP_H


#include "err.h"
#include "pbuf.h"
#include "config.h"

#ifndef ETHBCP_HWADDR_LEN
#define ETHBCP_HWADDR_LEN     6   //硬件地址长度
#endif

#ifndef ETH_PAD_SIZE
#define ETH_PAD_SIZE          0  //填充位长度
#endif


#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct eth_addr
{
	u8t addr[ETHBCP_HWADDR_LEN];  //mac地址定义
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif

#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct eth_hdr                 //以太网帧定义
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


#define SIZEOF_ETH_HDR (14+ETH_PAD_SIZE) //以太网帧头长度



#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct ethbcp_hdr                      //short format link control frame:用于初始配置阶段的信息传输
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

#define SIZEOF_EHTBCP_HDR    17   //short format link control frame头部长度

//注意下面ETHTYPE_BCP是正确的类型，其他三个分别是arp、ip、vlan的类型
#define ETHTYPE_BCP       0x0802U   //ethertype 类型 注意这里必须设置为以太网协议类型
#define ETHTYPE_DP        0x0806U   //arp协议 -> 数据包协议  
#define ETHTYPE_SDP       0x0800U   //ip协议  ->数据流协议
#define ETHTYPE_TP        0x8100U   //vlan协议->trigger


struct netif;//防止重复包含

//总线配置信息reply函数,总体上这里的处理比较简单。这里也不是和arp类似有固定的流程(流程也有)
//至于如何使用是后面的工作。
err_t ethbcp_output(struct netif* netif,struct pbuf* p,u8t dst_alias,u16t type);


//ssnp协议栈的全部数据接收处理函数,也就是以太网数据链路层的接收函数
err_t ethernet_input(struct pbuf* p,struct netif* netif);


//******************************************TBIM特有代码************************************************************
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



//******************************************TBC特有代码************************************************************
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
