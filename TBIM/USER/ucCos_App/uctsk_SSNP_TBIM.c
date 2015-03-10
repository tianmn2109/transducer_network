#include "ssnp/config.h"
#if NODE == TBIM
#include <includes.h>
#include "ssnp/ssnp_for_app.h"
#include "ssnp/ssnp.h"
#include "ssnp/TBIM.h"

#include "ssnp/debug.h"
#include "ssnp/mem.h"
#include "uctsk_SSNP_TBIM.h"

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
	
#define TBC_ADDRESS          		  255
#define RECV_SEND_TIME        		200 //#未完成#：这里假设接收和回复命令需要使用200us
#define TIME_SLOT_LEN	        		100 //这里设定时间槽为100us
	
#define DELAY                  		50  //从TBC开始发送到TBIM应用层接收到的时延                   #测量#
#define TRIGGER_RECV_EXE_TIME  		100 //把这个时间定义为TBIM足够接收并执行一个trigger的时间     #测量#
#define CMD_RECV_EXE_TIME      		200 //把这个时间定义为TBIM足够接收名执行一个命令的时间        #测量#
#define DELAY_TIME_MEASURE_NUM    6

sys_sem_t sem;
#define SIGNAL sys_sem_signal(sem)//释放信号量

enum msg_type       
{
	DP,
	TP
};

typedef enum
{
	FREE=0,
	SYN,
	ASYN,
	WORKING
}state;



struct TBIM_msg
{
	enum msg_type type;
	struct netconn* conn;
	struct netbuf* buf;//只有dp数据才需要参数，tp不需要参数
	sys_sem_t op_completed;
};


//#define TBIM_MSG_NUM  10
//static OS_MEM* tbim_msg;
//static u8t tbim_msg_pool[TBIM_MSG_NUM * sizeof(struct TBIM_msg)];

static sys_mbox_t tbim_mbox;


