
#include "api_msg.h"
#include "dp.h"
#include "sdp.h"
#include "tp.h"
#include "memp.h"
#include "ssnp.h"
#include "debug.h"
static void recv_dp(void* arg,struct dp_pcb* pcb,struct pbuf* p,u8t alias,u8t tdcn_num,u8t tbim_tdcn_num)
{
	struct netbuf* buf;
	struct netconn* conn;

	conn=(struct netconn*)arg;
	if((conn == NULL) || (conn->recvmbox == SYS_MBOX_NULL))
	{
		pbuf_free(p);
		return;
	}

	buf=(struct netbuf*)memp_alloc(MEMP_NETBUF);
	if(buf == NULL)
	{
		pbuf_free(p);  
		return;
	}
	else
	{
		buf->p=p;
		buf->alias=alias;
		buf->tdcn_num=tdcn_num;
		buf->tbim_tdcn_num=tbim_tdcn_num;
	}

	if(sys_mbox_trypost(conn->recvmbox,buf) != ERR_OK)//这里使用trypost是为了防止可能完全阻塞掉协议栈线程
	{
		netbuf_delete(buf);
		return;
	}
}
static void recv_sdp(void* arg,struct sdp_pcb* pcb,struct pbuf* p,u8t alias,u8t tdcn_num,u8t tbim_tdcn_num)
{
	struct netbuf* buf;
	struct netconn* conn;

	conn=(struct netconn*)arg;
	if((conn == NULL) || (conn->recvmbox == SYS_MBOX_NULL))
	{
		SSNP_DEBUG_PRINT("recv_sdp():error:conn is NULL or mbox is NULL.\r\n");
		pbuf_free(p);
		return;
	}

	buf=(struct netbuf*)memp_alloc(MEMP_NETBUF);
	if(buf == NULL)
	{
		SSNP_DEBUG_PRINT("recv_sdp():buf is NULL.\r\n");
		pbuf_free(p);  
		return;
	}
	else
	{
		buf->p=p;
		buf->alias=alias;
		buf->tdcn_num=tdcn_num;
		buf->tbim_tdcn_num=tbim_tdcn_num;
	}

	if(sys_mbox_trypost(conn->recvmbox,buf) != ERR_OK)//这里使用trypost是为了防止可能完全阻塞掉协议栈线程
	{
	//	SSNP_DEBUG_PRINT("recv_sdp():failed to post sdp data to mbox.\r\n");
		netbuf_delete(buf);
		return;
	}
}

static void recv_tp(void* arg,struct tp_pcb* pcb,u8t alias,u8t tdcn_num,u8t tbim_tdcn_num)
{
	struct netconn* conn;
	struct netbuf* buf;

	conn=(struct netconn*)arg;
	if((conn == NULL) || (conn->recvmbox == SYS_MBOX_NULL))
		return;

	buf=(struct netbuf*)memp_alloc(MEMP_NETBUF);
	if(buf == NULL)
	{
		return;
	}
	else
	{
		buf->p=NULL;
		buf->alias=alias;
		buf->tdcn_num=tdcn_num;
		buf->tbim_tdcn_num=tbim_tdcn_num;
	}

	if(sys_mbox_trypost(conn->recvmbox,buf) != ERR_OK)//这里使用trypost是为了防止可能完全阻塞掉协议栈线程
	{
		netbuf_delete(buf);
		return;
	}
}

static void pcb_new(struct api_msg_arg* arg)
{
	switch(arg->conn->type)
	{
	case NETCONN_DP:
		{
			arg->conn->pcb.dp=dp_new();
			if(arg->conn->pcb.dp == NULL)
			{
				arg->err=ERR_MEMORY;
				break;
			}
			dp_recv(arg->conn->pcb.dp,recv_dp,arg->conn);
			break;
		}

	case NETCONN_SDP:
		{
			arg->conn->pcb.sdp=sdp_new();
			if(arg->conn->pcb.sdp == NULL)
			{
				arg->err=ERR_MEMORY;
				break;
			}
			sdp_recv(arg->conn->pcb.sdp,recv_sdp,arg->conn);
			break;
		}
	case NETCONN_TP:
		{
			arg->conn->pcb.tp=tp_new();
			if(arg->conn->pcb.tp == NULL)
			{
				arg->err=ERR_MEMORY;
				break;
			}
			tp_recv(arg->conn->pcb.tp,recv_tp,arg->conn);
			break;
		}
	default:
		{
			arg->err=ERR_VALUE;
			break;
		}
	}
}

void do_newconn(struct api_msg_arg* arg)
{
	arg->err=ERR_OK;
	if(arg->conn->pcb.dp == NULL)
		pcb_new(arg);//结构已经在arg->err中了

	SSNP_APIMSG_ACK(arg);
}

void do_bind(struct api_msg_arg* arg)
{
	if(ERR_IS_FATAL(arg->conn->last_err))//上一个api函数出现严重错误，那么当前api函数就不要再调用内核了
	{
		arg->err=arg->conn->last_err;
	}
	else
	{
		arg->err=ERR_VALUE;
		if(arg->conn->pcb.dp != NULL)//这里比较有意思
		{
			switch(arg->conn->type)
			{
			case NETCONN_DP:
				{
					arg->err=dp_bind(arg->conn->pcb.dp,arg->arg.bc.alias,arg->arg.bc.tdsn_num);
					break;
				}
			case NETCONN_SDP:
				{ 
					arg->err=sdp_bind(arg->conn->pcb.sdp,arg->arg.bc.alias,arg->arg.bc.tdsn_num);
					break;
				}
			case NETCONN_TP:
				{
					arg->err=tp_bind(arg->conn->pcb.tp,arg->arg.bc.alias,arg->arg.bc.tdsn_num);
					break;
				}
			default:
				break;
			}
		}
	}
	SSNP_APIMSG_ACK(arg);
}

