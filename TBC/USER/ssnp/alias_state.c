#include "alias_state.h"
#include "mem.h"
#include "sys.h"
#include "alias_map.h"
#include "timer.h"
#include "config.h"
#if NODE == TBC
struct alias_state* as_list;
static struct alias_state* done_list;
void* get_alias_done_list(void)
{
	return done_list;
}
static void copy_uuid(struct uuid* from,struct uuid* to)
{
	u8t i;
	for(i=0;i<10;i++)
	{
		to->uid[i]=from->uid[i];
	}
}
static struct alias_state* find(u8t alias)
{
	struct alias_state* i;
	for(i=as_list;i;i=i->next)
	{
		if(i->alias == alias)
			return i;
	}
	return NULL;
}
u8t is_exist_in_pending_list(u8t alias)
{
	struct alias_state* i;
	i=NULL;

	i=find(alias);
	return i == NULL ? 0 : 1;
}
static void delete_alias(u8t alias)
{
	struct alias_state* p;
	struct alias_state* prev;
	
	if(as_list == NULL)
		return ;
	
	if(as_list->next == NULL)
	{
		if(as_list->alias == alias)
		{
			mem_free((void*)as_list);
			as_list = NULL;
		}
	}
	else
	{
		prev=as_list;
		p=as_list->next;
		while(p)
		{
			if(p->alias == alias)
			{
				prev->next=p->next;
				mem_free((void*)p);
				return ;
			}
			prev=p;
			p=p->next;
		}
	}
}
/***********************************************************************
 *
 *   if(num == 0)˵���ں��Ѿ����˳�ʱeventɾ���ˣ���Ҫ�������
 *      ��ӳ�ʱevent
 *
 *   ����һģ��������Ӧ�����һ����ʱ��������������ĳ�ʱevent�Ϳ�����
 *   ��Ϊ�����ں˶���ͬһ���߳�֮��
 ***********************************************************************/
err_t add_alias_pending(u8t alias,struct uuid* id)
{
	struct alias_state* as;

	as=(struct alias_state*)mem_alloc(sizeof(struct alias_state));
	if(as == NULL)
		return ERR_MEMORY;
	as->alias=alias;
	as->s_time=sys_time_get();
	copy_uuid(id,&as->uid);
	
	if(as_list == NULL)
	{
		as_list=as;
		as->next=NULL;
		
		sys_timeout(ALIASCHECK_TMR_INTERVSL,alias_check_timer,NULL);//�����ǰû�д����ڵ㣬��ô��ӱ�������ʱ��
	}
	else
	{
		as->next=as_list;
		as_list=as;
	}

	return ERR_OK;
}
void alias_done(u8t alias,u8t* info)
{
	struct alias_state* p;
	struct alias_state* prev;
	
	struct alias_state* as;
	u8t i;
	
	as=NULL;
	if(as_list == NULL)
		return;
	if(as_list->next == NULL )
	{
		if(as_list->alias == alias)
		{				
			as=as_list;
			if(done_list == NULL)
			{
				done_list=as_list;
				as_list=NULL;
			}
			else
			{
				as_list->next=done_list;
				done_list=as_list;
				as_list=NULL;
			}
	  }
	}
	else
	{
		prev=as_list;
		p=as_list;
		while(p)
		{
			if(p->alias == alias)
			{
				as=p;
				if(p == as_list)
					as_list=as_list->next;
				else
					prev->next=p->next;
				if(done_list == NULL)
				{
					done_list=p;
					p->next=NULL;
				}
				else
				{
					p->next=done_list;
					done_list=p;
				}
				break ;
			}
			prev=p;
			p=p->next;
		}
	}
	if(as)
	{
		for(i=0;i<PHYTEDS_INFO_LEN;i++)
			as->phyteds_info[i]=info[i];
	}
}

/***************************************************************************************
 *
 * ��ʱ��������ʱ����ʱ��t1,t2�����t2>=t1��ô����������ϵͳʱ���0xffffffff���ƻ���
 * ���ϵͳ�в������¼����TBIM��ô��������û����Ӧ��TBIM��û�б�Ҫ��
 *
 ***************************************************************************************/
void clear_unresponsive_alias()
{
	struct alias_state* as;
	u32t time_now;
	u32t time;
	u8t alias;
	for(as=as_list;as;as=as->next)
	{
		time_now=sys_time_get();
		if(time_now >= as->s_time)
		{
			time=time_now - as->s_time;
		}
		else
		{
			time=0xffffffff - as->s_time + time_now;
		}
		
		if(time > NORESPONSE_TMR_INTERVSL)
		{
			alias=as->alias;
			delete_alias(alias);
			del_alias_map(alias);//���մ˱���
		}
	}
}
#endif
