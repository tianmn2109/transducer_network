#include "api_msg.h"
#include "ssnp.h"
#include "memp.h"
#include "mem.h"
#include "debug.h"

/*
-------------------------------------------------------------------------------------
      注意对于TBC来说，DP和TP的conn是没有必要带有recvmbox的，因为TBC不会接受命令和trigger，
	  TBC本身就是客户端，只有SDP需要带有recvmbox，用于接收流数据，但是没有弄清楚这里为什么
	  最多只能调用四次OSQCreate()，再继续调用就返回空值，下面就有两种方法了：
	  A:一种是弄清楚为什么？
	  B:再有干脆所有sdp连接都使用同一个mbox，这样在同步时间内不断轮训这一个mbox就可以了,
	    也不用各个mbox之间来回切换了，同时查询的方式也更加的简单了。恩，这种方法更加
		好一些。
	 实际上我们可以自己来设计一个队列，这里主要考虑到TBC除了这几个连接之外还需要一个mbox用来
	 接收上位机发送的命令，考虑多了，主线程一个，sdp一个，上位机接受命令一个，dp的命令接收也是需要的。足够了。
	 而且所有的dp连接使用一个mbox也是可以的，因为每次都和一个TBIM进行通信。
	 对于TBIM来说是SDP是没有必要带有mbox的。
	 我们就按照上面的思路执行，首先我们就这样，先把前面的流程跑通了。
-------------------------------------------------------------------------------------
*/
static struct netconn* netconn_alloc(enum netconn_type t)
{
	struct netconn* conn;
	u32t size;

	//conn=(struct netconn*)memp_alloc(MEMP_NETCONN);
	conn=(struct netconn*)mem_alloc(sizeof(struct netconn));
	if(conn == NULL)
	{
		SSNP_DEBUG_PRINT("netconn_alloc():alloc error NULL.\r\n");
		return NULL;
	}

	conn->last_err=ERR_OK;
	conn->type=t;
	conn->pcb.dp=NULL;

	switch(t)
	{
	case NETCONN_DP:
		{
			size=DEFAULT_DP_RECVMBOX_SIZE;
			break;
		}
	case NETCONN_SDP:
		{
			size=DEFAULT_SDP_RECVMBOX_SIZE;
			break;
		}
	case NETCONN_TP:
		{
			size=DEFAULT_TP_RECVMBOX_SIZE;
			break;
		}
	default:
		{	printf("netconn_alloc():unknown netconn type.\r\n");
			memp_free(MEMP_NETCONN,conn);
			return NULL;
		}
	}

	conn->op_completed=sys_sem_new(0);
	if(conn->op_completed == SYS_SEM_NULL)
	{	printf("error1.\r\n");
		memp_free(MEMP_NETCONN,conn);
		return NULL;
	}
//	printf("netconn_alloc():size=%d.\r\n",size);
#if NODE == TBC
	conn->recvmbox=SYS_MBOX_NULL;//对于TBC来说所有连接的recvmbox设置为空
#endif
	
#if NODE == TBIM
	conn->recvmbox=sys_mbox_new(size);
	if(conn->recvmbox == SYS_MBOX_NULL)
	{	printf("error2.\r\n");
		sys_sem_free(conn->op_completed);//如果在这里出错那么删除调用刚才成功分配的信号量
		memp_free(MEMP_NETCONN,conn);
		return NULL;
	}
#endif
	
	conn->flags=0;
	return conn;
}

struct netconn* netconn_new(enum netconn_type t)
{
	struct netconn* conn;
	struct api_msg msg;

	conn=netconn_alloc(t);
	if(conn != NULL)
	{  
		msg.function=do_newconn;
		msg.arg.conn=conn;
		if(SSNP_APIMSG(&msg) != ERR_OK)
		{
			sys_sem_free(conn->op_completed);//删除信号量。
			sys_mbox_free(conn->recvmbox);//删除邮箱
			memp_free(MEMP_NETCONN,conn);
			return NULL;
		}
	}
	return conn;
}