void do_connect(struct api_msg_arg* arg)
{
	if(arg->conn->pcb.dp == NULL)
	{
		arg->err=ERR_CLOSED;
	}
	else
	{
		switch(arg->conn->type)
		{
		case NETCONN_DP: 
			{
				arg->err=dp_connect(arg->conn->pcb.dp,arg->arg.bc.alias,arg->arg.bc.tdsn_num);
				break;
			}
		case NETCONN_SDP:
		  {
				arg->err=sdp_connect(arg->conn->pcb.sdp,arg->arg.bc.alias,arg->arg.bc.tdsn_num);
				break;
			}
		case NETCONN_TP:
			{
				arg->err=tp_connect(arg->conn->pcb.tp,arg->arg.bc.alias,arg->arg.bc.tdsn_num);
				break;
			}
		default:
			{
				arg->err=ERR_VALUE;
				break;
			}
		}
	}
	sys_sem_signal(arg->conn->op_completed);
}

void do_disconnect(struct api_msg_arg* arg)
{//因为前面做过netconn是否为空的判断了，所以这里不用再次判断
	switch(arg->conn->type)
	{
	case NETCONN_DP:
		{
			dp_disconnect(arg->conn->pcb.dp);
			arg->err=ERR_OK;
			break;
		}
	case NETCONN_SDP:
		{
			sdp_disconnect(arg->conn->pcb.sdp);
			arg->err=ERR_OK;
			break;
		}
	case NETCONN_TP:
		{
			tp_disconnect(arg->conn->pcb.tp);
			arg->err=ERR_OK;
			break;
		}
	default:
		{
			arg->err=ERR_VALUE;
			break;
		}
	}
	SSNP_APIMSG_ACK(arg);
}

void do_send(struct api_msg_arg* arg)
{
	if(ERR_IS_FATAL(arg->conn->last_err))
	{
		arg->err=arg->conn->last_err;
	}
	else
	{
		arg->err=ERR_NOTCONNECT;
		if(arg->conn->pcb.dp != NULL)
		{
			switch(arg->conn->type)
			{
			case NETCONN_DP:
				{	 
				//	SSNP_DEBUG_PRINT("do_send():ssnp thread send.\r\n");
					arg->conn->pcb.dp->local_tdcn_num=arg->arg.b->tbim_tdcn_num;//因为dp的pcb中的端口号开始没有绑定，因此发送的时候指定
					arg->err=dp_sendto(arg->conn->pcb.dp,arg->arg.b->p,arg->arg.b->alias,arg->arg.b->tdcn_num);
					break;
				}
			case NETCONN_SDP:
				{
					SSNP_DEBUG_PRINT("do_send():ssnp thread send.\r\n");
					arg->conn->pcb.sdp->local_tdcn_num=arg->arg.b->tbim_tdcn_num;//因为dp的pcb中的端口号开始没有绑定，因此发送的时候指定
					arg->err=sdp_sendto(arg->conn->pcb.sdp,arg->arg.b->p,arg->arg.b->alias,arg->arg.b->tdcn_num);
					break;
				}
			case NETCONN_TP:
				{
					arg->conn->pcb.tp->local_tdcn_num=arg->arg.b->tbim_tdcn_num;
					arg->err=tp_sendto(arg->conn->pcb.tp,NULL,arg->arg.b->alias,arg->arg.b->tdcn_num);//注意发送trigger是不需要数据的
					break;
				}
			default:
				break;
			}
		}
	}
	SSNP_APIMSG_ACK(arg);
}

/*
 *  删除recvmbox和里面的数据
 *
 *
 *
 */
static void netconn_drain(struct netconn* conn)
{
	void* mem;

	if(conn->recvmbox != SYS_MBOX_NULL)
	{
		while(sys_mbox_tryfetch(conn->recvmbox,&mem) != SYS_MBOX_EMPTY)
		{
			netbuf_delete((struct netbuf*)mem);
		}
		sys_mbox_free(conn->recvmbox);
		conn->recvmbox=SYS_MBOX_NULL;
	}


}
/*
 * 删除netconn的pcb信息
 */
void do_delconn(struct api_msg_arg* arg)
{
	netconn_drain(arg->conn);

	if(arg->conn->pcb.dp != NULL)
	{
		switch(arg->conn->type)
		{
		case NETCONN_DP:
			{
				arg->conn->pcb.dp->recv_arg = NULL;
				dp_remove(arg->conn->pcb.dp);//注意pcb是在此函数中删除的
				break;
			}
		case NETCONN_SDP:
			{
				arg->conn->pcb.sdp->recv_arg = NULL;
				sdp_remove(arg->conn->pcb.sdp);//注意pcb是在此函数中删除的
				break;
			}
		case NETCONN_TP:
			{
				arg->conn->pcb.tp->recv_arg=NULL;
				tp_remove(arg->conn->pcb.tp);
				break;
			}
		default:
			break;
		}
		arg->conn->pcb.dp=NULL;
	}

	if(arg->conn->op_completed != SYS_SEM_NULL)
		sys_sem_signal(arg->conn->op_completed);
}
#if NODE == TBIM
void do_getalias(struct api_msg_arg* arg)
{
	arg->arg.ga.alias=getalias();
	sys_sem_signal(arg->arg.ga.op_completed);
}
#endif

#if NODE == TBC
void do_get_alias_alloc_state(struct api_msg_arg* arg)
{
	arg->arg.gl.alias_alloc_done=get_alias_alloc_state();
	arg->arg.gl.alias_list=get_done_list();
	sys_sem_signal(arg->arg.gl.op_completed);
}
#endif
