#ifndef ALIAS_STATE_H
#define ALIAS_STATE_H
#include "opt.h"
#include "err.h"
#include "bcp_msg.h"
/*************************************************************************************
 *  
 *  ���ﲢ����Ҫά��һ��״̬��Ϣ����Ϊ�����������еģ�һ����TBC�����˱�������   
 *   ��Ϣ�����ǻ�û�н��յ�TBIM���صı���Ӧ����Ϣ���������ֻ������״̬һ�������
 *   ��һ����û����ɣ�������ɵ�һ���Ѿ���������ɾ���ˡ�
 *   
 *   ���ﻹ��Ҫά��һ����ʱevent����������վ���Ҫ��ʱ��ѯ�ǲ����б�����ʱ�䴦�ڴ���
 *   ״̬���������ô�ͻ��մ˱�����
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