extern  struct TBIM_ tbim;
/*
------------------------------------------------------------------------------------------------------------
	一定要注意这里的计时范围一次最多是65536us,也就是大概65ms,注意两个时间段不要超过65ms否则没有办法计时
------------------------------------------------------------------------------------------------------------
*/
static void TIM5_Init()
{
	TIM_TimeBaseInitTypeDef Tim5;  
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);  
  	Tim5.TIM_Period=0xffff-1;	//注意TBIM中断周期100us
	Tim5.TIM_Prescaler=72-1;
	Tim5.TIM_ClockDivision=0;
	Tim5.TIM_CounterMode=TIM_CounterMode_Down;
	TIM_TimeBaseInit(TIM5,&Tim5);
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM5,ENABLE);
}  
static void NVIC_Configuration_TIM5(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure one bit for preemption priority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* Enable the ETH Interrupt 在这里使能以太网中断*/
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
static void TIM5_delay_us(u16t us)//使用定时器不用自己判断了
{
	TIM_SetCounter(TIM5,us);
	while(us > 1)
		us=TIM_GetCounter(TIM5);
}

void begin_epoch_callback(u16t syn_timeslot,u16t asyn_timeslot)
{
	///time_f=TIM_GetCounter(TIM5);//计时开始
}
static void app_init()
{
//	u8t err;
//	tbim_msg=OSMemCreate((void*)tbim_msg_pool,TBIM_MSG_NUM,sizeof(struct TBIM_msg),&err);
	
//	tbim_mbox=sys_mbox_new(MAX_QUEUE_ENTRIES);
	sem=sys_sem_new(0);
	TIM5_Init();
//	NVIC_Configuration_TIM5();
}

/***********************************************************************************************
 * 应用程序各个线程栈大小
 ***********************************************************************************************/
static OS_STK AppSSNPTaskStk[APP_TASK_SSNP_STK_SIZE];//ssnp应用程序主线程栈大小

static OS_STK AppSSNPdpTaskStk[APP_TASK_DP_STK_SIZE];//ssnp应用dp线程栈大小
static OS_STK AppSSNPtpTaskStk[APP_TASK_TP_STK_SIZE];//ssnp应用tp线程栈大小
static OS_STK AppSSNPsdpTaskStk[APP_TASK_SDP_STK_SIZE];//ssnp应用sdp线程栈大小


static void uctsk_SSNP_dp_recv(void* pdata)
{
	struct netconn* conn;
	struct netbuf* buf;
	struct TBIM_msg msg;
	u8t* alias;

	
	alias=pdata;
	msg.op_completed=sys_sem_new(0);
	
	conn=netconn_new(NETCONN_DP);
	netconn_bind(conn,*alias,0);//先将别名绑定到0
	netconn_connect(conn,TBC_ADDRESS,0);//TBC的别名总是1
	printf("uctsk_SSNP_dp_recv(): tbim alias is %d, tbc alias is %d.\r\n",*alias,TBC_ADDRESS);
	
	for(;;)
	{
		netconn_recv(conn,&buf);
		SSNP_DEBUG_PRINT("uctsk_SSNP_dp_recv(): app get dp data.\r\n");
		msg.type=DP;
		msg.buf=buf;
		msg.conn=conn;
		sys_mbox_post(tbim_mbox,&msg);
		SSNP_DEBUG_PRINT("uctsk_SSNP_dp_recv(): waiting for TBIM exe cmd.\r\n");
		sys_arch_sem_wait(msg.op_completed,0);
		SSNP_DEBUG_PRINT("uctsk_SSNP_dp_recv(): TBIM exe cmd done.\r\n");
		netbuf_delete(buf);

		OSTimeDly(1);
	}
}
static void uctsk_SSNP_tp_recv(void* pdata)
{
	struct netconn* conn;
	struct netbuf* buf;
	struct TBIM_msg msg;
	u8t* alias;
	
	alias=pdata;
	buf=NULL;
	msg.op_completed=sys_sem_new(0);
	
	conn=netconn_new(NETCONN_TP);
	netconn_bind(conn,*alias,0);
	netconn_connect(conn,TBC_ADDRESS,0);
	printf("uctsk_SSNP_tp_recv(): tbim alias is %d, tbc alias is %d.\r\n",*alias,TBC_ADDRESS);
	for(;;)
	{
		netconn_recvtrigger_buf(conn,&buf);
		msg.type=TP;
		msg.buf=buf;
		msg.conn=conn;
		sys_mbox_post(tbim_mbox,&msg);
		SSNP_DEBUG_PRINT("uctsk_SSNP_tp_recv(): waiting for TBIM exe trigger.\r\n");
		sys_arch_sem_wait(msg.op_completed,0);
		SSNP_DEBUG_PRINT("uctsk_SSNP_dp_recv(): TBIM exe done trigger.\r\n");
		netbuf_delete(buf);
		
		OSTimeDly(1);
	}
}
static void uctsk_SSNP_sdp(void* pdata)
{
	send_sampling_data();
}
static void App_SSNP_DP_TaskCreate(u8t* alias)
{
	CPU_INT08U  os_err;

	os_err = os_err; /* prevent warning... */
	
	os_err=OSTaskCreate((void (*)(void *)) uctsk_SSNP_dp_recv,	
		                   (void*          ) alias,
											 (OS_STK*        ) &AppSSNPdpTaskStk[APP_TASK_DP_STK_SIZE -1],
											 (INT8U          ) APP_TASK_SSNP_DP_PRIO);
}
static void App_SSNP_SDP_TaskCreate(u8t* alias)
{
	CPU_INT08U  os_err;

	os_err = os_err; /* prevent warning... */
	
	os_err=OSTaskCreate((void (*)(void *)) uctsk_SSNP_sdp,	
		                   (void*          ) alias,
											 (OS_STK*        ) &AppSSNPsdpTaskStk[APP_TASK_SDP_STK_SIZE -1],
											 (INT8U          ) APP_TASK_SSNP_SDP_PRIO);
}
static void App_SSNP_TP_TaskCreate(u8t* alias)
{
	CPU_INT08U  os_err;

	os_err = os_err; /* prevent warning... */
	
	os_err=OSTaskCreate((void (*)(void *)) uctsk_SSNP_tp_recv,	
		                   (void*          ) alias,
											 (OS_STK*        ) &AppSSNPtpTaskStk[APP_TASK_TP_STK_SIZE -1],
											 (INT8U          ) APP_TASK_SSNP_TP_PRIO);
}

static void uctsk_SSNP(void* pdata); 
static void uctsk_SSNP_TBIM(void* pdata); 
static void uctsk_SSNP_TBIM_new(void* pdata);
static void uctsk_SSNP_TBIM_new_new(void* pdata);
void  App_SSNPTaskCreate (void)//ssnp应用线程
{
    CPU_INT08U  os_err;

	os_err = os_err; /* prevent warning... */

	os_err = OSTaskCreate((void (*)(void *)) uctsk_SSNP_TBIM_new,				
                          (void          * ) 0,							
                          (OS_STK        * )&AppSSNPTaskStk[APP_TASK_SSNP_STK_SIZE - 1],		
                          (INT8U           ) APP_TASK_SSNP_PRIO  );							
}



static void uctsk_SSNP(void* pdata)
{
	u8t alias;
	struct TBIM_msg* msg;
	struct TBIM_arg arg;
	void* data;
	u8t* dataptr;
	u8t cmd_class;
	u8t cmd_func;
	u16t len;
	u16t time_f;
	u16t time_n;
	u16t time;
	u16t *syn_itv;
	u16t *asyn_itv;
	u16t *begin_timeslot;
	state st;

	
	hardware_init();
	protocol_init();
  app_init();	
	SSNP_DEBUG_PRINT("uctsk_SSNP():init ok.\r\n");
	data=NULL;
	st=FREE;

	alias=get_alias();
	TBIM_init(alias);
	SSNP_DEBUG_PRINT("uctsk_SSNP():get alias ok.\r\n");                      
	App_SSNP_DP_TaskCreate(&alias);
	App_SSNP_TP_TaskCreate(&alias);
	SSNP_DEBUG_PRINT("uctsk_SSNP():dp and tp thread start ok.\r\n");
	
	//注意TBC在发送一次begin epoch需要92us时间，这段时间足够TBIM执行到这里
	while(!is_streaming_ok());//#未完成#：这里可能会有点问题，一个线程不断检测另一个线程的变量，这个会不会有点问题？
	
	get_syn_asyn_begin_timeslot(syn_itv,asyn_itv,begin_timeslot);
	TIM5_delay_us(TIME_SLOT_LEN - DELAY); //等待第一个时间槽
	time_f=TIM_GetCounter(TIM5);//第一个时间槽开始时间
	
	//#未完成#：这里应该将time_f退回到第零个时间槽
	st=SYN;
	
	for(;;)
	{
		switch(st)
		{
			case FREE:
			{
				break;
			}
			case SYN:
			{
				if((*begin_timeslot) == 1)
				{
					//依次发送此TBIM中所有的传感器数据
				}
				else
				{
					TIM5_delay_us(((*begin_timeslot) - 1) * TIME_SLOT_LEN);//等待自己的时间槽
					//依次发送此TBIM中所有的传感器数据
				}
				while(st == SYN)
				{
					time_n=TIM_GetCounter(TIM5);
					time = time_n > time_f ? time_n - time_f : 0xffff - time_f + time_n;
					if(time >= (*syn_itv + 1) * TIME_SLOT_LEN)//这里还有一个空闲槽
					{
						time_f=TIM_GetCounter(TIM5);
						st=ASYN;
						break;
					}
				}
				break;
			}
			case ASYN:
			{
				while(st == ASYN)                           
				{
					sys_arch_mbox_fetch(tbim_mbox,(void*)&msg,0);
					switch(msg->type)
					{
						case DP:
						{  
							SSNP_DEBUG_PRINT("uctsk_SSNP():got dp cmd.\r\n");
							arg.conn=msg->conn;
							netbuf_data(msg->buf,&data,&len); 
							dataptr=(u8t*)data;
							cmd_class=*dataptr;
							cmd_func=*(++dataptr);
							arg.arg=(void*)(++dataptr);
							arg.alias=msg->buf->alias;
							arg.tdcn_num=msg->buf->tdcn_num;
							arg.tbim_tdcn_num=msg->buf->tbim_tdcn_num;
							execute(cmd_class,cmd_func,0,(void*)&arg);
							sys_sem_signal(msg->op_completed);
							
							break;
						}
						case TP:
						{  
							SSNP_DEBUG_PRINT("uctsk_SSNP():TBIM get trigger.\r\n"); 
							arg.conn=msg->conn;
							arg.alias=msg->buf->alias;
							arg.tdcn_num=msg->buf->tdcn_num;
							arg.tbim_tdcn_num=msg->buf->tbim_tdcn_num;
							trigger((void*)&arg);
							sys_sem_signal(msg->op_completed);
							
							break;
						}
						default:
						{
							break;//#未完成#：这里是不是还需要其他的操作比如删除接收到的数据，不需要因为程序中没有设定其他类型。
						}
					}
					time_n=TIM_GetCounter(TIM5) + RECV_SEND_TIME;//预留出和回复的时间
					if(time_n >= (*asyn_itv) * TIME_SLOT_LEN)
					{
						TIM5_delay_us(RECV_SEND_TIME);
						st=SYN;
						time_f=TIM_GetCounter(TIM5);///#未完成#：这里是不是应该将time_f等待到第一个时间槽？/
						break;
					}
				}
				break;
			}
			default:break;
		}
	}
//	
//	for(;;)                           
//	{
//		sys_arch_mbox_fetch(tbim_mbox,(void*)&msg,0);
//		switch(msg->type)
//		{
//			case DP:
//			{  
//				SSNP_DEBUG_PRINT("uctsk_SSNP():got dp cmd.\r\n");
//				arg.conn=msg->conn;
//				netbuf_data(msg->buf,&data,&len); 
//				dataptr=(u8t*)data;
//				cmd_class=*dataptr;
//				cmd_func=*(++dataptr);
//				arg.arg=(void*)(++dataptr);
//				arg.alias=msg->buf->alias;
//				arg.tdcn_num=msg->buf->tdcn_num;
//				arg.tbim_tdcn_num=msg->buf->tbim_tdcn_num;
//				execute(cmd_class,cmd_func,(void*)&arg);
//				sys_sem_signal(msg->op_completed);
//				
//				break;
//			}
//			case TP:
//			{  
//				SSNP_DEBUG_PRINT("uctsk_SSNP():TBIM get trigger.\r\n"); 
//				arg.conn=msg->conn;
//				arg.alias=msg->buf->alias;
//				arg.tdcn_num=msg->buf->tdcn_num;
//				arg.tbim_tdcn_num=msg->buf->tbim_tdcn_num;
//				trigger((void*)&arg);
//				sys_sem_signal(msg->op_completed);
//				
//				break;
//			}
//			default:
//			{
//				break;//#未完成#：这里是不是还需要其他的操作比如删除接收到的数据，不需要因为程序中没有设定其他类型。
//			}
//		}
//		OSTimeDly(1);		
//	}
}
//注意这里还没有考虑新增TBIM，也就是重新接收到分配时间片命令的情况
static void uctsk_SSNP_TBIM(void* pdata)
{
	u8t alias;
	void* data;
	u8t cmd_class;
	u8t cmd_func;
	void* arguement;
	u16t time_f;
	u16t time_n;
	u16t time;
	u16t syn_itv;
	u16t asyn_itv;
	u16t begin_timeslot;
	u8t k;
	state st;
	u8t Chn_num;
	u8t tdcn_sn;
	
	hardware_init();
	protocol_init();
	app_init();	
	
	SSNP_DEBUG_PRINT("uctsk_SSNP():init ok.\r\n");
	data=NULL;
	st=FREE;


	SSNP_DEBUG_PRINT("uctsk_SSNP():waitting for alias.\r\n");	
	alias=get_alias();
	TBIM_init(alias);
	Chn_num=get_TBIM_Chn_num();
	printf("uctsk_SSNP():get alias %d.\r\n",alias);


	while(1)
	{
		switch(st)
		{
			case FREE:
			{
				TBIM_recv_cmd_and_execute();
				if(get_TBIM_init_state() == WAITTING_FOR_TIME)
				{

					//时间测量函数
					SSNP_DEBUG_PRINT("uctsk_SSNP():waiting for reflect msg.\r\n");
				    //
					for(k=0;k<DELAY_TIME_MEASURE_NUM;k++)//这里只是用来测量时延
					{
						while(!is_reflect_ok());
						tbim_send_reflect_reply_msg();
						set_reflect_ok_unfinished_TEST();
					}
					SSNP_DEBUG_PRINT("uctsk_SSNP():send reflect reply msg done.\r\n");
	
	
					while(!is_assign_time_slot_done());
					get_syn_asyn_begin_timeslot(&syn_itv,&asyn_itv,&begin_timeslot); 
					
					//here，生成二维表，根据自己的最大周期生成二维表
					
					
					//注意TBC在发送一次begin epoch需要92us时间，这段时间足够TBIM执行到这里
					while(!is_streaming_ok());//#未完成#：这里可能会有点问题，一个线程不断检测另一个线程的变量，这个会不会有点问题？
					TIM5_delay_us(TIME_SLOT_LEN - DELAY); //等待第一个时间槽,这里的DELAY就是上面需要测量的时间延迟
					time_f=TIM_GetCounter(TIM5);//第一个时间槽开始时间	
					st=SYN;
				}
				break;
			}
			case SYN:
			{
				if((begin_timeslot) != 1)
				{
					TIM5_delay_us(((begin_timeslot) - 1) * TIME_SLOT_LEN);//等待自己的时间槽
					time_f=TIM_GetCounter(TIM5);//记下自己时间槽开始的时间
				}
					
				for(tdcn_sn=0;tdcn_sn<Chn_num;Chn_num++)//依次发送此TBIM中所有的传感器数据
				{
					data=get_sensor_data(tdcn_sn);
					
					if(data != NULL)
					{
						TBIM_send_streaming_data(tdcn_sn,data);
						
						time_n=TIM_GetCounter(TIM5);
						time = time_n >= time_f ? time_n - time_f : 0xffff - time_f + time_n;
						TIM5_delay_us(TIME_SLOT_LEN - time);//发送完数据后在自己的时间槽中等待
						time_f=TIM_GetCounter(TIM5);
					}
					else
					{
						time_n=TIM_GetCounter(TIM5);//没有数据发送完数据后在自己的时间槽中等待	
						time = time_n >= time_f ? time_n - time_f : 0xffff - time_f + time_n;
						TIM5_delay_us(TIME_SLOT_LEN - time);		
						time_f=TIM_GetCounter(TIM5);							
					}
				}
				
				TIM5_delay_us(TIME_SLOT_LEN * ( (syn_itv) - (begin_timeslot) + 1) );//有一个空闲时间槽
				time_f=TIM_GetCounter(TIM5);
				st=ASYN;
				break;
			}
			case ASYN:
			{
				while(st == ASYN)
				{
					TBIM_recv_cmd_and_execute();
					
					time_n=TIM_GetCounter(TIM5);
					time = time_n >= time_f ? time_n - time_f : 0xffff - time_f + time_n;
					
					if(time + TRIGGER_RECV_EXE_TIME >= TIME_SLOT_LEN * (asyn_itv))//如果剩余的时间不足以接收并执行一个trigger
					{
						TIM5_delay_us(TIME_SLOT_LEN * (asyn_itv + 1 ) - time);//注意这里跳过了第0号时间槽
						time_f=TIM_GetCounter(TIM5);
						st=SYN;
						break;
					}
		
					TBIM_recv_trigger_and_execute();
					
					time_n=TIM_GetCounter(TIM5);
					time = time_n >= time_f ? time_n - time_f : 0xffff - time_f + time_n;	
					
					if(time + CMD_RECV_EXE_TIME >= TIME_SLOT_LEN * (asyn_itv))//如果剩余的时间不足以接收并执行一个命令
					{
						TIM5_delay_us(TIME_SLOT_LEN * (asyn_itv + 1 ) - time);//注意这里跳过了第0号时间槽
						time_f=TIM_GetCounter(TIM5);
						st=SYN;
						break;
					}					
				}
				break;
			}
			default:break;
		}
	}
}

u16t syn_time;//同步等待时间 (从同步开始到此TBIM第一个时间槽的时间长度)
u16t first_syn_delay_time;//同步等待时间去掉传输延迟(同步时间槽去掉延迟的时间长度)
u16t asyn_time;//异步时间间隔(从异步时间开始到结束)
u16t wait_for_asyn_time;//(当发送完所有的变送器数据后等待进入异步区间的时间，当然最后一个TBIM是不需要这个时间段的)
u16t time_h;//head
u16t time_t;//tail
u16t time;//时间间隔
u8t tdcn_sequence;//当前需要发送流数据的传感器序列号
u8t isfirst=1;
typedef void (*func)(void);
#define DEBUGING 0
static void first_syn_delay(void)
{
#if DEBUGING == 0
	if(isfirst)
	{
		TIM5_delay_us(first_syn_delay_time);
		isfirst=0;
	}
	else
		TIM5_delay_us(syn_time);
#endif
}
static void syn_delay(void)
{
#if DEBUGING == 0
	TIM5_delay_us(syn_time);
	//printf("syn_time():syn_time is %d.\r\n",syn_time);
#endif
}
static void time_slot_delay(void)
{
#if DEBUGING == 0
	TIM5_delay_us(TIME_SLOT_LEN);
#endif
}
static void wait_for_asyn(void)
{
#if DEBUGING == 0
	TIM5_delay_us(wait_for_asyn_time);
	//printf("wait_for_asyn():wait_for_asyn_time is %d.\r\n",wait_for_asyn_time);
#endif
}
u32t number=0;
u16t a;
u16t b;
u16t t;
static void send_streaming_date(void)
{
	u8t data[2];
	time_h=TIM_GetCounter(TIM5);
	 printf("send.\r\n");
	++number;	
	if(data != NULL)
	{
		TBIM_send_streaming_data(tdcn_sequence,&number);
	
		time_t=TIM_GetCounter(TIM5);
		time = time_h >= time_t ? time_h - time_t : 0xffff - time_t + time_h;
		TIM5_delay_us(TIME_SLOT_LEN - time);//发送完数据后在自己的时间槽中等待
	}
	else
	{
		time_t=TIM_GetCounter(TIM5);
		time = time_h >= time_t ? time_h - time_t : 0xffff - time_t + time_h;
		TIM5_delay_us(TIME_SLOT_LEN - time);							
	}

}

static void recv_exe_cmd(void)
{
#if DEBUGING == 0
//	u16t time_e;

	time_h=TIM_GetCounter(TIM5);
//	printf("recv_exe_cmd():time_h is %d.\r\n",time_h);
	while(1)
	{
		TBIM_recv_cmd_and_execute();	
		time_t=TIM_GetCounter(TIM5);
		time =time_h >= time_t ? time_h - time_t : 0xffff - time_t + time_h;
		if(time + CMD_RECV_EXE_TIME >= asyn_time)//如果剩余的时间不足以接收并执行一个trigger
		{
			TIM5_delay_us(asyn_time - time);//注意这里跳过了第0号时间槽
			break;
		}
		
		TBIM_recv_trigger_and_execute();	
		time_t=TIM_GetCounter(TIM5);
		time = time_h >= time_t ? time_h - time_t : 0xffff - time_t + time_h;
			
		if(time + TRIGGER_RECV_EXE_TIME >= asyn_time)//如果剩余的时间不足以接收并执行一个命令
		{
			TIM5_delay_us(asyn_time- time);//注意这里跳过了第0号时间槽
			break;
		}							
	}
//	time_e=TIM_GetCounter(TIM5);
//	printf("recv_exe_cmd():time_e is %d.\r\n",TIM_GetCounter(TIM5));
//  printf("recv_exe_cmd():the time tt %d,total is %d and is %d.\r\n",time,asyn_time- time,time_e >= time_h ? time_e - time_h : 0xffff - time_h + time_e);
#endif
}	
u8t* table;
int send_number=0; 
u8t isOk_now=0;
int be_gin;

static void uctsk_SSNP_TBIM_new(void* pdata)
{
	u8t alias;
	void* data;
	u16t syn_itv;
	u16t asyn_itv;
	u16t begin_timeslot;
	u8t k;
	u16t i;
	u8t j;
	state st;
	u8t Chn_num;
	u8t tdcn_sn;
	func** f_table;

	u8t func_table_row;
	u8t func_table_col;
	u8t factor;
	
	hardware_init();
	protocol_init();
	app_init();	
	
	SSNP_DEBUG_PRINT("\r\nTBIM init ok.\r\n");
	data=NULL;
	st=FREE;
	


	SSNP_DEBUG_PRINT("waitting for alias.\r\n");	
	alias=get_alias();
	TBIM_init(alias);
	Chn_num=get_TBIM_Chn_num();
	printf("uctsk_SSNP():get alias %d.\r\n",alias);

	while(1)
	{
		switch(st)
		{
			case FREE:
			{
				TBIM_recv_cmd_and_execute();
				if(get_TBIM_init_state() == WAITTING_FOR_TIME)
				{

					//时间测量函数
					SSNP_DEBUG_PRINT("waiting for reflect msg.\r\n");
				    //
					for(k=0;k<DELAY_TIME_MEASURE_NUM;k++)//这里只是用来测量时延
					{
						while(!is_reflect_ok());
						tbim_send_reflect_reply_msg();
						set_reflect_ok_unfinished_TEST();
					}
					SSNP_DEBUG_PRINT("send reflect reply msg done.\r\n");
	
	
					while(!is_assign_time_slot_done());
					SSNP_DEBUG_PRINT("get all time msg.\r\n");
					get_syn_asyn_begin_timeslot(&syn_itv,&asyn_itv,&begin_timeslot);
				//	printf("uctsk_SSNP():TBIM get syn_itv is %d,asyn_itv is %d,begin_timeslot is %d.\r\n",syn_itv,asyn_itv,begin_timeslot); 
					
					//生成二维表，根据最大周期生成二维表
					//列数为max_period()/(syn_itv+asyn_itv)
					//行数为1+传感器个数+1+1
					syn_time=begin_timeslot*TIME_SLOT_LEN;//同步时间长度，(从同步开始到此TBIM第一个时间槽的时间长度)

					first_syn_delay_time=TIME_SLOT_LEN-152/2;//(同步时间槽去掉延迟的时间长度)
					asyn_time=(asyn_itv-1)*TIME_SLOT_LEN;//(从异步时间开始到结束)

					//func_table_row=max_period()*10/(syn_itv+asyn_itv);
					func_table_row=100*10/(syn_itv+asyn_itv); //这里假定最大传感器周期是100ms
				//	printf("uctsk_SSNP():begin_timeslot is %d.\r\n",begin_timeslot);
					be_gin=begin_timeslot-1;
				//	printf("uctsk_SSNP():syn_itv is %d.\r\n",syn_itv);
				//	printf("uctsk_SSNP():asyn_itv is %d.\r\n",asyn_itv);
				//	printf("uctsk_SSNP():max_period is %d.\r\n",max_period());

					Chn_num =1;//这里假定只有一个传感器
					if(begin_timeslot+Chn_num==syn_itv-1)
						func_table_col=1+Chn_num+2;
					else
					{
						func_table_col=1+Chn_num+1+2;//新添加出一列
						wait_for_asyn_time=(syn_itv-(begin_timeslot+Chn_num)-1)*TIME_SLOT_LEN;
					}
					test_schedul_table();
					set_asy_iso_num(1+64);
				//	printf("uctsk_SSNP():fucn_table_row is %d.\r\n",func_table_row);
				//	printf("uctsk_SSNP():func_table_col is %d.\r\n",func_table_col);
		
					//注意TBC在发送一次begin epoch需要92us时间，这段时间足够TBIM执行到这里
					//App_SSNP_SDP_TaskCreate(NULL);
					while(!is_streaming_ok());//#未完成#：这里可能会有点问题，一个线程不断检测另一个线程的变量，这个会不会有点问题？
					st=WORKING;
					send_number=0;
					isOk_now=1;
					TIM5_delay_us(first_syn_delay_time);
					NVIC_Configuration_TIM5();
					//printf("recv begin epoch msg,send data.\r\n");
				    while(1)
						TBIM_recv_cmd_and_execute();
					
				}
				break;
			}
			case WORKING:
			{
					while(1)
					{}
				break;
			}
			default:break;
		}
	}
}  
static void uctsk_SSNP_TBIM_new_new(void* pdata)
{
hardware_init();
protocol_init();
	printf("init.\r\n");
  	TIM5_Init();
	NVIC_Configuration_TIM5();
	printf("TIM5 init.\r\n");
	TBIM_init(1);
	OSTimeDly(2000);	
	isOk_now=1;
	
	while(1)
	{
		OSTimeDly(1);
	}
}
static err_t send_streaming_date_new(void)
{
	++tdcn_sequence;
	if(tdcn_sequence!=1)
	{
		number=sensor_manage(tdcn_sequence);
		if(tdcn_sequence==17)
			tdcn_sequence=0;
		return TBIM_send_streaming_data(tdcn_sequence,&number);
	}
}
void TIM5_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM5,TIM_IT_Update)!= RESET )
	{
		TIM_ClearITPendingBit(TIM5,TIM_IT_Update);
		if(can_send_streaming_data())
			//SIGNAL;
			send_sampling_data_test();
	}
}
//static err_t send_sdp_data(void)
//{
//	u32t data;
//	while(1)
//	{
//		WAIT; 
//		data=sensor_manage(tbim.schedultable[tbim.time_slot_seq].tdcn);
//		return TBIM_send_streaming_data(tbim.schedultable[tbim.time_slot_seq].tdcn,&data);
//	}	
//}
int epoch_num=0;
int send_num=0;
/*
	在等待第一个时间槽过后，开中断，这时还要等待100us才可以开启第一个中断
*/
void TIM5_IRQHandler_test1(void)
{
	if(TIM_GetITStatus(TIM5,TIM_IT_Update)!= RESET )
	{
		TIM_ClearITPendingBit(TIM5,TIM_IT_Update);
		if(isOk_now)
		{ 
			++send_number;
			if(be_gin*16+1<=send_number && send_number <= (be_gin+1)*16)
			{
				send_streaming_date_new();
			}
			if(send_number%64==0)
			{
				send_number=0;
			}	
		}
	}
}
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART2, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
  {}

  return ch;
}
#endif
