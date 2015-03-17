#ifndef SSNP_FOR_APP_H
#define SSNP_FOR_APP_H
#include "ethernetif.h"
#include "init.h"
#include "ssnp.h"

void protocol_init(void);
void hardware_init(void);

// 不同EDS的地址
#define PHY_ADDR  (0x0000 + 18)
#define META_ADDR (0x0000 + 18 + 17)
#define TRANSDUCER_CHANNEL_ADDR   (0x0000 + 18 + 17 + 35)  

static u16 buf_tail = 0;
/*
--------------------------------------------------------------------------------------------------
																	TBC应用程序需要使用的接口
						
    主要是总线初始阶段发送配置信息
 
--------------------------------------------------------------------------------------------------
*/
#if NODE == TBC
void set_discovery_timer(void);
err_t send_discovery_msg(void);
err_t tbc_send_timeslot_msg(u8t alias,u8t tbim_tdcn_num,u16t beign_time_slot,u8t time_slot_num);
err_t tbc_send_define_epoch_msg(u16t syn_timeslot_num,u16t asyn_timeslot_num);
err_t tbc_send_begin_of_epoch_msg(void);
err_t tbc_send_reflect_msg(u8t alias);
#endif

#if NODE == TBIM
err_t tbim_send_reflect_reply_msg(void);
#endif

#endif
