#ifndef API_MSG_H
#define API_MSG_H

#include "api.h"

struct api_msg_arg
{
	struct netconn* conn;
	err_t err;
	union
	{
		struct netbuf* b;
		struct 
		{
			u8t proto;
		}n;
		struct 
		{
			u8t alias;
			u8t tdsn_num;
		}bc;
#if NODE == TBIM
		struct
		{
			sys_sem_t op_completed;
			u8t alias;
		}ga;//TBIM获取别名
#endif
		
#if NODE == TBC
		struct 
		{
			sys_sem_t op_completed;
			u8t alias_alloc_done;
			void* alias_list;
		}gl;
#endif
		struct 
		{
			u32t len;//这里其实不会接收到这么大的数据
		}r;
	}arg;
};

struct api_msg
{
	void (*function)(struct api_msg_arg* arg);
	struct api_msg_arg arg;
};

#if NODE == TBIM
void do_getalias(struct api_msg_arg* arg);
#endif

#if NODE == TBC
void do_get_alias_alloc_state(struct api_msg_arg* arg);
#endif

void do_newconn(struct api_msg_arg* arg);
void do_bind(struct api_msg_arg* arg);
void do_connect(struct api_msg_arg* arg);
void do_disconnect(struct api_msg_arg* arg);
void do_send(struct api_msg_arg* arg);
void do_delconn(struct api_msg_arg* arg);
#endif
