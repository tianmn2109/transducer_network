#ifndef TP_H
#define TP_H


#include "pbuf.h"
#include "err.h"
#include "netif.h"

#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct tp_addr//tp的地址格式
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
struct tp_hdr
{
	u8t pid_ake_ack;
	u8t v_sum_prt;
	struct tp_addr src;
	struct tp_addr dst;
	u16t chksum ;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif

#define TPH_PID(hdr) 						((hdr)->pid_ake_ack >> 2)
#define TPH_AKE(hdr) 						(((hdr)->pid_ake_ack >> 1) & 0x01)
#define TPH_ACK(hdr) 						((hdr)->pid_ake_ack & 0x01)
#define TPH_V(hdr)  						((hdr)->v_sum_prt >> 4 )
#define TPH_SUM(hdr) 						(((hdr)->v_sum_prt >> 3) & 0x01)
#define TPH_PRT(hdr) 						((hdr)->v_sum_prt & 0x07)
#define TPH_SRC_ALIAS(hdr)  		(NTOHS((hdr)->src.addr) >> 8)
#define TPH_SRC_TDCN_NUM(hdr)   (NTOHS((hdr)->src.addr) & 0x00ff)
#define TPH_DST_ALIAS(hdr)      (NTOHS((hdr)->dst.addr) >> 8)
#define TPH_DST_TDCN_NUM(hdr)   (NTOHS((hdr)->dst.addr) & 0x00ff)


#define TPH_PAA_SET(hdr,pid,ake,ack)  (hdr)->pid_ake_ack = (((pid) << 2) | ((ake) << 1) | (ack))
#define TPH_VPP_SET(hdr,v,sum,prt)  (hdr)->v_sum_prt = (((v) << 4) | ((sum) << 3) | (prt))
#define LINK(alias,tdcn_num) (((alias) << 8) | (tdcn_num))


#define TRIGGER_PROTOCOL 0x03
#define TG_LEN           8
#define LINK_LEN         14
#define TP_FLAG_CONNECT  0x01
struct tp_pcb;
typedef void (*tp_recv_fn)(void* arg,struct tp_pcb* pcb,u8t alias,u8t tdcn_num,u8t tbim_tdcn_num);
struct tp_pcb
{
	struct tp_pcb* next;

	u8t flag;

	u8t local_alias;//相当于本地ip(本地TBIM别名)
	u8t remote_alias;//远程ip(远端TBIM别名)

	u8t local_tdcn_num;//相当于本地端口(TBIM内传感器编号)
	u8t remote_tdcn_num;//远程端口

	tp_recv_fn recv;//处理函数
	void* recv_arg;
};

#define tp_init()

struct tp_pcb* tp_new(void);
err_t tp_bind(struct tp_pcb* pcb,u8t l_alias,u8t l_tdcn_num);
void  tp_remove(struct tp_pcb* pcb);
err_t tp_connect(struct tp_pcb* pcb,u8t r_alias,u8t r_tdcn_num);
void tp_disconnect(struct tp_pcb* pcb);
void tp_recv(struct tp_pcb* pcb,tp_recv_fn recv,void* recv_arg);

err_t tp_send(struct tp_pcb* pcb,struct pbuf* p);
err_t tp_sendto(struct tp_pcb* pcb,struct pbuf* p,u8t alias,u8t tdcn_num);
err_t tp_sendto_if(struct tp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t tdcn_num,struct netif* netif);
err_t tp_input(struct pbuf* p,struct netif* inp);
#endif
