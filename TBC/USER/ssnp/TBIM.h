#ifndef TBIM_H
#define TBIM_H

#include "bcp_msg.h"

/**********************************************************************************
 *                                  TBIM的初始化状态
 **********************************************************************************/
enum INIT_STATE
{
	READ_TBIM_STRUCT=0,
	READ_META_TEDS,
	READ_TDCN_TEDS,
	WAITTING_FOR_TIME
};

/**********************************************************************************
 *                                  uuid相关
 **********************************************************************************/
struct uuid* get_uuid(void);
u8t is_uuid_eq(struct uuid* id);


/**********************************************************************************
 *                               TEDS相关函数
 **********************************************************************************/
u8t get_asy_flag(void);
u8t get_payload_encoding(void);
f32 get_start_delay(void);
f32 get_reflect_delay(void);
f32 get_reflect_delay_uncertainty(void);


/**********************************************************************************
 *                           应用程序会使用到的接口
 * 
 *  TBIM命令执行等  
 *  由于上层只有一个TBIM，因此这里的初始化TBIM必须只有一个，因此这里将TBIM设计成私有
 * 变量,我们也不需要让其他应用知道dp，tp等这些链接的存在。
 **********************************************************************************/

void TBIM_init(u8t alias);
enum INIT_STATE get_TBIM_init_state(void);
u8t get_TBIM_Chn_num(void);
u16t max_period(void);
u16t get_sensor_period(u8t tdcn_num);
void* get_sensor_data(u8t tdcn_num);
void execute(u8t cmdClass,u8t cmdFunc,u8t tdcn_num,void *argument);//TBIM命令执行
void trigger_do(void);//TBIM接收到trigger应该采取的操作
void trigger(void* argument);//#未完成#：这里的接口设计的不好，重新思考下
err_t TBIM_send_streaming_data(u8t tbim_tdcn_num,void* data);//发送哪一个变送器通道的神马数据
err_t TBIM_recv_cmd_and_execute(void);//注意当前和下面的接收函数都是非阻塞式的
err_t TBIM_recv_trigger_and_execute(void);//因为只有TBC发送trigger，TBIM接收trigger，因此不用标记是哪一个发送来的，确定是TBC

#endif
