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
#define RECV_SEND_TIME        		200 //#δ���#�����������պͻظ�������Ҫʹ��200us
#define TIME_SLOT_LEN	        		100 //�����趨ʱ���Ϊ100us
	
#define DELAY                  		50  //��TBC��ʼ���͵�TBIMӦ�ò���յ���ʱ��                   #����#
#define TRIGGER_RECV_EXE_TIME  		100 //�����ʱ�䶨��ΪTBIM�㹻���ղ�ִ��һ��trigger��ʱ��     #����#
#define CMD_RECV_EXE_TIME      		200 //�����ʱ�䶨��ΪTBIM�㹻������ִ��һ�������ʱ��        #����#
#define DELAY_TIME_MEASURE_NUM    20
	
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
	struct netbuf* buf;//ֻ��dp���ݲ���Ҫ������tp����Ҫ����
	sys_sem_t op_completed;
};


//#define TBIM_MSG_NUM  10
//static OS_MEM* tbim_msg;
//static u8t tbim_msg_pool[TBIM_MSG_NUM * sizeof(struct TBIM_msg)];

static sys_mbox_t tbim_mbox;



/*
------------------------------------------------------------------------------------------------------------
	һ��Ҫע������ļ�ʱ��Χһ�������65536us,Ҳ���Ǵ��65ms,ע������ʱ��β�Ҫ����65ms����û�а취��ʱ
------------------------------------------------------------------------------------------------------------
*/
static void TIM5_Init()
{
	TIM_TimeBaseInitTypeDef Tim5;  
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);  
  	Tim5.TIM_Period=100-1;	//ע��TBIM�ж�����100us
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

    /* Enable the ETH Interrupt ������ʹ����̫���ж�*/
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
static void TIM5_delay_us(u16t us)//ʹ�ö�ʱ�������Լ��ж���
{
	TIM_SetCounter(TIM5,us);
	while(us > 1)
		us=TIM_GetCounter(TIM5);
}

void begin_epoch_callback(u16t syn_timeslot,u16t asyn_timeslot)
{
	///time_f=TIM_GetCounter(TIM5);//��ʱ��ʼ
}
static void app_init()
{
//	u8t err;
//	tbim_msg=OSMemCreate((void*)tbim_msg_pool,TBIM_MSG_NUM,sizeof(struct TBIM_msg),&err);
	
//	tbim_mbox=sys_mbox_new(MAX_QUEUE_ENTRIES);
	TIM5_Init();
//	NVIC_Configuration_TIM5();
}

/***********************************************************************************************
 * Ӧ�ó�������߳�ջ��С
 ***********************************************************************************************/
static OS_STK AppSSNPTaskStk[APP_TASK_SSNP_STK_SIZE];//ssnpӦ�ó������߳�ջ��С

static OS_STK AppSSNPdpTaskStk[APP_TASK_DP_STK_SIZE];//ssnpӦ��dp�߳�ջ��С
static OS_STK AppSSNPtpTaskStk[APP_TASK_TP_STK_SIZE];//ssnpӦ��tp�߳�ջ��С
//static OS_STK AppSSNPsdpTaskStk[APP_TASK_SDP_STK_SIZE];//ssnpӦ��sdp�߳�ջ��С


