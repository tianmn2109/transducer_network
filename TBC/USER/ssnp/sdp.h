#ifndef SDP_H
#define SDP_H

/*
 *  注意这里的sdp不像udp那样存在多对多的连接需要要端口号，这里也有类似的
 *  端口号也就是TBIM内寻址，也就是地址的后8位，从这个角度来看dp和sdp有
 *  网络层和传输层的作用了，dp和sdp上面命令服务和命令回复已经算是应用层了，
 *  因此协议栈到此就为止了，比如dp上面的命令服务和命令回复已经没有地址位了。
 *  
 *  至于上面的command services 和 reply protocol就是应用层的事了。
 *
 *
 *  所以这里的协议非常简单就一层。也就是dp和sdp的地位是一样的，所以全部使用dp就可以了.
 */


  /*
   *  另一点就是sdp其实类似于tcp，比如这里的计时和时间片等等就需要在这一层实现了，这里的按照
   *  时间片发送不可以依赖于底层以太网按照时间片发送，因为这是这一层协议的任务，而底层的以太网
   *  接口只管把数据包发送出去，其他的不可以依赖于网络接口层。还有就是dp，sdp和tp都需要按照时间片
   *  发送，所以都需要管理时间.
   *
   *  只有其他特性比如优先级，这个是以太网没有的，那么这个特性就只能使用软件来模拟了。但是其他的
   *  一定要清楚是哪一层的任务。
   */

   /*
    *  #未完成#：但是这个时间片管理到底是哪一层的任务？也就是按照时间片发送到底是谁来实现的？这里
	*           按照时间片发送数据应该就是在网络接口层用软件来模拟，这个不应该是上层的任务。网络接口层
	*           是可以查看到数据类型的，所以这里上层协议发送的数据还不可以直接发送，应该使用队列集中管理
	*           起来进行发送(分类队列进行管理)。
	*  #未完成#：还有超时管理等是谁的任务？
	*
	*
    */

   /*
    *   #未完成#：另外一点从这里看dp和上面的command services与reply protocol是实现在一起的(不是这样的)，
	*            dp获得变送器号后需要调用上层的接收函数，上层的接收函数一定要有别名这一参数就可以了也就是
	*            这里实现的。
	*   
	*   #未完成#：另一点其实不需要标记上层协议，因为dp的input函数一定是一个命令消息，那么上层应用直接回复就可以了。
	*            因为上层应用知道从dp传上来的数据一定是一个命令(不对，也有可能是响应消息)。是不是这样的对于TBC
	*            来说就一定是回复，对于TBIM来说就一定是命令。也就是需要确定是不是只有TBC才可以发送命令。
	*    标准上说的很清楚了，这里对于command services协议确实就是TBC用来向TBIM发送命令的。TBIM并不能发送命令
	*    而是只能回复命令。那么这样就简单了。
	*    从这个角度来看dp和sdp的接收处理函数确实不同
    */

   /*
    *   #未完成#：比如udp并不假设上层传递的是什么数据。
	*   
	*
	*   #未完成#：TBIM应该如何填写目的ip地址，也就是TBIM该如何知道TBC的别名，应该是通过这样的方法，那就是TBC会
	*            首先向TBIM发送命令，这样TBIM就可以知道TBC的别名了
    */
   //http://www.iqiyi.com/dianshiju/lzxxdnzb.html
	 
	 
	 

#include "pbuf.h"
#include "err.h"


#define ETHTYPE_SDP       0x0800U  //arp协议 -> 数据包协议  

#define SDP_HLEN           10 
#define LINK_HLEN         14    

#ifdef PACK_STRUCT_USE_INCLUDES
#include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct sdp_addr//dp的地址格式
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



#define PROTO_DGP   0x0806 //本层协议支持的协议标示符
#define PROTO_SDP   0x0800U
#define PROTO_CSP   0x04
#define PROTO_RP    0x05

//将别名和变送器号拼接成ip地址
#define LINK(alias,tdcn_num) (((alias) << 8) | (tdcn_num))


struct sdp_pcb;


  /*
   *  这里的回调函数负责pbuf的释放.
   *  arg：用户定义的参数
   *  pbc：使用哪一个pcb来接收数据
   *  addr：远端ip地址，从此地址接收到的数据
   *  tdcn_num:类似于端口号，哪个tbim中的哪个传感器 
   */ 
typedef void (*sdp_recv_fn)(void* arg,struct sdp_pcb* pcb,struct pbuf* p,u8t src_alias,u8t src_tdcn_num,u8t tbim_tdcn_num);


   //#未完成#：时间片管理，按照时间片进行数据传输
  /*
   *  本来这里直接使用255的数组表示255个内部传感器就可以了
   *  但是这样会浪费较多的内存，并且考虑到每个TBIM中可能不会
   *  有太多的传感器因此这里使用链表就可以了。
   *
   */
#define SDP_FLAG_CONNECT  0x01

struct sdp_pcb
{
	struct sdp_pcb* next;

	u8t flag;

	u8t local_alias;//相当于本地ip(本地TBIM别名)
	u8t remote_alias;//远程ip(远端TBIM别名)

	u8t local_tdcn_num;//相当于本地端口(TBIM内传感器编号)
	u8t remote_tdcn_num;//远程端口

	sdp_recv_fn recv;//处理函数
	void* recv_arg;
};

//应用层需要使用的函数
struct netif;//防止相互包含

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
 *  #未完成#：这里的dp和sdp应该是一样的(至少在这一层上是相同的，对吗？) 只不过在pid和接收函数上有所差别
 *
 *
 *
 */
//注意这里就不可以在使用底层(数据链路层)的地址格式了
err_t sdp_output(struct pbuf* p,struct sdp_addr* src,struct sdp_addr* dst,struct netif* netif);

//底层协议需要使用的函数
err_t sdp_input(struct pbuf* p,struct netif* inp);
	 
	 
#endif
