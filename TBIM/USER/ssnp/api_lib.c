#include "api_msg.h"
#include "ssnp.h"
#include "memp.h"
#include "mem.h"
#include "debug.h"

/*
-------------------------------------------------------------------------------------
      ע�����TBC��˵��DP��TP��conn��û�б�Ҫ����recvmbox�ģ���ΪTBC������������trigger��
	  TBC������ǿͻ��ˣ�ֻ��SDP��Ҫ����recvmbox�����ڽ��������ݣ�����û��Ū�������Ϊʲô
	  ���ֻ�ܵ����Ĵ�OSQCreate()���ټ������þͷ��ؿ�ֵ������������ַ����ˣ�
	  A:һ����Ū���Ϊʲô��
	  B:���иɴ�����sdp���Ӷ�ʹ��ͬһ��mbox��������ͬ��ʱ���ڲ�����ѵ��һ��mbox�Ϳ�����,
	    Ҳ���ø���mbox֮�������л��ˣ�ͬʱ��ѯ�ķ�ʽҲ���ӵļ��ˡ��������ַ�������
		��һЩ��
	 ʵ�������ǿ����Լ������һ�����У�������Ҫ���ǵ�TBC�����⼸������֮�⻹��Ҫһ��mbox����
	 ������λ�����͵�������Ƕ��ˣ����߳�һ����sdpһ������λ����������һ����dp���������Ҳ����Ҫ�ġ��㹻�ˡ�
	 �������е�dp����ʹ��һ��mboxҲ�ǿ��Եģ���Ϊÿ�ζ���һ��TBIM����ͨ�š�
	 ����TBIM��˵��SDP��û�б�Ҫ����mbox�ġ�
	 ���ǾͰ��������˼·ִ�У��������Ǿ��������Ȱ�ǰ���������ͨ�ˡ�
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
	conn->recvmbox=SYS_MBOX_NULL;//����TBC��˵�������ӵ�recvmbox����Ϊ��
#endif
	
#if NODE == TBIM
	conn->recvmbox=sys_mbox_new(size);
	if(conn->recvmbox == SYS_MBOX_NULL)
	{	printf("error2.\r\n");
		sys_sem_free(conn->op_completed);//��������������ôɾ�����øղųɹ�������ź���
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
			sys_sem_free(conn->op_completed);//ɾ���ź�����
			sys_mbox_free(conn->recvmbox);//ɾ������
			memp_free(MEMP_NETCONN,conn);
			return NULL;
		}
	}
	return conn;
}

//ע�����ﵱapi�������غ�api_msg��api_msg_arg���ᶪ����ֻ��ͨ��netconn�е�last_err����¼��һ��
//�����Ƿ���ȷִ�У���api_msg_arg�е�errֻ�ܼ�¼һ���ں˵��õķ��ؽ��
//����neconn��last_err�ѵ������̰߳�ȫ�𣬻��ж���߳�ͬʱ����һ��netconn�������ѵ�ֻ�����tcp
//��˵����
err_t netconn_bind(struct netconn* conn,u8t alias,u8t tdcn_num)//�󶨱���ip��ַ�Ͷ˿ں�
{
	struct api_msg msg;
	err_t err;

	msg.function=do_bind;
	msg.arg.conn=conn;
	msg.arg.arg.bc.alias=alias;
	msg.arg.arg.bc.tdsn_num=tdcn_num;

	err=SSNP_APIMSG(&msg);
	if(!ERR_IS_FATAL(conn->last_err))//ע������û����Ƴ��̰߳�ȫ�ģ���Ϊ���ﲻ��tcp�������ж���߳�ͬʱ������һλӦ���̺߳��ں��߳��ڷ���netconnʱ�Ǵ��е�
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

	*new_buf=NULL;//���ָ��

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
	if(new_buf == NULL || conn == NULL)//����Ҫ����һ��ָ��struct netbuf* buf����;���û�����������������ô��ָ�����NULL��,Ҳ���ǲ����Դ��ݿ�ָ��
		return ERR_ARGUMENT;


	*new_buf=NULL;//���ָ��

	if(conn->recvmbox == SYS_MBOX_NULL)
		return ERR_ARGUMENT;

	return neconn_recv_data(conn,(void**)new_buf);
}
err_t netconn_recv_unblock(struct netconn* conn,struct netbuf** new_buf)
{
	err_t err;
	void* msg;
	
	if(new_buf == NULL || conn == NULL)//����Ҫ����һ��ָ��struct netbuf* buf����;���û�����������������ô��ָ�����NULL��,Ҳ���ǲ����Դ��ݿ�ָ��
		return ERR_ARGUMENT;


	msg=NULL;
	*new_buf=NULL;//���ָ��

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
	
	if(msg == NULL)//û�н��յ����ݣ�Ҳ��������Ϊ��
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
 * ɾ��netconn
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
