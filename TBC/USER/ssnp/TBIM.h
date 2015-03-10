#ifndef TBIM_H
#define TBIM_H

#include "bcp_msg.h"

/**********************************************************************************
 *                                  TBIM�ĳ�ʼ��״̬
 **********************************************************************************/
enum INIT_STATE
{
	READ_TBIM_STRUCT=0,
	READ_META_TEDS,
	READ_TDCN_TEDS,
	WAITTING_FOR_TIME
};

/**********************************************************************************
 *                                  uuid���
 **********************************************************************************/
struct uuid* get_uuid(void);
u8t is_uuid_eq(struct uuid* id);


/**********************************************************************************
 *                               TEDS��غ���
 **********************************************************************************/
u8t get_asy_flag(void);
u8t get_payload_encoding(void);
f32 get_start_delay(void);
f32 get_reflect_delay(void);
f32 get_reflect_delay_uncertainty(void);


/**********************************************************************************
 *                           Ӧ�ó����ʹ�õ��Ľӿ�
 * 
 *  TBIM����ִ�е�  
 *  �����ϲ�ֻ��һ��TBIM���������ĳ�ʼ��TBIM����ֻ��һ����������ｫTBIM��Ƴ�˽��
 * ����,����Ҳ����Ҫ������Ӧ��֪��dp��tp����Щ���ӵĴ��ڡ�
 **********************************************************************************/

void TBIM_init(u8t alias);
enum INIT_STATE get_TBIM_init_state(void);
u8t get_TBIM_Chn_num(void);
u16t max_period(void);
u16t get_sensor_period(u8t tdcn_num);
void* get_sensor_data(u8t tdcn_num);
void execute(u8t cmdClass,u8t cmdFunc,u8t tdcn_num,void *argument);//TBIM����ִ��
void trigger_do(void);//TBIM���յ�triggerӦ�ò�ȡ�Ĳ���
void trigger(void* argument);//#δ���#������Ľӿ���ƵĲ��ã�����˼����
err_t TBIM_send_streaming_data(u8t tbim_tdcn_num,void* data);//������һ��������ͨ������������
err_t TBIM_recv_cmd_and_execute(void);//ע�⵱ǰ������Ľ��պ������Ƿ�����ʽ��
err_t TBIM_recv_trigger_and_execute(void);//��Ϊֻ��TBC����trigger��TBIM����trigger����˲��ñ������һ���������ģ�ȷ����TBC

#endif