//注意这里当api函数返回后，api_msg和api_msg_arg都会丢掉，只能通过netconn中的last_err来记录上一个
//函数是否正确执行，而api_msg_arg中的err只能记录一次内核调用的返回结果
//但是neconn的last_err难道不是线程安全吗，会有多个线程同时访问一个netconn吗？这里难道只是针对tcp
//来说的吗？
err_t netconn_bind(struct netconn* conn,u8t alias,u8t tdcn_num)//绑定本地ip地址和端口号
{
	struct api_msg msg;
	err_t err;

	msg.function=do_bind;
	msg.arg.conn=conn;
	msg.arg.arg.bc.alias=alias;
	msg.arg.arg.bc.tdsn_num=tdcn_num;

	err=SSNP_APIMSG(&msg);
	if(!ERR_IS_FATAL(conn->last_err))//注意这里没有设计成线程安全的，因为这里不像tcp，不会有多个线程同时访问这一位应用线程和内核线程在访问netconn时是串行的
		conn->last_err=err;
	return err;
}

err_t netconn_connect(struct netconn* conn,u8t alias,u8t tdcn_num)
{
	struct api_msg msg;
	err_t err;

	msg.function=do_connect;
	msg.arg.conn=conn;
	msg.arg.arg.bc.alias=alias;
	msg.arg.arg.bc.tdsn_num=tdcn_num;

	err=ssnp_apimsg(&msg);
	if(!ERR_IS_FATAL(conn->last_err))
		conn->last_err=err;
	return err;
}

err_t netconn_disconnect(struct netconn* conn)
{
	struct api_msg msg;
	err_t err;

	if(conn == NULL)
		return ERR_ARGUMENT;

	msg.function=do_disconnect;
	msg.arg.conn=conn;
	err=SSNP_APIMSG(&msg);

	if(!ERR_IS_FATAL(conn->last_err))
		conn->last_err=err;
	return err;
}

err_t netconn_send(struct netconn* conn,struct netbuf* buf)
{
	struct api_msg msg;
	err_t err;

	if(conn == NULL || buf == NULL)
		return ERR_ARGUMENT;
//	printf("netconn_send():API send.\r\n");
	msg.function=do_send;
	msg.arg.conn=conn;
	msg.arg.arg.b=buf;
	err=SSNP_APIMSG(&msg);

	if(!ERR_IS_FATAL(conn->last_err))
		conn->last_err=err;
	return err;
}

err_t netconn_sendtrigger(struct netconn* conn,u8t tbim_tdcn_num,u8t alias,u8t tdcn_num)
{
	struct netbuf buf;
	buf.alias=alias;
	buf.tdcn_num=tdcn_num;
	buf.tbim_tdcn_num=tbim_tdcn_num;
	buf.p=NULL;

	return netconn_send(conn,&buf);
}
err_t netconn_sendto(struct netconn* conn,struct netbuf* buf,u8t alias,u8t tdcn_num)
{
	if(buf != NULL)
	{	//printf("netconn_sendto():API send.\r\n");
		buf->alias=alias;
		buf->tdcn_num=tdcn_num;
		return netconn_send(conn,buf);
	}
	return ERR_VALUE;
}

static err_t neconn_recv_data(struct netconn* conn,void** new_buf)
{
	void *buf;
	err_t err;

	buf=NULL;

	if(new_buf == NULL || conn == NULL)
		return ERR_ARGUMENT;

	*new_buf=NULL;//清空指针

	if(conn->recvmbox == SYS_MBOX_NULL)
		return ERR_ARGUMENT;

	err=conn->last_err;
	if(ERR_IS_FATAL(err))
		return err;

	sys_arch_mbox_fetch(conn->recvmbox,&buf,0);
	*new_buf=buf;

	return ERR_OK;
}

