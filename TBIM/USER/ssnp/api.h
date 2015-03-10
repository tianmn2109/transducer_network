#ifndef API_H
#define API_H


#include "netbuf.h"
#include "sys.h"
#include "err.h"
#include "config.h"
enum netconn_type
{
	NETCONN_INVSLID=0,
	NETCONN_DP,
	NETCONN_SDP,
	NETCONN_TP
};

struct dp_pcb;
struct sdp_pcb;
struct tp_pcb;
struct api_msg_arg;


struct netconn//网络连接描述符
{
	enum netconn_type type;
	union
	{
		struct dp_pcb* dp;
		struct sdp_pcb* sdp;
		struct tp_pcb* tp;
	}pcb;

	err_t last_err;
	sys_sem_t op_completed;
	sys_mbox_t recvmbox;
	u8t flags;
};


struct netconn* netconn_new(enum netconn_type t);
err_t netconn_delete(struct netconn* conn);
err_t netconn_bind(struct netconn* conn,u8t alias,u8t tdcn_num);
err_t netconn_connect(struct netconn* conn,u8t alias,u8t tdcn_num);
err_t netconn_disconnect(struct netconn* conn);
err_t netconn_sendtrigger(struct netconn* conn,u8t tbim_tdcn_num,u8t alias,u8t tdcn_num);
err_t netconn_recvtrigger(struct netconn* conn,u8t* alias,u8t* tdcn_num);
err_t netconn_recvtrigger_unblock(struct netconn* conn,u8t* alias,u8t* tdcn_num);
err_t netconn_recvtrigger_buf(struct netconn* conn,struct netbuf** buf);//由于trigger并不携带数据，只有msg来源，其中来源保存此函数的参数buf中，也可以使用上面的函数
err_t netconn_sendto(struct netconn* conn,struct netbuf* buf,u8t alias,u8t tdcn_num);
err_t netconn_send(struct netconn* conn,struct netbuf* buf);//如果使用此函数那么就要完全设置好netbuf中的三个端口和ip参数
err_t netconn_recv(struct netconn* conn,struct netbuf** new_buf);//注意这里需要接收一个指针，就好比我们需要接收一个整数需要使用int* a,一样那么接收一个整数指针则需要使用int** a 
err_t netconn_recv_unblock(struct netconn* conn,struct netbuf** new_buf);//非阻塞式接收

#if NODE == TBIM
u8t get_alias(void);

#endif

#if NODE == TBC
void* get_tbim(void);
void set_connection_recvmbox(struct netconn* conn,sys_mbox_t mbox);
#endif

#endif