static void uctsk_SSNP_dp_recv(void* pdata)
{
	struct netconn* conn;
	struct netbuf* buf;
	struct TBIM_msg msg;
	u8t* alias;

	
	alias=pdata;
	msg.op_completed=sys_sem_new(0);
	
	conn=netconn_new(NETCONN_DP);
	netconn_bind(conn,*alias,0);//�Ƚ������󶨵�0
	netconn_connect(conn,TBC_ADDRESS,0);//TBC�ı�������1
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
static void App_SSNP_DP_TaskCreate(u8t* alias)
{
	CPU_INT08U  os_err;

	os_err = os_err; /* prevent warning... */
	
	os_err=OSTaskCreate((void (*)(void *)) uctsk_SSNP_dp_recv,	
		                   (void*          ) alias,
											 (OS_STK*        ) &AppSSNPdpTaskStk[APP_TASK_DP_STK_SIZE -1],
											 (INT8U          ) APP_TASK_SSNP_DP_PRIO);
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
void  App_SSNPTaskCreate (void)//ssnpӦ���߳�
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
	
	//ע��TBC�ڷ���һ��begin epoch��Ҫ92usʱ�䣬���ʱ���㹻TBIMִ�е�����
	while(!is_streaming_ok());//#δ���#��������ܻ��е����⣬һ���̲߳��ϼ����һ���̵߳ı���������᲻���е����⣿
	
	get_syn_asyn_begin_timeslot(syn_itv,asyn_itv,begin_timeslot);
	TIM5_delay_us(TIME_SLOT_LEN - DELAY); //�ȴ���һ��ʱ���
	time_f=TIM_GetCounter(TIM5);//��һ��ʱ��ۿ�ʼʱ��
	
	//#δ���#������Ӧ�ý�time_f�˻ص������ʱ���
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
					//���η��ʹ�TBIM�����еĴ���������
				}
				else
				{
					TIM5_delay_us(((*begin_timeslot) - 1) * TIME_SLOT_LEN);//�ȴ��Լ���ʱ���
					//���η��ʹ�TBIM�����еĴ���������
				}
				while(st == SYN)
				{
					time_n=TIM_GetCounter(TIM5);
					time = time_n > time_f ? time_n - time_f : 0xffff - time_f + time_n;
					if(time >= (*syn_itv + 1) * TIME_SLOT_LEN)//���ﻹ��һ�����в�
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
							break;//#δ���#�������ǲ��ǻ���Ҫ�����Ĳ�������ɾ�����յ������ݣ�����Ҫ��Ϊ������û���趨�������͡�
						}
					}
					time_n=TIM_GetCounter(TIM5) + RECV_SEND_TIME;//Ԥ�����ͻظ���ʱ��
					if(time_n >= (*asyn_itv) * TIME_SLOT_LEN)
					{
						TIM5_delay_us(RECV_SEND_TIME);
						st=SYN;
						time_f=TIM_GetCounter(TIM5);///#δ���#�������ǲ���Ӧ�ý�time_f�ȴ�����һ��ʱ��ۣ�/
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
//				break;//#δ���#�������ǲ��ǻ���Ҫ�����Ĳ�������ɾ�����յ������ݣ�����Ҫ��Ϊ������û���趨�������͡�
//			}
//		}
//		OSTimeDly(1);		
//	}
}
//ע�����ﻹû�п�������TBIM��Ҳ�������½��յ�����ʱ��Ƭ��������
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

					//ʱ���������
					SSNP_DEBUG_PRINT("uctsk_SSNP():waiting for reflect msg.\r\n");
				    //
					for(k=0;k<DELAY_TIME_MEASURE_NUM;k++)//����ֻ����������ʱ��
					{
						while(!is_reflect_ok());
						tbim_send_reflect_reply_msg();
						set_reflect_ok_unfinished_TEST();
					}
					SSNP_DEBUG_PRINT("uctsk_SSNP():send reflect reply msg done.\r\n");
	
	
					while(!is_assign_time_slot_done());
					get_syn_asyn_begin_timeslot(&syn_itv,&asyn_itv,&begin_timeslot); 
					
					//here�����ɶ�ά�������Լ�������������ɶ�ά��
					
					
					//ע��TBC�ڷ���һ��begin epoch��Ҫ92usʱ�䣬���ʱ���㹻TBIMִ�е�����
					while(!is_streaming_ok());//#δ���#��������ܻ��е����⣬һ���̲߳��ϼ����һ���̵߳ı���������᲻���е����⣿
					TIM5_delay_us(TIME_SLOT_LEN - DELAY); //�ȴ���һ��ʱ���,�����DELAY����������Ҫ������ʱ���ӳ�
					time_f=TIM_GetCounter(TIM5);//��һ��ʱ��ۿ�ʼʱ��	
					st=SYN;
				}
				break;
			}
			case SYN:
			{
				if((begin_timeslot) != 1)
				{
					TIM5_delay_us(((begin_timeslot) - 1) * TIME_SLOT_LEN);//�ȴ��Լ���ʱ���
					time_f=TIM_GetCounter(TIM5);//�����Լ�ʱ��ۿ�ʼ��ʱ��
				}
					
				for(tdcn_sn=0;tdcn_sn<Chn_num;Chn_num++)//���η��ʹ�TBIM�����еĴ���������
				{
					data=get_sensor_data(tdcn_sn);
					
					if(data != NULL)
					{
						TBIM_send_streaming_data(tdcn_sn,data);
						
						time_n=TIM_GetCounter(TIM5);
						time = time_n >= time_f ? time_n - time_f : 0xffff - time_f + time_n;
						TIM5_delay_us(TIME_SLOT_LEN - time);//���������ݺ����Լ���ʱ����еȴ�
						time_f=TIM_GetCounter(TIM5);
					}
					else
					{
						time_n=TIM_GetCounter(TIM5);//û�����ݷ��������ݺ����Լ���ʱ����еȴ�	
						time = time_n >= time_f ? time_n - time_f : 0xffff - time_f + time_n;
						TIM5_delay_us(TIME_SLOT_LEN - time);		
						time_f=TIM_GetCounter(TIM5);							
					}
				}
				
				TIM5_delay_us(TIME_SLOT_LEN * ( (syn_itv) - (begin_timeslot) + 1) );//��һ������ʱ���
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
					
					if(time + TRIGGER_RECV_EXE_TIME >= TIME_SLOT_LEN * (asyn_itv))//���ʣ���ʱ�䲻���Խ��ղ�ִ��һ��trigger
					{
						TIM5_delay_us(TIME_SLOT_LEN * (asyn_itv + 1 ) - time);//ע�����������˵�0��ʱ���
						time_f=TIM_GetCounter(TIM5);
						st=SYN;
						break;
					}
		
					TBIM_recv_trigger_and_execute();
					
					time_n=TIM_GetCounter(TIM5);
					time = time_n >= time_f ? time_n - time_f : 0xffff - time_f + time_n;	
					
					if(time + CMD_RECV_EXE_TIME >= TIME_SLOT_LEN * (asyn_itv))//���ʣ���ʱ�䲻���Խ��ղ�ִ��һ������
					{
						TIM5_delay_us(TIME_SLOT_LEN * (asyn_itv + 1 ) - time);//ע�����������˵�0��ʱ���
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

u16t syn_time;//ͬ���ȴ�ʱ�� (��ͬ����ʼ����TBIM��һ��ʱ��۵�ʱ�䳤��)
u16t first_syn_delay_time;//ͬ���ȴ�ʱ��ȥ�������ӳ�(ͬ��ʱ���ȥ���ӳٵ�ʱ�䳤��)
u16t asyn_time;//�첽ʱ����(���첽ʱ�俪ʼ������)
u16t wait_for_asyn_time;//(�����������еı��������ݺ�ȴ������첽�����ʱ�䣬��Ȼ���һ��TBIM�ǲ���Ҫ���ʱ��ε�)
u16t time_h;//head
u16t time_t;//tail
u16t time;//ʱ����
u8t tdcn_sequence;//��ǰ��Ҫ���������ݵĴ��������к�
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
//#if DEBUGING == 0
	//void* data;
	u8t data[2];
	time_h=TIM_GetCounter(TIM5);
	//data=get_sensor_data(tdcn_sn);//����ʹ��һ���ٶ�������
	 printf("send.\r\n");
	++number;	
	if(data != NULL)
	{
	//	printf("send_streaming_data():sending data %d.\r\n",tdcn_sequence);
	//	a=TIM_GetCounter(TIM5);
		TBIM_send_streaming_data(tdcn_sequence,&number);
//		b=TIM_GetCounter(TIM5);
	
//		printf("send_streaming_data():time is %d.\r\n",a>b?a-b:0xffff-b+a);
		
		time_t=TIM_GetCounter(TIM5);
		time = time_h >= time_t ? time_h - time_t : 0xffff - time_t + time_h;
		TIM5_delay_us(TIME_SLOT_LEN - time);//���������ݺ����Լ���ʱ����еȴ�
	}
	else
	{
		time_t=TIM_GetCounter(TIM5);
		time = time_h >= time_t ? time_h - time_t : 0xffff - time_t + time_h;
		TIM5_delay_us(TIME_SLOT_LEN - time);							
	}
	//printf("send_streaming_data():time is %d.\r\n",time);
//#endif
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
		if(time + CMD_RECV_EXE_TIME >= asyn_time)//���ʣ���ʱ�䲻���Խ��ղ�ִ��һ��trigger
		{
			TIM5_delay_us(asyn_time - time);//ע�����������˵�0��ʱ���
			break;
		}
		
		TBIM_recv_trigger_and_execute();	
		time_t=TIM_GetCounter(TIM5);
		time = time_h >= time_t ? time_h - time_t : 0xffff - time_t + time_h;
			
		if(time + TRIGGER_RECV_EXE_TIME >= asyn_time)//���ʣ���ʱ�䲻���Խ��ղ�ִ��һ������
		{
			TIM5_delay_us(asyn_time- time);//ע�����������˵�0��ʱ���
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

					//ʱ���������
					SSNP_DEBUG_PRINT("uctsk_SSNP():waiting for reflect msg.\r\n");
				    //
					for(k=0;k<DELAY_TIME_MEASURE_NUM;k++)//����ֻ����������ʱ��
					{
						while(!is_reflect_ok());
						tbim_send_reflect_reply_msg();
						set_reflect_ok_unfinished_TEST();
					}
					SSNP_DEBUG_PRINT("uctsk_SSNP():send reflect reply msg done.\r\n");
	
	
					while(!is_assign_time_slot_done());
					SSNP_DEBUG_PRINT("uctsk_SSNP():get all time msg.\r\n");
					get_syn_asyn_begin_timeslot(&syn_itv,&asyn_itv,&begin_timeslot);
					printf("uctsk_SSNP():TBIM get syn_itv is %d,asyn_itv is %d,begin_timeslot is %d.\r\n",syn_itv,asyn_itv,begin_timeslot); 
					
					//���ɶ�ά����������������ɶ�ά��
					//����Ϊmax_period()/(syn_itv+asyn_itv)
					//����Ϊ1+����������+1+1
					syn_time=begin_timeslot*TIME_SLOT_LEN;//ͬ��ʱ�䳤�ȣ�(��ͬ����ʼ����TBIM��һ��ʱ��۵�ʱ�䳤��)

					first_syn_delay_time=TIME_SLOT_LEN-152/2;//(ͬ��ʱ���ȥ���ӳٵ�ʱ�䳤��)
					asyn_time=(asyn_itv-1)*TIME_SLOT_LEN;//(���첽ʱ�俪ʼ������)

					//func_table_row=max_period()*10/(syn_itv+asyn_itv);
					func_table_row=100*10/(syn_itv+asyn_itv); //����ٶ���󴫸���������100ms
					printf("uctsk_SSNP():begin_timeslot is %d.\r\n",begin_timeslot);
					printf("uctsk_SSNP():syn_itv is %d.\r\n",syn_itv);
					printf("uctsk_SSNP():asyn_itv is %d.\r\n",asyn_itv);
					printf("uctsk_SSNP():max_period is %d.\r\n",max_period());

					Chn_num =1;//����ٶ�ֻ��һ��������
					if(begin_timeslot+Chn_num==syn_itv-1)
						func_table_col=1+Chn_num+2;
					else
					{
						func_table_col=1+Chn_num+1+2;//����ӳ�һ��
						wait_for_asyn_time=(syn_itv-(begin_timeslot+Chn_num)-1)*TIME_SLOT_LEN;
					}
					printf("uctsk_SSNP():fucn_table_row is %d.\r\n",func_table_row);
					printf("uctsk_SSNP():func_table_col is %d.\r\n",func_table_col);
				//	table=(u8t*)mem_alloc(1000*sizeof(u8t));
				//	printf("uctsk_SSNP():table alloc.\r\n");
				//	if(table == NULL)
				//		printf("uctsk_SSNP():table is NULL.\r\n");
				//	for(i=0;i<1000;i++)
				//		table[i]=0;
				//	table[1]=1;
				//	printf("uctsk_SSNP():table init.\r\n");
	/*				f_table=(func**)mem_alloc(func_table_row*sizeof(func**));
					for(i=0;i<func_table_row;i++)
						f_table[i]=(func*)mem_alloc(func_table_col*sizeof(func*));
					for(i=0;i<func_table_row;i++)
					{
						f_table[i][0]=syn_delay;//���ֵ��Ҫ����
						if(begin_timeslot+Chn_num==syn_itv-1)
						{
							f_table[i][begin_timeslot+Chn_num]=time_slot_delay;
							f_table[i][begin_timeslot+Chn_num+1]=recv_exe_cmd;
						}
						else
						{
							f_table[i][begin_timeslot+Chn_num]=wait_for_asyn;
							f_table[i][begin_timeslot+Chn_num+1]=time_slot_delay;
							f_table[i][begin_timeslot+Chn_num+2]=recv_exe_cmd;
						}
					}
					f_table[0][0]=first_syn_delay;
					for(j=1;j<=Chn_num;j++)
					{
						//factor=get_sensor_period(j)*10/(syn_itv+asyn_itv);
						factor=1;//�������factor��1
						for(i=0;i<func_table_row;i++)
						{
							if(i%factor == 0)
								f_table[i][j]=send_streaming_date;
							else
								f_table[i][j]=time_slot_delay;
						}
					}
	*/	
					//ע��TBC�ڷ���һ��begin epoch��Ҫ92usʱ�䣬���ʱ���㹻TBIMִ�е�����
					
					while(!is_streaming_ok());//#δ���#��������ܻ��е����⣬һ���̲߳��ϼ����һ���̵߳ı���������᲻���е����⣿
					st=WORKING;
					send_number=1;
					isOk_now=1;
					TIM5_delay_us(first_syn_delay_time);
					NVIC_Configuration_TIM5();
				//	printf("uctsk_SSNP():ok,here we go.\r\n");
					
				}
				break;
			}
			case WORKING:
			{
			/*	for(;;)
					for(i=0;i<func_table_row;i++)
					{
						a=TIM_GetCounter(TIM5);
						for(j=0;j<func_table_col;j++)
								tdcn_sequence=j,f_table[i][j]();
						b=TIM_GetCounter(TIM5);
						printf("uctsk_SSNP():time is %d.\r\n",a>b?a-b:0xffff-b+a);

					}  */
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
	u8t data[2];
	time_h=TIM_GetCounter(TIM5);
	//data=get_sensor_data(tdcn_sn);//����ʹ��һ���ٶ�������
//	 printf("sendl..\r\n");
	++number;	
	//if(data != NULL)
	return TBIM_send_streaming_data(tdcn_sequence,&number);
}
int epoch_num=0;
int send_num=0;
/*
	�ڵȴ���һ��ʱ��۹��󣬿��жϣ���ʱ��Ҫ�ȴ�100us�ſ��Կ�����һ���ж�
*/
void TIM5_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM5,TIM_IT_Update)!= RESET )
	{
		TIM_ClearITPendingBit(TIM5,TIM_IT_Update);
		if(isOk_now)
		{  //	printf("send_number is %d.\r\n",send_number);
			if(send_number>=6 && send_number<=6 )  //������41 24
			{
			//	a=TIM_GetCounter(TIM5);
				if(send_streaming_date_new() == ERR_OK)
					++send_num;
				//isOk_now=0;
			//	b=TIM_GetCounter(TIM5);
			//	printf("send_streaming_data():time is %d.\r\n",a>b?a-b:100-b+a);
			}
			++send_number;
			if(send_number == 1000)
			{
				send_number=0;
				
				if(epoch_num == 1000) //10600/2=5300
				{
					printf("TIM5_IRQHandler():epoch_num is %d,send_num is %d.\r\n",epoch_num,send_num);
					isOk_now=0;
				}
				
				++epoch_num;

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
