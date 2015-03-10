#include "config.h"
#if NODE == TBC
#include "TBC.h"
#include "ssnp/alias_state.h"
#include "ssnp/mem.h"
#include "ssnp_for_app.h"
#include "debug.h"
 #include "upcomputer.h"
#define RECV_TRY_NUM              	6
#define CMD_RESPONSE_WAIT_TIME    	6
#define TBC_ADDRESS               	255

struct TBC_ tbc;
//struct tbim tbim_table[MAX_TBIM_NUM];
//static sys_mbox_t mbox;//流数据存储缓冲
//static u8t first_free;

static void Delay(u16t time)
{
	OSTimeDly(time/(1000/OS_TICKS_PER_SEC));
}
//#未完成#：这里应该判断如果conn为空和firstfree超过了正常值
/*
      这里的发送第一次使用时是没有问题的，但是当系统正常运行后再次有新的TBIM加入后，这里的
   一些初始化命令只能在异步时间段内进行传输，但是这里的发送函数没有办法保证发送这些初始化
   命令一定在异步时间段内，这里出现的问题就是同步异步状态机到底应该有协议来管理还是应该由应用
   层来管理？
       怎么感觉这个状态机应该由应用层来维护？但是如果这样做的话会出现一个问题就是：我们也采用类似
    协议内核的处理办法用一个mbox来维护所有需要处理的信息,这样同步(对于TBC来说是接收数据)
     和异步信息(对于TBC来说是上位机发送的命令)以及比如新加入的TBIM信息会放在同一个mbox中，这样就
    没有办法保证比如我们在异步时间段内，时间段内，时间段内.......好像没有什么问题。

       这样做是可以的，把状态机在应用层维护，这样在同步时间段内查询所有的网络TP连接看是否有数据，同时
    由于TBIM不会主动向TBC发送命令也就是dp数据和trigger，这样就简单了，在异步区间TBC就检测是否有
    上位机传送的命令，同时检测是否有新加入的TBIM就可以了。

        这里的问题就是时间精确度的问题了，根据讨论同步时间段为5.12ms,但是这里可以精确的进行计时吗？
    这里的精确可能做不到，但是在哪个区间是一定可以精确判定的，根据这里timer的实现方法，我们不太可能
    精确计时到0.01ms，但是在哪个时间段是一定可以准确判断的，这样就可以了。
        这里还有一个问题，比如TBC在处理流数据时有可能会超出同步时间段，但是对于TBC来说流数据只是用于接收
    超出时间段也没有什么问题。但是对于异步时间段 的发送上位机命令如果超过了异步时间段应该怎么处理？也
    就是说开始时间不一定要多精确只要能保证在这个时间段内就可以了，但是对于截止时间来说一定要精确的
		在相应的时间段内停止而不可以和下一个时间段冲突，如何精确的截止这个问题应该如何处理？一种方法是我们
		可以提前估计一下发送此类数据需要多长时间，如果超过则在下一时间段不发送，否则发送，这样也是可以的。
		
		  对于时间段的维护我们可以自己来管理，使用OSTimeGet()方法每次获取时间，但是每个ticket的时间长度是10ms
		而这里的同步时间段只有5ms,因此不可以使用这个函数来完成时间管理。
		  如果有精确计时方法,我们先不要管TBIM这一端如何准确和TBC同步，我们首先解决TBC上精确计时的问题。假设我们
		找的了精确计时的办法，那么我们在得到TBIM信息并且分配完时间片后
		//微秒延迟 http://blog.csdn.net/liuyu60305002/article/details/6942722
		
		   我们也可以这样设计就是TBC在发出begin epoch后TBC和TBIM之间商定等待一个时间段T后开始，在TBIM接收到后在T时间段
		内可以有足够的时间用来设置中断，这样就可以对流数据的发送进行一次时间设置，当再次接收到begin epoch后重新建立
		中断就可以了。
		   http://zhidao.baidu.com/link?url=uPFWjvuVr_GkMgRGRhJlXS1CnJaSnU-f7awBz55LfYZYGSDeElzX1eXDGuIImbsPNwKgofO9cuuj4zWg54qRaa
		可以使用溢出中断的方式。
		
		   测量了一下应用程序从开始调用发送函数到数据完全发出需要92us，发送trigger数据需要42us。以100us计算那么每个时间槽
		应该有100us，512个传感器需要51200us=51.2ms，每个epoch为100ms，这样是比较合理的。
		   有了这样的精确时间计算函数，TBC状态机的维护就可以完全在应用程序中实现了：初始状态是空闲状态在发送epoch后等待一段时间
		，这一段时间假设定为200us，这样定义是保证所有的TBIM的上层应用得到此消息，TBC进入同步时间段状态，在这个状态下只去接收数据然后
		查询是不是超过了51200us，如果超过则进入异步状态，查询mbox发送命令，并在每次发送命令之前计算如果发送此命令会不会和同步时间段冲突
		这个时间还要考虑到TBIM接受并回复的时间，如果可以发送那么发送就可以了，然后看时间是不是进入了同步状态。这样这个状态机的维护就可以
		在TBC的应用程序中实现了。
		
*/

