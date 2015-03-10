
#include "ssnp.h"
#include "init.h"
#include "memp.h"
#include "debug.h"
#include "ethbcp.h"

static sys_mbox_t mbox;//ssnp�ں��̵߳Ļ�����

static ssnp_init_done_fn ssnp_init_done;
static void* ssnp_init_done_arg;

err_t ssnp_apimsg(struct api_msg* apimsg)
{
	struct ssnp_msg msg;
	if(mbox != SYS_MBOX_NULL)
	{
		msg.type=SSNP_MSG_API;
		msg.msg.apimsg=apimsg;
		sys_mbox_post(mbox,&msg);
		sys_arch_sem_wait(apimsg->arg.conn->op_completed,0);//�����ȴ�ֱ���ź����ͷ�
		return apimsg->arg.err;//ע�������ֵ����do_�����б��޸ĵģ���do_������ͷ����Ϊok���������ִ�е�״̬��������Ӧ��ֵ��
	}
	return ERR_VALUE;
}
#if NODE == TBIM
err_t ssnp_apimsg_alias(struct api_msg* apimsg)
{
	struct ssnp_msg msg;
	if(mbox != SYS_MBOX_NULL)
	{
		msg.type=SSNP_MSG_API;
		msg.msg.apimsg=apimsg;
		sys_mbox_post(mbox,&msg);
		sys_arch_sem_wait(apimsg->arg.arg.ga.op_completed,0);//�����ȴ�ֱ���ź����ͷ�
		return apimsg->arg.err;//ע�������ֵ����do_�����б��޸ĵģ���do_������ͷ����Ϊok���������ִ�е�״̬��������Ӧ��ֵ��
	}
	return ERR_VALUE;	
}
#endif
#if NODE == TBC
err_t ssnp_apimsg_alias_list(struct api_msg* apimsg)
{
	struct ssnp_msg msg;
	if(mbox != SYS_MBOX_NULL)
	{
		msg.type=SSNP_MSG_API;
		msg.msg.apimsg=apimsg;
		sys_mbox_post(mbox,&msg);
		sys_arch_sem_wait(apimsg->arg.arg.gl.op_completed,0);//�����ȴ�ֱ���ź����ͷ�
		return apimsg->arg.err;//ע�������ֵ����do_�����б��޸ĵģ���do_������ͷ����Ϊok���������ִ�е�״̬��������Ӧ��ֵ��
	}
	return ERR_VALUE;		
}
#endif
static void ssnp_thread(void* arg)
{
	struct ssnp_msg* msg;

	if(ssnp_init_done != NULL)
		ssnp_init_done(ssnp_init_done_arg);

	for(;;)
	{
		sys_timeouts_mbox_fetch(mbox,(void**)&msg);
		switch(msg->type)
		{
		case SSNP_MSG_API:
			{
				msg->msg.apimsg->function(&(msg->msg.apimsg->arg));
				break;//���ﷵ�غ���Զ�ɾ��ssnp_msg
			}
		case SSNP_MSG_INPKT:
			{
				ethernet_input(msg->msg.inp.p,msg->msg.inp.netif);//#δ���#���ݶ�����
				memp_free(MEMP_SSNP_MSG_INPKT,msg);
				break;
			}
		case SSNP_MSG_TIMEOUT:
			{
				sys_timeout(msg->msg.tmo.msecs,msg->msg.tmo.h,msg->msg.tmo.arg);
				memp_free(MEMP_SSNP_MSG_API,msg);
				break;
			}
		case SSNP_MSG_UNTIMEOUT:
			{
				sys_untimeout(msg->msg.tmo.h,msg->msg.tmo.arg);
				memp_free(MEMP_SSNP_MSG_API,msg);
				break;
			}
		default:
			break;
		}
	}
}

void ssnp_init(ssnp_init_done_fn initfunc,void* arg)
{
	protocol_stack_init();

	ssnp_init_done=initfunc;
	ssnp_init_done_arg=arg;

	mbox=sys_mbox_new(SSNP_MBOX_SIZE);//����ssnp���̵߳�mbox

	sys_thread_new("ssnp_thread",ssnp_thread,NULL,SSNP_THREAD_STACKSIZE,SSNP_THREAD_PRIO);//����ssnp���߳�
}


/**************************************************************************************************************
 * 
 *
 *  ��һ���������Ϣ��msgֱ��post�������оͷ����˲��õȴ���ִ����ɣ�Ŀ�������жϿ���ִ�����
 *
 **************************************************************************************************************/
err_t ssnp_input(struct pbuf* p,struct netif* inp)
{                                              
	struct ssnp_msg* msg;

	if(mbox == SYS_MBOX_NULL)
		return ERR_VALUE;

	msg=(struct ssnp_msg*)memp_alloc(MEMP_SSNP_MSG_INPKT);
	if(msg == NULL)
	{
	  SSNP_DEBUG_PRINT("ssnp_input():ssnp_msg alloc failed.\r\n");
		return ERR_MEMORY;
	}

	msg->type=SSNP_MSG_INPKT;
	msg->msg.inp.p=p;
	msg->msg.inp.netif=inp;
	if(sys_mbox_trypost(mbox,msg) != ERR_OK)
	{
	  SSNP_DEBUG_PRINT("ssnp_input():try post ssnp_msg failed.\r\n");
		memp_free(MEMP_SSNP_MSG_INPKT,msg);
		return ERR_MEMORY;
	}

	return ERR_OK;
}

err_t ssnp_timeout(u32t msecs,sys_timeout_handler h,void* arg)
{
	struct ssnp_msg* msg;

	if(mbox != SYS_MBOX_NULL)
	{
		msg=(struct ssnp_msg*)memp_alloc(MEMP_SSNP_MSG_API);
		if(msg == NULL)
			return ERR_MEMORY;

		msg->type=SSNP_MSG_TIMEOUT;
		msg->msg.tmo.msecs=msecs;
		msg->msg.tmo.h=h;
		msg->msg.tmo.arg=arg;
		sys_mbox_post(mbox,msg);
		return ERR_OK;
	}
	return ERR_VALUE;
}

err_t ssnp_untimeout(sys_timeout_handler h,void* arg)
{
	struct ssnp_msg* msg;

	if(mbox != SYS_MBOX_NULL)
	{
		msg=(struct ssnp_msg*)memp_alloc(MEMP_SSNP_MSG_API);
		if(msg == NULL)
			return ERR_MEMORY;

		msg->type=SSNP_MSG_UNTIMEOUT;
		msg->msg.tmo.h=h;
		msg->msg.tmo.arg=arg;
		sys_mbox_post(mbox,msg);
		return ERR_OK;
	}
	return ERR_VALUE;
}
