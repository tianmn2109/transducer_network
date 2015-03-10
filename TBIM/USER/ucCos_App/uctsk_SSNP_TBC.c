#include "ssnp/config.h"
#if NODE == TBC
#include <includes.h>
#include <math.h>
#include "ssnp/ssnp_for_app.h"
#include "ssnp/ssnp.h"
#include "ssnp/debug.h"
#include "ssnp/TBC.h"
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/*******************************************************************************
 *
 *   注意初始化顺序    
 *
 *
 *******************************************************************************/
static sys_mbox_t streaming_data_recv_mbox;//TBC需要处理的消息队列，这一队列用于流数据存储
static sys_mbox_t cmd_reply_recv_mbox;
static sys_mbox_t upcomputer_cmd_recv_mbox;//TBC需要处理的消息队列，这一队列用于流数据存储
extern struct TBC_ tbc;
#define SEND_DISMSG_NUM         	6
#define SEND_DISMSG_INTERVSL    	500
#define RCV_MSG_TIME            	100+6// 需要测量的值：接收一次数据并且完全处理完成所需要的时间,比如推送需要的时间6us
#define SEND_MSG_TIME           	100
#define SYN_INTERVAL            	51400//同步时间长度
#define ASYN_INTERVAL           	48600//异步时间长度,异步时间长度这样计算是可以的，因为TBC不需要严格的按照时间片执行，只需要分清同步异步界限就可以了
#define DELAY_TIME_MEASURE_NUM    20
typedef enum
{
	FREE=0,
	SYN,
	ASYN
}state;
/***********************************************************************************************
 * 应用程序各个线程栈大小
 ***********************************************************************************************/