/*
---------------------------------------------------------------------------------------------
         注意TBC这里一定要都是helper函数，不要处理涉及到具体应用程序的任务。
         应用程序需要做的就是接收上位机的命令，然后发送命令，接收流数据，计算时间表，就可以了。
     还有就是TBC的数据结构设计好：这里有哪些数据是TBC需要知道但是应用程序不需要知道的？
   

---------------------------------------------------------------------------------------------
*/
void TBC_init(sys_mbox_t sdr_mbox,sys_mbox_t crr_mbox)
{
	tbc.streaming_data_recv_mbox=sdr_mbox;	
	tbc.cmd_reply_recv_mbox=crr_mbox;
	tbc.time_slot_seq=0;
	tbc.asyn_num=0;
	tbc.syn_num=0;
}
void set_syn_asyn_num(u16t syn,u16t asyn)
{
	tbc.syn_num=syn;
	tbc.asyn_num=asyn;
}
struct tbim* find_tbim(u8t alias)
{
	u8t i;
	for(i=0;i<MAX_TBIM_NUM;i++)
		if(tbc.tbim_table[i].alias == alias)
			return &tbc.tbim_table[i];
	return NULL;
}
static err_t send_cmd_to_tbim(struct tbim* t,u8t tdcn_num,void* data,u16t data_len)
{
	struct netbuf buf;
	err_t err;


	buf.alias=t->alias;//对方的别名
	buf.tdcn_num=tdcn_num;  
	
	buf.tbim_tdcn_num=0;
	buf.p=NULL;	//这里一定要注意将p设置为空
	
	netbuf_ref(&buf,data,data_len);
	err=netconn_sendto(t->conn_dp,&buf,buf.alias,buf.tdcn_num);
	netbuf_free(&buf);
	
	return err;
}
static err_t send_noArguCmd_to_tbim(struct tbim* t,u8t tdcn_num,u8t cmdClass, u8t cmdFunc)
{
	struct netbuf buf;
	err_t err;
	u8t data[2];
	
	data[0]=cmdClass;
	data[1]=cmdFunc;

	buf.alias=t->alias;//对方的别名
	buf.tdcn_num=tdcn_num;  
	
	buf.tbim_tdcn_num=0;
	buf.p=NULL;	//这里一定要注意将p设置为空
	
	netbuf_ref(&buf,(void*)data,2);
	err=netconn_sendto(t->conn_dp,&buf,buf.alias,buf.tdcn_num);
//	printf("send_noArguCmd_to_tbim():send cmdclass %d cmdfunc %d to tbim %d.\r\n",cmdClass,cmdFunc,t->alias);
	netbuf_free(&buf);
	
	return err;
}
/*
  TBC向TBIM发送数据本身不需要指定TBC自己的
*/
err_t TBC_send_noArguCmd_to_tbim(struct tbim* t,u8t dst_tdcn_num,u8t cmdClass, u8t cmdFunc)
{
	if(t == NULL)
		return ERR_ARGUMENT;
	
	return send_noArguCmd_to_tbim(t,dst_tdcn_num,cmdClass,cmdFunc);
}

err_t TBC_send_cmd_to_tbim(struct tbim* t,u8t dst_tdcn_num,void* cmd_data,u16t data_len)
{
	if(t == NULL)
		return ERR_ARGUMENT;
	
	return send_cmd_to_tbim(t,dst_tdcn_num,cmd_data,data_len);
}

err_t TBC_send_noArgucmd(u8t alias,u8t tbim_tdcn_num,u8t cmdclass,u8t cmdfunc)
{
	return TBC_send_noArguCmd_to_tbim(find_tbim(alias),tbim_tdcn_num,cmdclass,cmdfunc);
}

