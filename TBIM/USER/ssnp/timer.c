#include "timer.h"
#include "memp.h"
#include "ethbcp.h"
#include "alias_state.h"
//ȫ�ֳ�ʱ�¼�����ʱ��ӽ������ɳ�
static struct sys_timeo* next_timeout;


#if NODE == TBC
void discovery_timer(void* arg)
{
	InitiateDiscovery(arg);
	if(get_alias_alloc_state == 0)
		sys_timeout(DISCOVERY_TMR_INTERVSL,discovery_timer,arg);
}
extern struct alias_state* as_list;
void alias_check_timer(void* arg)
{
	clear_unresponsive_alias();
	if(as_list)
		sys_timeout(ALIASCHECK_TMR_INTERVSL,alias_check_timer,NULL);
}
static void alias_alloc_finish(void* arg)
{
	set_alias_alloc_done();
}
#endif

void sys_timeouts_init()
{
#if NODE == TBC
	sys_timeout(TBC_ALIASALLOC_WAIT_INTERVSL,alias_alloc_finish,NULL);
#endif
}

void sys_timeout(u32t msecs,sys_timeout_handler handler,void* arg)
{
	struct sys_timeo* timeout;
	struct sys_timeo* t;

	timeout=(struct sys_timeo*)memp_alloc(MEMP_SYS_TIMEOUT);
	if(timeout == NULL)
		return ;

	timeout->next=NULL;
	timeout->h=handler;
	timeout->arg=arg;
	timeout->time=msecs;

	if(next_timeout == NULL)
	{
		next_timeout=timeout;
		return ;
	}

	if(next_timeout->time > msecs)//����������¼���������ı�ͷ
	{
		next_timeout->time -= msecs;
		timeout->next=next_timeout;
		next_timeout=timeout;
	}
	else
	{
		for(t=next_timeout; t!=NULL; t=t->next)
		{
			timeout->time -= t->time;
			if(t->next == NULL || t->next->time > timeout->time)
			{
				if(t->next != NULL)
					t->next->time -= timeout->time;//��Ϊ�м����һ��ʱ�䣬��˺������Ӧ��ȥ

				timeout->next =t->next;
				t->next = timeout;
				break;
			}
		}
	}
}

void sys_untimeout(sys_timeout_handler handler,void* arg)
{
	struct sys_timeo* prev;
	struct sys_timeo* i;

	if(next_timeout == NULL)
		return ;

	i=next_timeout;
	prev=NULL;

	for(;i != NULL; prev=i,i=i->next)
	{
		if(i->h == handler && i->arg == arg)
		{
			if(prev == NULL)
				next_timeout=i->next;
			else
				prev->next=i->next;

			if(i->next != NULL)
				i->next->time += i->time;


			memp_free(MEMP_SYS_TIMEOUT,i);
			return ;
		}
	}
}

void sys_timeouts_mbox_fetch(sys_mbox_t mbox,void** msg)
{
	u32t time_needed;
	struct sys_timeo* tmptimeout;
	sys_timeout_handler handler;
	void* arg;

	for(;;)
	{
		if(next_timeout == NULL)//û�г�ʱ�¼���Ҫ����
		{
			time_needed=sys_arch_mbox_fetch(mbox,msg,0);//�����ȴ���ֱ�ӷ���
			return;
		}
		else
		{
			if(next_timeout->time > 0)//��δ��ʱ
			{
				time_needed=sys_arch_mbox_fetch(mbox,msg,next_timeout->time);//����ָ��ʱ��
			}
			else
			{
				time_needed=SYS_ARCH_TIMEOUT;//���ֳ�ʱ�¼�
			}

			if(time_needed == SYS_ARCH_TIMEOUT)//����ʱ�¼�
			{
				tmptimeout=next_timeout;
				next_timeout=tmptimeout->next;
				handler=tmptimeout->h;
				arg=tmptimeout->arg;

				memp_free(MEMP_SYS_TIMEOUT,tmptimeout);
				if(handler != NULL)
					handler(arg);//��ʱ�����������ȴ���������
			}
			else//δ��ʱ�����趨��ʱ���ڵõ�msg
			{
				if(time_needed < next_timeout->time)
					next_timeout->time -= time_needed;
				else
					next_timeout->time=0;//������Ҳ���Կ��������ʱ����������ô��׼ȷ

				return ;
			}
		}
	}
}
