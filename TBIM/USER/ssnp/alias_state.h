#ifndef ALIAS_STATE_H
#define ALIAS_STATE_H
#include "opt.h"
#include "err.h"
#include "bcp_msg.h"
/*************************************************************************************
 *  
 *  这里并不需要维护一个状态信息，因为凡是在链表中的，一定是TBC发送了别名分配   
 *   消息的那是还没有接收到TBIM返回的别名应答消息。因此这里只有两个状态一个是完成
 *   另一个是没有完成，凡是完成的一定已经在链表中删除了。
 *   
 *   这里还需要维护一个超时event，如果链表不空就需要定时查询是不是有别名长时间处于待定
 *   状态，如果有那么就回收此别名。
 *************************************************************************************/
 #define PHYTEDS_INFO_LEN 14
struct alias_state
{
	struct alias_state* next;
	u32t s_time;	
	u8t alias;
	u8t phyteds_info[PHYTEDS_INFO_LEN];
	struct uuid uid;
};
err_t add_alias_pending(u8t alias,struct uuid* id);
void alias_done(u8t alias,u8t* info);
void clear_unresponsive_alias(void);
u8t is_exist_in_pending_list(u8t alias);
void* get_alias_done_list(void);
#endif