err_t TBC_send_cmd(u8t alias,u8t tbim_tdcn_num,void* cmd_data,u16t data_len)
{
	return TBC_send_cmd_to_tbim(find_tbim(alias),tbim_tdcn_num,cmd_data,data_len);
}
err_t TBC_send_trigger_to_tbim(struct tbim* t,u8t tbim_tdcn_num)
{
	if(t == NULL)
		return ERR_ARGUMENT;
	
	return netconn_sendtrigger(t->conn_tp,tbim_tdcn_num,TBC_ADDRESS,0);
	
}
err_t TBC_send_trigger(u8t alias,u8t tbim_tdcn_num)
{
	return TBC_send_trigger_to_tbim(find_tbim(alias),tbim_tdcn_num);
}
err_t TBC_send_upcomputer_cmd(struct cmd_item item)
{
	return TBC_send_noArgucmd(item.alias,item.tdcn,item.cmd_class,item.cmd_func);
}
//err_t TBC_recv_streaming_data_from_tbim_unblock(struct tbim* t,u8t *tbim_tdcn_num,struct pbuf** buf)
//{
//	struct netbuf* nbuf;
//	err_t err;
//	if(t == NULL || buf == NULL || tbim_tdcn_num == NULL)
//		return ERR_ARGUMENT;
//	
//	err=netconn_recv_unblock(t->conn_sdp,&nbuf);
//	if(err != ERR_OK)
//		return ERR_VALUE;
//	
//	*buf=nbuf->p;
//	*tbim_tdcn_num=nbuf->tbim_tdcn_num;
//}
static err_t get_TBIM_struct(struct tbim *t)
{
	u8t n;
	u8t* data;
	u8t isOk;
	struct netbuf* buf;
	err_t err;
	
	isOk=0;
//	SSNP_DEBUG_PRINT("get_TBIM_struct():sending tbim struct msg .\r\n");
	if(TBC_send_noArguCmd_to_tbim(t,0,INITIALIZATION,READ_TBIM_STRUCTURE)!=ERR_OK)//这里的0代表的是TBIM本身，不是指TBC本身
	{
		SSNP_DEBUG_PRINT("get_TBIM_struct():send get tbim struct msg failed.\r\n");
		return ERR_VALUE;
	}
//	SSNP_DEBUG_PRINT("get_TBIM_struct():send tbim struct msg done.\r\n");
	for(n =0;n<RECV_TRY_NUM;n++)
	{//	printf("get_TBIM_struct():try %d.\r\n",n);
		if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
		{//	SSNP_DEBUG_PRINT("get_TBIM_struct():get tbim struct reply msg.\r\n");
			data=(u8t*)buf->p->data;//我们只需要知道一个TBIM中传感器的数量,因为后面都是连续的，
			t->sensor_num=(*(data+3));//跳过命令回复协议数据头部信息
		//	printf("%d %d %d %d %d %d %d.\r\n",*(data-2),*(data-1),*data,*(data+1),*(data+2),*(data+3),*(data+4));
		//	printf("get_TBIM_struct():the TBIM %d has %d sensors.\r\n",t->alias,t->sensor_num);
			isOk=1;
			netbuf_delete(buf);
			break;
		}
		Delay(CMD_RESPONSE_WAIT_TIME);
	}
	if(!isOk)
		return ERR_VALUE;	
	else
		return ERR_OK;
}
static err_t get_TBIM_meta_TEDS(struct tbim *t)
{
	u8t n;
	u8t* data;
	u8t isOk;
	struct netbuf* buf;
	err_t err;
	u8t* iset;
	u32t* offset;
	u16t* max_block_size;
	u8t read_teds_cmd[2+6];//报文头2(TBC这一端没有status octet位)+参数大小6
	
	
	isOk=0;
	if(TBC_send_noArguCmd_to_tbim(t,0,QUERY_REDS,MODULE_META_TEDS)!=ERR_OK)//这里的0代表的是TBIM本身，不是指TBC本身
		return ERR_VALUE;
	
	for(n =0;n<RECV_TRY_NUM;n++)
	{
		if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
		{
			//#未完成#：这里不做处理，直接认为meta_teds是存在的。
			isOk=1;
			netbuf_delete(buf);
			break;
		}
		Delay(CMD_RESPONSE_WAIT_TIME);
	}
	if(!isOk)
		return ERR_VALUE;	
	
	iset=(u8t*)read_teds_cmd;
	*iset=READ_TEDS_BLOCK;
	*(++iset)=MODULE_META_TEDS;
	offset=(u32t*)(++iset);
	*offset=35;//meta_teds大小定位35字节
	max_block_size=(u16t*)(iset+4);
	*max_block_size=35;
//	SSNP_DEBUG_PRINT("get_TBIM_meta_TEDS():sending read meta teds cmd.\r\n");
	if(TBC_send_cmd_to_tbim(t,0,read_teds_cmd,8)!=ERR_OK)//这里的0代表的是TBIM本身，不是指TBC本身
		return ERR_VALUE;
	
	isOk=0;
	for(n =0;n<RECV_TRY_NUM;n++)
	{
		if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
		{
			//#未完成#：这里不做处理，接收到meta_teds后直接删掉了。
			isOk=1;
			netbuf_delete(buf);
			break;
		}
		Delay(CMD_RESPONSE_WAIT_TIME);
	}
	if(!isOk)
		return ERR_VALUE;	
	else
		return ERR_OK;
}
static err_t get_TBIM_tdcn_TEDS(struct tbim *t)
{
	u8t n;
	u8t i;
	struct netbuf* buf;
	u8t isOk;
	u8t* iset;
	u32t* offset;
	u16t* max_block_size;
	u8t read_teds_cmd[2+6+1];//报文头2(TBC这一端没有status octet位)+参数大小7，其中包括向哪一个变送器通道号寻址
	
	
	read_teds_cmd[0]=READ_TEDS_BLOCK;
	read_teds_cmd[1]=TDCN_TEDS;
	iset=(u8t*)read_teds_cmd;
	offset=(u32t*)(iset+2);
	*offset=118;
	max_block_size=(u16t*)(iset+4);
	*max_block_size=118;
	iset+=8;//指向变送器通道号设置位
	
	
	for(n=1;n<=t->sensor_num;n++)
	{
//		printf("get_TBIM_tdcn_TEDS():TBC sending query tdcn teds to TBIM %d,tdcn %d.\r\n",t->alias,n);
		if(TBC_send_noArguCmd_to_tbim(t,n,QUERY_REDS,TDCN_TEDS)!=ERR_OK)
			return ERR_VALUE;
		
		isOk=0;
		for(i=0;i<RECV_TRY_NUM;i++)
		{
			if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
			{
				//#未完成#：这里不做处理，直接认为tdcn_teds是存在的。
				isOk=1;
				netbuf_delete(buf);
				break;
			}
			Delay(CMD_RESPONSE_WAIT_TIME);	
		}
		if(!isOk)
			return ERR_VALUE;	
		
		*iset=n;//写入需要读取哪个变送器通道的tdcn_teds
		if(TBC_send_cmd_to_tbim(t,0,read_teds_cmd,9)!=ERR_OK)
			return ERR_VALUE;
		
		isOk=0;
		for(i=0;i<RECV_TRY_NUM;i++)
		{
			if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
			{
				//#未完成#:这里不做处理，接收到tdcn_teds后直接删掉了。
				iset=(u8t*)buf->p->data;
				//send_TEDS_upcomputer(&iset[2],t->alias);
				printf("@%d",t->alias);
				for(isOk=0;isOk<32;isOk++)
				{
					printf("%d",iset[isOk+2]);	
				}
				printf("@");
				isOk=1;
				netbuf_delete(buf);
				break;
			}
			Delay(CMD_RESPONSE_WAIT_TIME);	
		}
		if(!isOk)
			return ERR_VALUE;		
	}
	return ERR_OK;
}