err_t netconn_recv(struct netconn* conn,struct netbuf* *new_buf)
{
	if(new_buf == NULL || conn == NULL)//首先要申请一个指针struct netbuf* buf变量;如果没有申请这个变量，那么其指针就是NULL了,也就是不可以传递空指针
		return ERR_ARGUMENT;


	*new_buf=NULL;//清空指针

	if(conn->recvmbox == SYS_MBOX_NULL)
		return ERR_ARGUMENT;

	return neconn_recv_data(conn,(void**)new_buf);
}
err_t netconn_recv_unblock(struct netconn* conn,struct netbuf** new_buf)
{
	err_t err;
	void* msg;
	
	if(new_buf == NULL || conn == NULL)//首先要申请一个指针struct netbuf* buf变量;如果没有申请这个变量，那么其指针就是NULL了,也就是不可以传递空指针
		return ERR_ARGUMENT;


	msg=NULL;
	*new_buf=NULL;//清空指针

	if(conn->recvmbox == SYS_MBOX_NULL)
		return ERR_ARGUMENT;

	err=conn->last_err;
	if(ERR_IS_FATAL(err))
		return err;
		
	msg=sys_arch_mbox_fetch_unblock(conn->recvmbox);
	if(msg == NULL)
	{
		return ERR_VALUE;
	}
	else
	{
		*new_buf=msg;
		return ERR_OK;
	}
}
err_t netconn_recvtrigger_buf(struct netconn* conn,struct netbuf** buf)
{
	struct netbuf* buffer;
	err_t err;
	if(buf == NULL || conn == NULL || conn->recvmbox == SYS_MBOX_NULL)
		return ERR_ARGUMENT;
	
	*buf=NULL;
	err=netconn_recv(conn,&buffer);
	if(err != ERR_OK)
		return ERR_VALUE;
	
	*buf=buffer;
	return err;
}
err_t netconn_recvtrigger(struct netconn* conn,u8t* alias,u8t* tdcn_num)
{
	struct netbuf* buf;
	err_t err;

	if(alias == NULL || tdcn_num == NULL)
		return ERR_ARGUMENT;

	buf=NULL;
	err=netconn_recv(conn,&buf);
	if(err != ERR_OK)
		return ERR_VALUE;

	*alias=buf->alias;
	*tdcn_num=buf->tdcn_num;

	netbuf_delete(buf);

	return err;
}
err_t netconn_recvtrigger_unblock(struct netconn* conn,u8t* alias,u8t* tdcn_num)
{
	struct netbuf* buf;
	void* msg;
	err_t err;

	if(alias == NULL || tdcn_num == NULL)
		return ERR_ARGUMENT;

	msg=NULL;
	buf=NULL;	
	err=conn->last_err;
	if(ERR_IS_FATAL(err))
		return err;
	
	msg=sys_arch_mbox_fetch_unblock(conn->recvmbox);
	
	if(msg == NULL)//没有接收到数据，也就是邮箱为空
	{
		return ERR_VALUE;
	}
	else
	{
		buf=(struct netbuf*)msg;
		*alias=buf->alias;
		*tdcn_num=buf->tdcn_num;
		netbuf_delete(buf);
		
		return ERR_OK;
	}
}
/*
 * 删除netconn
 */
static void netconn_free(struct netconn* conn)
{
	SSNP_ASSERT("PCB must be deallocated outside this function", conn->pcb.dp != NULL);
	SSNP_ASSERT("recvmbox must be deallocated before calling this function",conn->recvmbox!=SYS_MBOX_NULL);

	sys_sem_free(conn->op_completed);
	conn->op_completed=SYS_SEM_NULL;

	memp_free(MEMP_NETCONN,conn);
}

err_t netconn_delete(struct netconn* conn)
{
	struct api_msg msg;

	if(conn == NULL)
		return ERR_OK;

	msg.function=do_delconn;
	msg.arg.conn=conn;
	ssnp_apimsg(&msg);

	netconn_free(conn);

	return ERR_OK;
}

#if NODE == TBIM
u8t get_alias()
{
	struct api_msg msg;
	err_t err;
	
	msg.arg.arg.ga.op_completed=sys_sem_new(0);
	msg.arg.arg.ga.alias=0;
	msg.function=do_getalias;

  for(;;)
	{
		err=ssnp_apimsg_alias(&msg);
		
		if(msg.arg.arg.ga.alias!=0)
			return msg.arg.arg.ga.alias;
	}
}
#endif

#if NODE == TBC
void* get_tbim()
{
	struct api_msg msg;
	err_t err;
	
	msg.arg.arg.gl.op_completed=sys_sem_new(0);
	msg.arg.arg.gl.alias_list=NULL;
	msg.arg.arg.gl.alias_alloc_done=0;
	msg.function=do_get_alias_alloc_state;
	
	err=ssnp_apimsg_alias_list(&msg);
	return msg.arg.arg.gl.alias_list;
}
void set_connection_recvmbox(struct netconn* conn,sys_mbox_t mbox)
{
	if(conn == NULL || mbox == SYS_MBOX_NULL)
		return ;
	conn->recvmbox=mbox;
}
#endif
