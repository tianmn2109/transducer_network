#ifndef DP_H
#define DP_H

#include "pbuf.h"
#include "err.h"


#define ETHTYPE_DP        0x0806U   //arp协议 -> 数据包协议  

#define DP_HLEN           10 
#define LINK_HLEN         14    

#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct dp_addr//dp的地址格式
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
struct dp_hdr
{
	u8t pid_ake_ack;
	u8t v_psn_prt;
	u16t len;
	struct dp_addr src;
	struct dp_addr dst;
	u16t sqn ;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "epstruct.h"
#endif

#define DPH_PID(hdr) 						((hdr)->pid_ake_ack >> 2)
#define DPH_AKE(hdr) 						(((hdr)->pid_ake_ack >> 1) & 0x01)
#define DPH_ACK(hdr) 						((hdr)->pid_ake_ack & 0x01)
#define DPH_V(hdr)   						((hdr)->v_psn_prt >> 4 )
#define DPH_PSN(hdr) 						(((hdr)->v_psn_prt >> 3) & 0x01)
#define DPH_PRT(hdr) 						((hdr)->v_psn_prt & 0x07)
#define DPH_LEN(hdr) 						((hdr)->len)
#define DPH_SRC_ALIAS(hdr)  		(NTOHS((hdr)->src.addr) >> 8)
#define DPH_SRC_TDCN_NUM(hdr)   (NTOHS((hdr)->src.addr) & 0x00ff)
#define DPH_DST_ALIAS(hdr)      (NTOHS((hdr)->dst.addr) >> 8)
#define DPH_DST_TDCN_NUM(hdr)   (NTOHS((hdr)->dst.addr) & 0x00ff)
   

#define DPH_PAA_SET(hdr,pid,ake,ack)  (hdr)->pid_ake_ack = (((pid) << 2) | ((ake) << 1) | (ack))
#define DPH_VPP_SET(hdr,v,psn,prt)  (hdr)->v_psn_prt = (((v) << 4) | ((psn) << 3) | (prt))



#define PROTO_DGP   0x0806 //本层协议支持的协议标示符
#define PROTO_SDP   0x0800U
#define PROTO_CSP   0x04
#define PROTO_RP    0x05

//将别名和变送器号拼接成ip地址
#define LINK(alias,tdcn_num) (((alias) << 8) | (tdcn_num))


struct dp_pcb;



  /*
   *  这里的回调函数负责pbuf的释放.
   *  arg：用户定义的参数
   *  pbc：使用哪一个pcb来接收数据
   *  addr：远端ip地址，从此地址接收到的数据
   *  tdcn_num:类似于端口号，哪个tbim中的哪个传感器 
   */ 
typedef void (*dp_recv_fn)(void* arg,struct dp_pcb* pcb,struct pbuf* p,u8t src_alias,u8t src_tdcn_num,u8t tbim_tdcn_num);
//typedef void (*sdp_recv_fn)(void* arg,struct sdp_pcb* pcb,struct pbuf* p,u8t alias,u8t tdcn_num);

   //#未完成#：时间片管理，按照时间片进行数据传输
  /*
   *  本来这里直接使用255的数组表示255个内部传感器就可以了
   *  但是这样会浪费较多的内存，并且考虑到每个TBIM中可能不会
   *  有太多的传感器因此这里使用链表就可以了。
   *
   */
#define DP_FLAG_CONNECT  0x01

struct dp_pcb
{
	struct dp_pcb* next;

	u8t flag;

	u8t local_alias;//相当于本地ip(本地TBIM别名)
	u8t remote_alias;//远程ip(远端TBIM别名)

	u8t local_tdcn_num;//相当于本地端口(TBIM内传感器编号)
	u8t remote_tdcn_num;//远程端口

	dp_recv_fn recv;//处理函数
	void* recv_arg;
};

//typedef struct  dp_pcb sdp_pcb;
//struct sdp_pcb
//{
//	struct sdp_pcb* next;
//
//	u8t local_alias;//相当于本地ip(本地TBIM别名)
//	u8t remote_alias;//远程ip(远端TBIM别名)
//
//	u8t local_tdcn_num;//相当于本地端口(TBIM内传感器编号)
//	u8t remote_tdcn_num;//远程端口
//
//	sdp_recv_fn recv;
//	void* recv_arg;
//};

//应用层需要使用的函数
struct netif;//防止相互包含

#define dp_init()

struct dp_pcb*   dp_new(void);
err_t            dp_bind(struct dp_pcb* pcb,u8t l_alias,u8t l_tdcn_num);
void             dp_remove(struct dp_pcb* pcb);
err_t            dp_connect(struct dp_pcb* pcb,u8t r_alias,u8t r_tdcn_num);
void             dp_disconnect(struct dp_pcb* pcb);
void             dp_recv(struct dp_pcb* pcb,dp_recv_fn recv,void* recv_arg);
err_t            dp_send(struct dp_pcb* pcb,struct pbuf* p);
err_t            dp_sendto(struct dp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num);
err_t            dp_sendto_if(struct dp_pcb* pcb,struct pbuf* p,u8t dst_alias,u8t dst_tdcn_num,struct netif* netif);


//#define sdp_init()
//#define sdp_new()                dp_new()
//#define sdp_bind(p,a,n)          dp_bind(p,a,n)
//#define sdp_remove(p)            dp_remove(p)
//#define sdp_connect(p,a,n)       dp_connect(p,a,n)
//#define sdp_disconnect(p)        dp_connect(p)
//#define sdp_recv(p,r,a)          dp_recv(p,r,a)
//#define sdp_send(p,b)            dp_send(p,b)
//#define sdp_sendto(p,b,a,d)      dp_send(p,b,a,d)
//#define sdp_sendto_if(p,b,a,d,n) dp_sendto_if(p,b,a,d,n)

/*
 *  #未完成#：这里的dp和sdp应该是一样的(至少在这一层上是相同的，对吗？) 只不过在pid和接收函数上有所差别
 *
 *
 *
 */








//注意这里就不可以在使用底层(数据链路层)的地址格式了
err_t dp_output(struct pbuf* p,struct dp_addr* src,struct dp_addr* dst,struct netif* netif);

//底层协议需要使用的函数
err_t dp_input(struct pbuf* p,struct netif* inp);


#endif