static err_t create_TBIM(u8t uid,u8t alias)
{	
	struct tbim *t;
	struct netbuf* buf;
	struct netconn* con;
	struct netconn* con1;
	struct netconn* con2;
	err_t err;
	u8t n;
	u8t* data;
	u8t isOk;
	printf("create_TBIM():create tbim %d.\r\n",alias);
	isOk=0;
//	printf("create_TBIM():first postion is %d.\r\n",tbc.first_free);
	t=&tbc.tbim_table[tbc.first_free++];
	
	t->alias=alias;
	t->uuid=uid;
	


	t->conn_dp=netconn_new(NETCONN_DP);
	set_connection_recvmbox(t->conn_dp,tbc.cmd_reply_recv_mbox);
	if(t->conn_dp == NULL)
	{
	//	SSNP_DEBUG_PRINT("create_TBIM():dp conn is NULL.\r\n");
		return ERR_VALUE;
	}
	netconn_bind(t->conn_dp,TBC_ADDRESS,0);//TBC綁定自己的地址
	netconn_connect(t->conn_dp,alias,0);//連接到對方
//	SSNP_DEBUG_PRINT("create_TBIM():dp conn done.\r\n");




	t->conn_tp=netconn_new(NETCONN_TP);
	if(t->conn_tp == NULL)
	{
		SSNP_DEBUG_PRINT("create_TBIM():tdp conn is NULL.\r\n");
		return ERR_VALUE;
	}
	netconn_bind(t->conn_tp,TBC_ADDRESS,0);
	netconn_connect(t->conn_tp,alias,0);
//	SSNP_DEBUG_PRINT("create_TBIM():tp conn done.\r\n");


	t->conn_sdp=netconn_new(NETCONN_SDP);
	set_connection_recvmbox(t->conn_sdp,tbc.streaming_data_recv_mbox);//将mbox挂接到所有sdp连接的接收缓冲
	if(t->conn_sdp == NULL)
	{
		SSNP_DEBUG_PRINT("create_TBIM():sdp conn is NULL.\r\n");
		return ERR_VALUE;
	}
	netconn_bind(t->conn_sdp,TBC_ADDRESS,0);
	netconn_connect(t->conn_sdp,alias,0);
//	SSNP_DEBUG_PRINT("create_TBIM():sdp conn done.\r\n");



	
	t->valid=0;
//	SSNP_DEBUG_PRINT("create_TBIM():TBC getting TBIM struct.\r\n");
	if(get_TBIM_struct(t) != ERR_OK)
	{
		SSNP_DEBUG_PRINT("create_TBIM():TBC get TBIM struct failed.\r\n");
		return ERR_VALUE;
	}
	SSNP_DEBUG_PRINT("create_TBIM():TBC get TBIM struct OK.\r\n");
//	SSNP_DEBUG_PRINT("create_TBIM():TBC getting TBIM meta TEDS.\r\n");
	if(get_TBIM_meta_TEDS(t)!=ERR_OK)
		return ERR_VALUE;
	SSNP_DEBUG_PRINT("create_TBIM():TBC get TBIM meta TEDS OK.\r\n");
//	SSNP_DEBUG_PRINT("create_TBIM():TBC getting TBIM tdcn TEDS.\r\n");
	if(get_TBIM_tdcn_TEDS(t)!=ERR_OK)
		return ERR_VALUE;
	SSNP_DEBUG_PRINT("create_TBIM():TBC get TBIM tdcn TEDS OK.\r\n");
		
	t->valid=1;
	return ERR_OK;
}

