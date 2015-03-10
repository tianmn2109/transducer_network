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
 *   if(num == 0)说明内核已经将此超时event删除了，需要重新添加
 *      添加超时event
 *
 *   在这一模块中我们应该添加一个超时处理函数，和上面的超时event就可以了
 *   因为所有内核都在同一个线程之中
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
		
		sys_timeout(ALIASCHECK_TMR_INTERVSL,alias_check_timer,NULL);//如果当前没有待定节点，那么添加别名检测计时器
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
 * 超时计算两个时间差的时候t1,t2，如果t2>=t1那么正常，否则系统时间从0xffffffff又绕回了
 * 如果系统中不会有新加入的TBIM那么这里的清空没有响应的TBIM就没有必要了
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
			del_alias_map(alias);//回收此别名
		}
	}
}
#endif