static u32t average_time(u16t arr[],u8t n)
{
	u16t t;
	u8t i;
	u8t k;
	u32t sum;

	for(i=0;i<n;i++)
		printf("%d ",arr[i]);
	printf("\r\n");
	for(i=1;i<n;i++)
	{
		t=arr[i];
		for(k=i;k>=0;k--)
		{
			if(t < arr[k-1])
				arr[k]=arr[k-1];
			else
				break;
		}
		arr[k]=t;
	}
	sum=0;
	for(i=0;i<n;i++)
		printf("%d ",arr[i]);
	printf("\r\n");
	for(i=n/4;i<n/4+n/2;i++)
		sum+=arr[i];
	return sum/(n/2);
}
static void avertime_new(u8t alias,u16t arr[],u8t n)
{
	int sum;
	int max;
	int min;
	double std;
	int diff;
	int aver;
	double res;

	int i;
	sum=0;
	max=-1;
	min=100000000;
	for(i=0;i<n;i++)
	{
		if(arr[i]>max)
			max=arr[i];
		if(arr[i] < min)
			min=arr[i];
		sum+=arr[i];
	}
	aver=sum/n;
	
	diff=0;
	for(i=0;i<n;i++)
		diff+=(arr[i]-aver)*(arr[i]-aver);
	res=(double)(diff/n);
	std=sqrt(res);
	printf("TBIM %d,aver=%d,max=%d,min=%d. max-min=%d,diff=%d,diff/n=%d,std dif=%.6f.\r\n",alias,aver,max,min,max-min,diff,diff/n,std);
	
}
static OS_STK AppSSNPTaskStk[APP_TASK_SSNP_STK_SIZE];//ssnp应用程序主线程栈大小
static void uctsk_SSNP_TBC(void* pdata); 
static void uctsk_SSNP_TBC_new(void* pdata);
void  App_SSNPTaskCreate (void)//ssnp应用线程
{
    CPU_INT08U  os_err;

	os_err = os_err; /* prevent warning... */

	os_err = OSTaskCreate((void (*)(void *)) uctsk_SSNP_TBC_new,				
                          (void          * ) 0,							
                          (OS_STK        * )&AppSSNPTaskStk[APP_TASK_SSNP_STK_SIZE - 1],		
                          (INT8U           ) APP_TASK_SSNP_PRIO  );							
}
static void Delay(u16t time)
{ 
	OSTimeDly(time/(1000/OS_TICKS_PER_SEC));
}
static void TIM5_Init()
{
	TIM_TimeBaseInitTypeDef Tim5;  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);  
  Tim5.TIM_Period=0xffff;
	Tim5.TIM_Prescaler=72-1;
	Tim5.TIM_ClockDivision=0;
 
	Tim5.TIM_CounterMode=TIM_CounterMode_Down;
	TIM_TimeBaseInit(TIM5,&Tim5);
	TIM_Cmd(TIM5,ENABLE);
}  
static void TIM5_delay_us(u16t us)
{
	TIM_SetCounter(TIM5,us);
	while(us > 1)
		us=TIM_GetCounter(TIM5);
}
static void app_init()
{	
	streaming_data_recv_mbox=sys_mbox_new(MAX_QUEUE_ENTRIES);//流数据接收
	if(streaming_data_recv_mbox == SYS_MBOX_NULL)
		printf("app_init():error,streaming_data_recv_mbox is NULL.\r\n");
	cmd_reply_recv_mbox=sys_mbox_new(MAX_QUEUE_ENTRIES);
	if(cmd_reply_recv_mbox == SYS_MBOX_NULL)
		printf("app_init():error,cmd_reply_recv_mbox is NULL.\r\n");
	upcomputer_cmd_recv_mbox=sys_mbox_new(MAX_QUEUE_ENTRIES);
	if(upcomputer_cmd_recv_mbox == SYS_MBOX_NULL)
		printf("app_init():error,upcomputer_cmd_recv_mbox is NULL.\r\n");
	TIM5_Init();
	TBC_init(streaming_data_recv_mbox,cmd_reply_recv_mbox);
}
/*
--------------------------------------------------------------------------------------------------------

		//A:查询命令缓冲区，看是否有上位机命令需要发送，如果需要返回数据则等待一段时间并等待接受返回数据
		//B:查询时间，如果还有足够的空闲时间则接受一下异步数据
		//C:如果此异步数据是新加入的TBIM，则重新进入FREE状态，st=FREE;
		//D:查询时间，如果还有足够的空闲时间则接受一下同步数据
		//E:超时则设置同步时间st=SYN
		//F:break;
	//A:向上位机推送当前在线TBIM的信息
	for(;;)//状态机是不是应该在这里？
	{
		sys_arch_mbox_fetch(mbox,(void*)&msg,0);
		switch(msg->type)
		{
			case TBC_MSG_UPPER:
			{
				tbc_send_cmd_to_tbim(find_tbim(msg->msg.upper_msg.tbim_alias),msg->msg.upper_msg.tdcn_num,msg->msg.upper_msg.cmdclass,msg->msg.upper_msg.cmdfunc);
				break;
			}
			case TBC_MSG_NEW_TBIM:
			{
				TBIM_recongnition(msg->msg.new_tbim_list);//#未完成#：这里还应该在改进一下
				//#未完成#：重新分配时间片
				//#未完成#：重新开始
				break;
			}
			case TBC_MSG_RECV_TBIMSG:
			{
				//#未完成#：查询所有的连接缓冲区是否有接收到的数据，并将其推送到上位机中
				break;
			}
			default:break;
		}
	}
	
	
//	while(1)
//	{
//		time_n=TIM_GetCounter(TIM5);
//		if(time_n > time_f)
//			time=time_n - time_f;
//		else
//			time=0xffff - time_f + time_n;
//		
//		if(time >= 100)
//		{
//			st=SYN;
//			break;
//		}
//	}
---------------------------------------------------------------------------------------------------------
*/
static void uctsk_SSNP_TBC(void* pdata)
{
	struct tbc_msg *msg;
	u8t n_send_dismsg;
	void* tbim_list;
	state st;
	u8t i;
	u8t k;
	u16t delay_time[160];
	u16t time_f;
	u16t time_n;
	u16t time;
	u16t averageTime;

	struct alias_state* p;
	
	tbim_list=NULL;
	st=FREE;
	msg=NULL;
	
	hardware_init();
	protocol_init();
	SSNP_DEBUG_PRINT("uctsk_SSNP():TBC init ok.\r\n");
	app_init();
	
	for(n_send_dismsg=0;n_send_dismsg<SEND_DISMSG_NUM;n_send_dismsg++)
	{
		send_discovery_msg();//这里能直接调用吗？因为内核和这里都使用了netif，会不会出错？但是这里应该是线程安全的，因为对于netif都是只读操作
		Delay(SEND_DISMSG_INTERVSL);
	}
	//识别TBIM
	SSNP_DEBUG_PRINT("uctsk_SSNP():get alias list.\r\n");
	tbim_list=get_tbim();//#未完成#：这里的get_tbim()函数需要修改一下
	if(tbim_list)
	{
		SSNP_DEBUG_PRINT("uctsk_SSNP():recongizing TBIM .\r\n");
		TBIM_recongnition(tbim_list);
	}
	SSNP_DEBUG_PRINT("uctsk_SSNP():recongize TBIM done.\r\n");
	for(i=0;i<MAX_TBIM_NUM;i++)
	{ 
		if(tbc.tbim_table[i].alias != 0)
		{
			//计时
			for(k=0;k<DELAY_TIME_MEASURE_NUM;k++)  //只是用来测量时延
			{
				//printf("uctsk_SSNP():TBC send reflect msg to tbim %d.\r\n",tbc.tbim_table[i].alias);
				time_f=TIM_GetCounter(TIM5);
				
				tbc_send_reflect_msg(tbc.tbim_table[i].alias);
				while(!is_reflect_reply_ok());
				
				time_n=TIM_GetCounter(TIM5);
				time = time_n > time_f ? time_n - time_f : 0xffff - time_f + time_n;
				delay_time[k]=time;
				//printf("uctsk_SSNP():time_f=%d,time_n=%d,the delay time is %d.\r\n",time_f,time_n,time);
				set_reflect_reply_unfinished();
			}
			//#未完成#：这里应该将时间差发送回去
			averageTime=average_time(delay_time,DELAY_TIME_MEASURE_NUM);
		}
	}
	printf("uctsk_SSNP():should send time slot assign msg.\r\n");
	for(;;);
	get_epoch();
	tbc_send_define_epoch_msg(SYN_TIMESLOT_NUM + 2,tbc.min_period*10 - SYN_TIMESLOT_NUM -2);//同步时间预留出两个时间槽
	timeslot_alloc();//分配时间槽,并将时间槽分配信息发送出去

	time_f=TIM_GetCounter(TIM5);//计时开始
	st=SYN;
	tbc_send_begin_of_epoch_msg();//发送开始标识

	for(;;)
	{
		switch(st)
		{
			case FREE:
			{
				//接收所有的sdp数据并push到上位机。
				//识别TBIM，重新分配时间槽，并发送begin epoch msg
				break;
			}
			case SYN:
			{
				/*这里的一个问题是如何确保接收所有的同步数据不会超过同步时间段？注意这里对于TBC来说时间限制不会很严格，只要保证不会在同步时间内发送命令就可以了*/
				/*
				       这里的另一个问题就是如何在同步时间内尽可能多的接收sdp数据，因为如果在这一个epoch中没有接收到那么只能等到下一个epcoh了，这样会造成延迟，这样看来
				   就只能在异步事件的空闲时间中查一下是不是有流数据，但是这种做法有可能会影响命令的发送，有可能导致命令的发送会延迟一个epoch，所以应该如何处理？
				*/
				while(st == SYN)
				{
					for(i=0;i<MAX_TBIM_NUM;i++)
					{
						if(tbc.tbim_table[i].alias != 0)
						{
							//A：查询sdp数据，将其push到串口缓冲中
							//#未完成#：注意对于每个sdp连接的mbox中都可能有多个传感器的sdp数据，这里应该如何很好的处理？
						}
						time_n=TIM_GetCounter(TIM5) + RCV_MSG_TIME;//预留出接收时间
						time = time_n > time_f ? time_n - time_f : 0xffff - time_f + time_n;
						if(time >= SYN_INTERVAL)
						{
							TIM5_delay_us(RCV_MSG_TIME);//不够接收一次数据，那么等待此时间后进入异步状态
							st=ASYN;
							time_f=TIM_GetCounter(TIM5);
							break;
						}
					}
				}
				break;
			}
			case ASYN:
			{
				while(st == ASYN)
				{
					sys_arch_mbox_fetch(upcomputer_cmd_recv_mbox,(void*)&msg,1);//这里我们只等待1ms，因为异步时间间隔一共有50ms
					if(msg != NULL)
					{
						if(msg->type == TBC_MSG_UPPER)
						{/* 这里假设TBC命令不需要回复，否则还需要接收回复消息，#未完成#：处理命令回复数据*/
							TBC_send_noArguCmd_to_tbim(find_tbim(msg->msg.upper_msg.tbim_alias),msg->msg.upper_msg.tdcn_num,msg->msg.upper_msg.cmdclass,msg->msg.upper_msg.cmdfunc);
						}
						else
						{
							//#未完成#：识别新的TBIM，并且重新开始
							st=FREE;
							break;
						}
						time_n=TIM_GetCounter(TIM5) + SEND_MSG_TIME;
						time = time_n > time_f ? time_n - time_f : 0xffff - time_f + time_n;
						if(time >= ASYN_INTERVAL)
						{
							TIM5_delay_us(SEND_MSG_TIME);//不够接收一次数据，那么等待此时间后进入异步状态
							st=SYN;
							time_f=TIM_GetCounter(TIM5);
							break;							
						}
					}
					//if(tbim_table[i].alias != 0)
				}
				break;
			}
			default:break;
		}
		//这里可以OSTimeDly(1)吗？
	}
}