/*************************************************************************************
TBIM_recongnition:完成的任务是：

 	  根据别名建立网络连接描述
		发送Read TBIM structure命令
		获取TBIM的meta_TEDS
		获取TBIM各个变送器通道TEDS
		解析TBIM中TEDS信息
 *************************************************************************************/ 
void TBIM_recongnition(void* alias_list)
{
	struct alias_state* p;
//	SSNP_DEBUG_PRINT("TBIM_recongnition():TBC getting TBIM meta TEDS OK.\r\n");
//	for(p=(struct alias_state*)alias_list;p;p=p->next)
	//	printf("TBIM_recongnition(): the list alias %d.\r\n",p->alias);

	for(p=(struct alias_state*)alias_list;p;p=p->next)
		create_TBIM(p->uid.uid[9],p->alias);
	for(p=(struct alias_state*)alias_list;p;p=p->next)
		mem_free((void*)p);
}
static void send_timeslot_msg()
{
	//使用tbc_send_timeslot_msg()函数发送时间槽分配msg
	u8t i;
	u16t timeslot;
	
	timeslot=1;
	for(i=0;i<MAX_TBIM_NUM;i++)
	{
		if(tbc.tbim_table[i].valid)
		{
			tbc_send_timeslot_msg(tbc.tbim_table[i].alias,0,timeslot,1);
			timeslot+=tbc.tbim_table[i].sensor_num;
		}
	}
}
void timeslot_alloc()
{
	//计算时间槽.
	send_timeslot_msg();
}


void get_epoch(void)
{
	u8t i;
	u8t j;
	u16t min=0xffff;
	u16t max=0;
	for(i=0;i<MAX_TBIM_NUM;i++)
	{
		if(tbc.tbim_table[i].valid)
		{
			for(j=0;j<tbc.tbim_table[i].sensor_num;j++)
			{
				if(tbc.tbim_table[i].sensor_table[j].nPeriod < min)
					min=tbc.tbim_table[i].sensor_table[j].nPeriod;
				if(tbc.tbim_table[i].sensor_table[j].nPeriod > max)
					max=tbc.tbim_table[i].sensor_table[j].nPeriod;
			}
		}
	}
	if(min!=0xffff && max!=0)
	{
		tbc.max_period=max;
		tbc.min_period=min;
		printf("get_epoch():the max period is %d,the min period is %d.\r\n",max,min);
	}
}

void send_schedul_table(struct schedultable_item* table,u16t table_len)
{
	u16t i;
	for(i=0;i<table_len;i++)
		tbc_send_timeslot_msg(table[i].alias,table[i].tdcn,table[i].begin_time_slot,table[i].num);
}
#endif