/*
----------------------------------------------------------------------------------
	测试使用的变量。
----------------------------------------------------------------------------------
*/
u32t recv_num=0;//接收到的数据量
u32t syn_num=0;	//同步周期的个数
u32t asyn_num=0;//异步周期的个数
 //-----------测试用变量。--------------------
u16t test_time[1200];
static void test_analysis(u16t data[],u16t n)
{
	u16t i;
	for(i=0;i<n-1;i++)
	{
		if(data[i]-data[i+1] <0 || data[i]-data[i+1] > 200)
			printf("%d \r\n",data[i]-data[i+1]);
		else
			printf("%d ",data[i]-data[i+1]);
			
	}
	printf("\r\n");
}
//-------------------------------
static void uctsk_SSNP_TBC_new(void* pdata)
{
	struct tbc_msg *msg;
	u8t n_send_dismsg;
	void* tbim_list;
	state st;
	u8t i;
	u8t k;
	u16t delay_time[20];
	u16t time_f;
	u16t time_n;
	u32t time;

	u16t aaaa;
	u16t bbbb;
	u32t syn_total_time;
	u16t mem_recv_time[100];
	u16t index;
	u8t m;
  //-----------测试用变量。--------------------
   u8t test_alias;
   
   u16t test_time_index;
   u16t t_i; 
   u8t is_test;
  //-------------------------------

	u32t averageTime;
	u32t* data;
	struct netbuf* buf;
	syn_total_time=0; 	
	tbim_list=NULL;
	st=FREE;
	msg=NULL;
	aaaa=0;
	index=0;
  //-----------测试用变量。--------------------
   test_time_index=0;
   is_test=0;
  //-------------------------------	
	hardware_init();
	protocol_init();
	SSNP_DEBUG_PRINT("\r\n\r\n\r\nuctsk_SSNP():TBC init ok.\r\n");
	app_init();
	
	for(n_send_dismsg=0;n_send_dismsg<SEND_DISMSG_NUM;n_send_dismsg++)
	{
		send_discovery_msg();//这里能直接调用吗？因为内核和这里都使用了netif，会不会出错？但是这里应该是线程安全的，因为对于netif都是只读操作
		Delay(SEND_DISMSG_INTERVSL);
	}
	//识别TBIM
	SSNP_DEBUG_PRINT("uctsk_SSNP():get alias list.\r\n");
	tbim_list=get_tbim();//#未完成#：这里的get_tbim()函数需要修改一下
	if(tbim_list)
	{
		SSNP_DEBUG_PRINT("uctsk_SSNP():recongizing TBIM.\r\n");
		TBIM_recongnition(tbim_list);
	}
	SSNP_DEBUG_PRINT("uctsk_SSNP():recongize TBIM done.\r\n");
	for(i=0;i<MAX_TBIM_NUM;i++)
	{ 
		if(tbc.tbim_table[i].alias != 0)
		{
			//计时
			for(k=0;k<DELAY_TIME_MEASURE_NUM;k++)  //只是用来测量时延
			{
				//printf("uctsk_SSNP():TBC send reflect msg to tbim %d.\r\n",tbc.tbim_table[i].alias);
				time_f=TIM_GetCounter(TIM5);
				
				tbc_send_reflect_msg(tbc.tbim_table[i].alias);
				while(!is_reflect_reply_ok());
				
				time_n=TIM_GetCounter(TIM5);
				time = time_f > time_n ? time_f - time_n : 0xffff - time_n + time_f;
				delay_time[k]=time;	
				//printf("uctsk_SSNP():time_f=%d,time_n=%d,the delay time is %d.\r\n",time_f,time_n,time);
				set_reflect_reply_unfinished();
			}
			//#未完成#：这里应该将时间差发送回去
		//	averageTime=average_time(delay_time,DELAY_TIME_MEASURE_NUM);
			//printf("uctsk_SSNP():the average time is %d.\r\n",averageTime);
			avertime_new(tbc.tbim_table[i].uuid,delay_time,DELAY_TIME_MEASURE_NUM);
		}
	}
	printf("uctsk_SSNP():should send time slot assign msg.\r\n");

	get_epoch();
	//tbc_send_define_epoch_msg(SYN_TIMESLOT_NUM + 2,tbc.min_period*10 - SYN_TIMESLOT_NUM -2);//同步时间预留出两个时间槽
	tbc_send_define_epoch_msg(SYN_TIMESLOT_NUM + 2,100*10 - SYN_TIMESLOT_NUM -2);//同步时间预留出两个时间槽
	printf("uctsk_SSNP():send define epoch msg done.\r\n");
	timeslot_alloc();//分配时间槽,并将时间槽分配信息发送出去
	printf("uctsk_SSNP():send time slot alloc msg done.\r\n");

	time_f=TIM_GetCounter(TIM5);//计时开始
	st=SYN;
	tbc_send_begin_of_epoch_msg();//发送开始标识
	//printf("uctsk_SSNP():send begin of epoch msg done.\r\n");
	// for(t_i=0;t_i<500;t_i++)
	 //	test_time[t_i]=t_i;
	for(;;)
	{
	//	if(syn_num % 50 == 0)
	//		printf("uctsk_SSNP_TBC_new(): syn_num is %d,recv_data_num is %d,asyn_num is %d.\r\n",syn_num,recv_num,asyn_num);
	//	if(index == 100)
	//	{
	//		for(m=0;m<100;m++)
	//			printf("%d\r\n",mem_recv_time[m]);
	//		break;
	//	}
		switch(st)
		{
			case FREE:
			{
				//接收所有的sdp数据并push到上位机。
				//识别TBIM，重新分配时间槽，并发送begin epoch msg
				break;
			}
			case SYN:
			{	
				
				++syn_num;
			//	aaaa=TIM_GetCounter(TIM5) ;
				while(st == SYN)
				{
				//	aaaa=TIM_GetCounter(TIM5) ;			
					buf=(struct netbuf*)sys_arch_mbox_fetch_unblock(streaming_data_recv_mbox);
					
			    //	bbbb=TIM_GetCounter(TIM5);
					if(buf != NULL)
					{
						test_time[test_time_index++]=TIM_GetCounter(TIM5) ;
						//printf("uctsk_SSNP_TBC_new():the time is %d.\r\n", bbbb > aaaa ? bbbb - aaaa : 0xffff - aaaa + bbbb);
					
						data=(u32t*)buf->p->data;
						test_alias=buf->alias;
					//	mem_recv_time[index++]=	TIM_GetCounter(TIM5);
						//printf("uctsk_SSNP_TBC_new():TBC recv streaming data %d from TBIM %d tdcn %d\r\n",*data,buf->alias,buf->tdcn_num);
						++recv_num;
						netbuf_delete(buf);
					}
					time_n=TIM_GetCounter(TIM5) ;//预留出接收时间
					time = time_f > time_n ? time_f - time_n : 0xffff - time_n + time_f;

				//	printf("uctsk SSNP():syn time is %d.\r\n",time);
					if(time + RCV_MSG_TIME >= SYN_INTERVAL)
					{
						TIM5_delay_us(SYN_INTERVAL - time);//不够接收一次数据，那么等待此时间后进入异步状态
						st=ASYN;
						time_f=TIM_GetCounter(TIM5);
						break;
					}	
				//	bbbb=TIM_GetCounter(TIM5) ;
				//	time = bbbb > aaaa ? bbbb - aaaa : 0xfff+f - aaaa + bbbb;
				//	printf("uctsk_SSNP_TBC_new():before is %d,after is %d.the time is %d.\r\n",aaaa,bbbb,time);									
				}
				//printf("uctsk_SSNP_TBC_mew():the syn time is %d.\r\n",time);
			if(syn_num % 50 == 0 && !is_test)
			{
				printf("uctsk_SSNP_TBC_new(): syn_num is %d,recv_data_num is %d,asyn_num is %d.\r\n",syn_num,recv_num,asyn_num+1);
			 /* 	for(t_i=0;t_i<test_time_index;t_i++)
				{
					printf("%d ",test_time[t_i]);
				}
				printf("\r\nthe  test_time_index is %d.\r\n",test_time_index);
				test_analysis(test_time,test_time_index);
				is_test=1; */
				break;
			}
			}
			case ASYN:
			{
			//	aaaa=TIM_GetCounter(TIM5) ;
				++asyn_num;
				while(st == ASYN)
				{
					if((buf=(struct netbuf*)sys_arch_mbox_fetch_unblock(streaming_data_recv_mbox))!=NULL) //在异步时间段中接受剩余的同步时间数据
					{ 
					test_time[test_time_index++]=TIM_GetCounter(TIM5) ;
						++recv_num;
						netbuf_delete(buf);						
					}
					if((msg =(struct tbc_msg *)sys_arch_mbox_fetch_unblock(upcomputer_cmd_recv_mbox))!= NULL)
					{
						if(msg->type == TBC_MSG_UPPER)
						{/* 这里假设TBC命令不需要回复，否则还需要接收回复消息，#未完成#：处理命令回复数据*/
							TBC_send_noArguCmd_to_tbim(find_tbim(msg->msg.upper_msg.tbim_alias),msg->msg.upper_msg.tdcn_num,msg->msg.upper_msg.cmdclass,msg->msg.upper_msg.cmdfunc);
						}
						else
						{
							//#未完成#：识别新的TBIM，并且重新开始
							st=FREE;
							break;
						} 
					}
					time_n=TIM_GetCounter(TIM5) ;
					time = time_f > time_n ? time_f - time_n : 0xffff - time_n + time_f;
				//	printf("uctsk SSNP():asyn time is %d.\r\n",time); 
					if(time + SEND_MSG_TIME>= ASYN_INTERVAL)
					{
						TIM5_delay_us(ASYN_INTERVAL - time);//不够接收一次数据，那么等待此时间后进入异步状态
					//	printf("uctsk_SSNP_TBC_new():the wait time is %d.\r\n",ASYN_INTERVAL - time);
						st=SYN;
						time_f=TIM_GetCounter(TIM5);
						break;							
					}
				}
			//	bbbb=TIM_GetCounter(TIM5) ;
			//	time = bbbb > aaaa ? bbbb - aaaa : 0xffff - aaaa + bbbb;
			//	printf("uctsk_SSNP_TBC_new():before is %d,after is %d.the asyn time is %d.\r\n",aaaa,bbbb,time);
		//	printf("uctsk_SSNP_TBC_mew():the asyn time is %d.\r\n",time);
				break;
			}
			default:break;
		}
		//这里可以OSTimeDly(1)吗？
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
