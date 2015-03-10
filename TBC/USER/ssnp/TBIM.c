#include "TBIM.h"
#include "TEDS.h"
#include "api.h"
#include "uctsk_SSNP_TBIM.h"
#include "TBIMDriver.h"
#include "mem.h"
#include "debug.h"
#include "sdp.h"

#define TBC_ADDRESS 0xff
#define UUIDLEN 10

#define TDCN_NUM   16
#define MAX_TBIM_ADDR 16
/*********************************************************************************
 *                                 TBIM初始化相关函数
 *********************************************************************************/
enum STATE
{
	HALTED,
	OPERATING,
	SLEEP
};
struct channelRegistItem
{
	u8t nChnNum;//通道号，通道在TBIM内的编号，也就是注册表中的下标，这里实际上是没有必要的，因为直接使用下标就可以了
	u8t nAddr;//在TBIM内的地址
	u8t nChnSn;//此地址的偏移地址
	void* tedsBuf;
	union
	{
		int d;
		u8t databuf[4];
	}data;
	u16t nPeriod;//采用周期
	u8t state;//状态
};

struct TBIM_
{
	u8t alias;
	
	enum INIT_STATE init_st;//TBIM所处的初始化状态
	enum STATE current_state;//TBIM所处的工作状态
	
	struct netconn* conn_dp;
	struct netconn* conn_sdp;
	struct netconn* conn_tp;
	
	u8t Chn_num;
	struct channelRegistItem chnReg[TDCN_NUM];//注册表,这里有一个问题就是应该让应用层使用这个注册表吗？这个注册表应用程序是没有必要知道的
};
static struct TBIM_ tbim;


static void insertChannel(u8t index,u8t addr,u8t nChnSn,struct TransducerChannel_TEDS* teds_buf)
{
	tbim.chnReg[index].nAddr=addr;
	tbim.chnReg[index].nChnNum=nChnSn;
	tbim.chnReg[index].tedsBuf=teds_buf;

	//#未完成#:这里根据teds得到周期
}
void TBIM_init(u8t alias)
{
	u8t addr;
	u8t nChnCnt;
	u8t nChnSn;
	u8t index=0;
	struct TransducerChannel_TEDS* teds_buf; 
	
	for(addr=0;addr<MAX_TBIM_ADDR;addr++)
	{
		nChnCnt=findTdcn(addr);
		if(nChnCnt != (u8t)-1)
		{
			for(nChnSn=0;nChnSn<nChnCnt;nChnSn++)
			{
				getTedsFromTdcn(teds_buf,addr,nChnSn);
				insertChannel(index++,addr,nChnSn,teds_buf);
			}
		}
		else
			break;
	}
	
	tbim.alias=alias;
	tbim.Chn_num=index;
	
	tbim.conn_dp=netconn_new(NETCONN_DP);
	if(tbim.conn_dp == NULL)
		printf("TBIM_init():ERROR,conn_dp is NULL.\r\n");
	netconn_bind(tbim.conn_dp,alias,0);
	netconn_connect(tbim.conn_dp,TBC_ADDRESS,0);//TBC的别名总是255 
	
	tbim.conn_sdp=netconn_new(NETCONN_SDP);
	if(tbim.conn_sdp == NULL)
		printf("TBIM_init():ERROR,conn_sdp is NULL.\r\n");
	netconn_bind(tbim.conn_sdp,alias,0);
	netconn_connect(tbim.conn_sdp,TBC_ADDRESS,0);//TBC的别名总是255
		
	tbim.conn_tp=netconn_new(NETCONN_TP);
	if(tbim.conn_tp == NULL)
		printf("TBIM_init():ERROR,conn_tp is NULL.\r\n");
	netconn_bind(tbim.conn_tp,alias,0);
	netconn_connect(tbim.conn_tp,TBC_ADDRESS,0);//TBC的别名总是255
				
	tbim.init_st=READ_TBIM_STRUCT;
	tbim.current_state=HALTED;
}

static err_t TBIM_send_cmd_reply_msg(u8t tbim_tdcn_num,void* data,u16t data_len)
{
	struct netbuf buf;
	err_t err;
	
	buf.alias=TBC_ADDRESS;
	buf.tdcn_num=0;
	buf.tbim_tdcn_num=tbim_tdcn_num;
	buf.p=NULL;
	
	netbuf_ref(&buf,(void*)data,data_len);
	err=netconn_sendto(tbim.conn_dp,&buf,buf.alias,buf.tdcn_num);
	netbuf_free(&buf);
	
	return err;
}
enum INIT_STATE get_TBIM_init_state(void)
{
	return tbim.init_st;
}

u16t get_sensor_period(u8t tdcn_num)
{
	return tbim.chnReg[tdcn_num-1].nPeriod;
}
u8t get_TBIM_Chn_num(void)
{
	return tbim.Chn_num;
}

void* get_sensor_data(u8t tdcn_num)
{
	void* data;
	
	data=NULL;
	if(getDataFromChn(&data,tbim.chnReg[tdcn_num].nAddr,tbim.chnReg[tdcn_num].nChnSn) != -1)
	{
		return data;
	}
	else
	{
		return NULL;
	}
}
u16t max_period(void)
{
	u16 max;
	u8t i;
	
	max=0;
	for(i=0;i<tbim.Chn_num;i++)
		if(tbim.chnReg[i].nPeriod > max)
			max=tbim.chnReg[i].nPeriod;
	
	return max;
}
err_t TBIM_send_streaming_data(u8t tbim_tdcn_num,void* data)
{
	struct netbuf buf;
	err_t err;
	
	buf.alias=TBC_ADDRESS;
	buf.tdcn_num=0;
	buf.tbim_tdcn_num=tbim_tdcn_num;//向TBC发送流数据，那么TBC的变送器通道号一律为0
	
	buf.p=NULL;	//这里一定要注意将p设置为空
	netbuf_ref(&buf,data,4);//数据长度一律是4字节
//	err=netconn_sendto(tbim.conn_sdp,&buf,buf.alias,buf.tdcn_num);//注意这里使用sdp进行流数据的发送
	tbim.conn_sdp->pcb.sdp->local_tdcn_num=tbim_tdcn_num;
	
	if((err=sdp_sendto(tbim.conn_sdp->pcb.sdp,buf.p,buf.alias,buf.tdcn_num)) != ERR_OK)
		printf("TBIM_send_streaming_data():send data failed.\r\n");
//	printf("TBIM_send_streaming_data():tbim send data to %d.\r\n",buf.alias);
	netbuf_free(&buf);//这里一定要注意释放内存，否则内存泄露
	
	return err;
}
//注意：这里的所有函数的前提是所有的命令和trigger都是由TBC发送的，因此回复也是向TBC发送
err_t TBIM_recv_cmd_and_execute(void)
{
	struct netbuf* buf;
	void* data;
	u16t data_len;
	u8t* dataptr;
	u8t cmdclass;
	u8t cmdfunc;
	void* argument;
	err_t err;
	
	buf=NULL;
	err=netconn_recv_unblock(tbim.conn_dp,&buf);
	
	if(err != ERR_OK)
		return err;
	
	netbuf_data(buf,&data,&data_len);
	
	dataptr=(u8t*)data;
	cmdclass=*dataptr;
	cmdfunc=*(++dataptr);
	argument=(void*)(++dataptr);
	printf("TBIM_recv_cmd_and_execute():executing cmdclass=%d,cmdfunc=%d.\r\n",cmdclass,cmdfunc);
	execute(cmdclass,cmdfunc,buf->tbim_tdcn_num, argument);
	
	netbuf_delete(buf);//底层发送上来的数据一定要delete掉
	
	return ERR_OK;
}

err_t TBIM_recv_trigger_and_execute(void)
{
	err_t err;
	u8t alias;
	u8t tdcn_num;
	
	err=netconn_recvtrigger_unblock(tbim.conn_tp,&alias,&tdcn_num);
	
	if(err == ERR_OK)
		trigger_do();
	
	return err;
}


/*********************************************************************************
 *                                 uuid相关函数
 *********************************************************************************/
static struct uuid uid={0,0,0,0,0,0,0,0,0,2}; 
struct uuid* get_uuid()
{
	return &uid;
}
u8t is_uuid_eq(struct uuid* id)
{
	u8t i;
	for(i=0;i<UUIDLEN;i++)
	{
		if(id->uid[i] != uid.uid[i])
			return 0;
	}
	return 1;
}


/**********************************************************************************
 *    这里没有必要需要底层协议知道上层TEDS的存在，下层协议只需要和TBIM交互就可以了
 **********************************************************************************/
u8t get_asy_flag() 
{
	return phy_teds_asy_flag();
}
u8t get_payload_encoding()
{
	return phy_teds_payload_encoding();
}
f32 get_start_delay()
{
	return phy_teds_start_delay();
}
f32 get_reflect_delay()
{
	return phy_teds_reflect_delay();
}
f32 get_reflect_delay_uncertainty()
{
	return phy_teds_reflect_delay_uncertainty();
}

/**********************************************************************************
 *
 *
 *
 *                               命令执行相关函数
 *
 *
 *
 *
 **********************************************************************************/
#define INITCOMMNUM 17
#define OPCOMMNUM   14
#define OPMODENUM   6

#define ACCODE_METATEDS 2
#define ACCODE_TCTEDS   3

//内部所有的变送器发送数据相关的操作都通过这两个函数进行
static void send_cmd_reply(void* data,void*argument)
{
	
}
static void sent_trigger_reply(void* argument)
{
	
}

typedef void (*function)(u8t tdcn_num,void *argument);
enum
{
    INITIALIZATION=1,
    OPERATIONAL,
    QUERYTEDS,
    READTEDSBLCOK,
    WRITETEDSBLCOK,
    UPDATETEDS,
    SETOPERATINGMODE,
    READOPERATINGMODE,
    RUNDIAGONSITICS
};

enum INITIALIZATION_COMMAND
{
	SLEEP_=1,
	WAKEUP,
	SET_TDCN_DATA_REPETITION_COUNT,
	READ_TDCN_DATA_REPETITION_COUNT,
	SET_TDCN_PRETIRGGER_COUNT,
	READ_TDCN_PRETRIGGER_COUNT,
	CALIBRATE_TDCN,
	ZERO_TDCN,
	ENABLE_CORRECTIONS,
	DISABLE_CORRECTIONS,
	ENABLE_TDCN,
	ADDRESSGROUP_DEFINITION,
	READ_ADDRESSGROUP_ASSIGNMENT,
	ERASE_OPERATIONAL_SETUP,
	STORE_OPERATIONAL_SETUP,
	READ_TBIM_STRUCTURE
};
enum OPERATIONAL_COMMAND
{
	ENABLE_TDCN_TRIGGER=1,
	DISABLE_TDCN_TRIGGER,
	WRITE_SERVICE_REQUEST_MASK,
	READ_SERVICE_REQUEST_MASK,
	READ_STATUS,
	QUERY_DATA_BLOCK,
	READ_TDCN_DATA,
	READ_TDCN_DATA_BLOCK,
	WRITE_TDCN_DATA,
	WRITE_TDCN_DATA_BLOCK,
	READ_TBIM_VERSION,
	RESETS,
	HALT
};
enum TEDS_ACCESS_CODE
{
	MODULE_META_TEDS=1,
	META_ID_TEDS,
	TDCN_TEDS,
	TDCN_ID_TEDS,
	CALIBRATION_TEDS,
	CALIBRATION_ID_TEDS,
	END_USER_APP_SEPCIFIC_TEDS,
	FREQUENCY_RESPONSE_TEDS,
	TRANSFER_FUNCTION_TEDS,
	CMD_TEDS,
	LOCATION_AND_TITLE_TEDS,
	COMMISSINOING_TEDS,
	PHY_TEDS
};
static void sleep(u8t tdcn_num,void *argument)
{
}
static void wakeup(u8t tdcn_num,void *argument)
{
/*	struct TBIM_arg* arg;
	struct netbuf buf;
	u8t data[12]="TBIM wake up";
	
	printf("TBIM(): TBIM wake up.\r\n");
	
	arg=(struct TBIM_arg*)argument;


	buf.tbim_tdcn_num=arg->tbim_tdcn_num;
	buf.alias=arg->alias;
	buf.tdcn_num=arg->tdcn_num;
	buf.p=NULL;	//这里一定要注意将p设置为空
	netbuf_ref(&buf,(void*)data,12);
	printf("TBIM(): sending msg.\r\n");
	
	netconn_sendto(arg->conn,&buf,buf.alias,buf.tdcn_num);
	
	printf("TBIM(): sent msg done ok .\r\n");
	netbuf_free(&buf);//这里一定要注意释放内存，否则内存泄露
	*/
}
static void setTCdataRepCount(u8t tdcn_num,void *argument)
{
}
static void readTCdataRepCount(u8t tdcn_num,void *argument)
{
}
static void setTCpretriggerCount(u8t tdcn_num,void *argument)
{
}
static void readTCpretriggerCount(u8t tdcn_num,void *argument)
{
}
static void calibrateTC(u8t tdcn_num,void *argument)
{
}
static void zeroTC(u8t tdcn_num,void *argument)
{
}
static void enableCorrections(u8t tdcn_num,void *argument)
{
}
static void disableCorrections(u8t tdcn_num,void *argument)
{
}
static void enableTC(u8t tdcn_num,void *argument)
{
}
static void addressGroupDef(u8t tdcn_num,void *argument)
{
}
static void readAddressGroupAssignment(u8t tdcn_num,void *argument)
{
}
static void eraseOperationSetup(u8t tdcn_num,void *argument)
{
}
static void storeOperationSetup(u8t tdcn_num,void *argument)
{
}
static void readTBIMstruct(u8t tdcn_num,void *argument)
{
	u8t chnNum;
	u8t* reply_msg;
	u8t* iset;
	u8t i;
	u8t data_len;
	err_t err;
	SSNP_DEBUG_PRINT("readTBIMstruct():sending TBIM struct.\r\n");
	reply_msg=NULL;
	chnNum=tbim.Chn_num;
	chnNum=1;//TEST:这里只是用来验证
	data_len=3+chnNum+1;//3字节的头部信息和协议数据
	reply_msg=(u8t*)mem_alloc(data_len);
	iset=reply_msg;
	if(reply_msg == NULL)
		return ;
	*iset=INITIALIZATION;
	*(++iset)=READ_TBIM_STRUCTURE;
	*(++iset)=0;//#未完成#：这里将status octet都设置为0
	*(++iset)=chnNum;
	for(i=1;i<=chnNum;i++)//注意变送器通道号最小从1开始，0是TBIM
		*(++iset)=i;
	
	err=TBIM_send_cmd_reply_msg(0,reply_msg,data_len);
	mem_free((void*)reply_msg);
	tbim.init_st=READ_META_TEDS;//等待TBC读取meta_teds
	SSNP_DEBUG_PRINT("readTBIMstruct():send TBIM struct OK.\r\n");
}

function initialization[INITCOMMNUM]={NULL,sleep,wakeup,setTCdataRepCount,readTCdataRepCount,setTCpretriggerCount,
                            readTCpretriggerCount,calibrateTC,zeroTC,enableCorrections,disableCorrections,enableTC,
                            addressGroupDef,readAddressGroupAssignment,eraseOperationSetup,storeOperationSetup,readTBIMstruct};


static void enableTCtrigger(u8t tdcn_num,void *argument)
{
}
static void disableTCtrigger(u8t tdcn_num,void *argument)
{
}
static void writeServiceReqMask(u8t tdcn_num,void *argument)
{
}
static void readServiceReqMask(u8t tdcn_num,void *argument)
{
}
static void readStatus(u8t tdcn_num,void *argument)
{
}
static void queryDataBlock(u8t tdcn_num,void *argument)
{
}
static void readTCdata(u8t tdcn_num,void *argument)
{
}
static void readTCdataBlock(u8t tdcn_num,void *argument)
{
}
static void writeTCdata(u8t tdcn_num,void *argument)
{
}
static void writeTCdataBlock(u8t tdcn_num,void *argument)
{
}
static void readTBIMversion(u8t tdcn_num,void *argument)
{
}
static void reset(u8t tdcn_num,void *argument)
{
}
static void halt(u8t tdcn_num,void *argument)
{
}
function op[OPCOMMNUM]={NULL,enableTCtrigger,disableTCtrigger,writeServiceReqMask,readServiceReqMask,readStatus,queryDataBlock,
                       readTCdata,readTCdataBlock,writeTCdata,writeTCdataBlock,readTBIMversion,reset,halt};


static void queryTEDS(u8t cmdFunc, u8t tdcn_num,void *argument)
{
	u8t *iset;
	err_t err;
	u32t* attribute;
	u16t* maximum_block_size;
	u8t query_TEDS_response[3+1+4+4+2];
	
	switch(cmdFunc)
	{
		case MODULE_META_TEDS:
		{
			SSNP_DEBUG_PRINT("queryTEDS():querying meta teds.\r\n");
			iset=(u8t*)query_TEDS_response;
			*iset=QUERYTEDS;
			*(++iset)=MODULE_META_TEDS;
			*(++iset)=0;//status bit
			*(++iset)=0;//TEDS attribure;
			attribute=(u32t*)iset;
			*attribute=35;//meta_teds大小
			*(++attribute)=35;
			maximum_block_size=(u16t*)(iset+8);
			*maximum_block_size=35;
			
			err=TBIM_send_cmd_reply_msg(0,query_TEDS_response,14);
			SSNP_DEBUG_PRINT("queryTEDS():querying meta teds OK.\r\n");
			
			break;
		}
		case TDCN_TEDS:
		{
			//#未完成#：这里应该根据变送器通道号来获取其teds内容
			printf("queryTEDS():querying tdcn %d teds.\r\n",tdcn_num);
			iset=(u8t*)query_TEDS_response;
			*iset=QUERYTEDS;
			*(++iset)=TDCN_TEDS;
			*(++iset)=0;//status bit
			*(++iset)=0;//TEDS attribure;
			attribute=(u32t*)iset;
			*attribute=118;//tdcn_teds大小
			*(++attribute)=118;
			maximum_block_size=(u16t*)(iset+8);
			*maximum_block_size=118;
			
			err=TBIM_send_cmd_reply_msg(0,query_TEDS_response,14);
			SSNP_DEBUG_PRINT("queryTEDS():querying tdcn teds OK.\r\n");
			break;			
		}
		default:break;
	}
}
static void readTEDS(u8t cmdFunc,u8t tdcn_num, void *argument)
{
	u8t i;
	u8t* iset;
	err_t err;
	u32t* offset;
	u16t* max_block_size;
	
	u8t meta_teds[3+35];
	u8t tdcn_teds[3+118];
	switch(cmdFunc)
	{
		case MODULE_META_TEDS:
		{
			SSNP_DEBUG_PRINT("readTEDS():reading meta teds.\r\n");
			iset=(u8t*)argument;
			offset=(u32t*)argument;//得到两个read teds block命令参数
			max_block_size=(u16t*)(iset+4);//得到两个read teds block命令参数
			
			iset=(u8t*)meta_teds;
			*iset=READTEDSBLCOK;
			*(++iset)=MODULE_META_TEDS;
			*(++iset)=0;//status bit
			for(i=0;i<35;i++)//meta_teds中没有数据，全部清零
				iset[i]=0;
			
			err=TBIM_send_cmd_reply_msg(0,meta_teds,38);
			tbim.init_st=READ_TDCN_TEDS;
			SSNP_DEBUG_PRINT("readTEDS():reading meta teds OK.\r\n");
			break;
		}
		case TDCN_TEDS:
		{
			iset=(u8t*)argument;
			offset=(u32t*)argument;//得到两个read teds block命令参数
			max_block_size=(u16t*)(iset+4);//得到两个read teds block命令参数
			printf("readTEDS():reading tdcn %d teds.\r\n",tdcn_num);
			iset=(u8t*)tdcn_teds;
			*iset=READTEDSBLCOK;
			*(++iset)=TDCN_TEDS;
			*(++iset)=0;//status bit
			for(i=0;i<118;i++)//tdcn_teds中没有数据，全部清零
				iset[i]=0;		

			err=TBIM_send_cmd_reply_msg(0,tdcn_teds,121);
			tbim.init_st=WAITTING_FOR_TIME;
			SSNP_DEBUG_PRINT("readTEDS():reading tdcn teds OK.\r\n");
			break;			
		}
		default:break;
	}
}
static void writeTEDS(u8t cmdFunc, u8t tdcn_num,void *argument)
{
}
static void updateTEDS(u8t cmdFunc,u8t tdcn_num, void *argument)
{
}
static void setOPmode(u8t cmdFunc,u8t tdcn_num, void *argument)
{
}
static void readOPmode()
{
}
static void rundiagnostics(u8t cmdFunc)
{
}
static void invalidCommand()
{
}

void execute(u8t cmdClass, u8t cmdFunc,u8t tdcn_num,void *argument)
{
    switch(cmdClass)
		{
    case INITIALIZATION:
	  {
        if(cmdFunc>=1&&cmdFunc<INITCOMMNUM)
            initialization[cmdFunc](tdcn_num,argument);
        else
            invalidCommand();
        break;
    }
    case OPERATIONAL:
		{
        if(cmdFunc>=1&&cmdFunc<OPCOMMNUM)
            op[cmdFunc](tdcn_num,argument);
        else
            invalidCommand();
        break;
    }
    case QUERYTEDS:
		{
        if(cmdFunc>=MODULE_META_TEDS && cmdFunc<=PHY_TEDS)
            queryTEDS(cmdFunc,tdcn_num,argument); 
        else
            invalidCommand();
        break;
    }
    case READTEDSBLCOK:
		{
        if(cmdFunc>=MODULE_META_TEDS && cmdFunc<=PHY_TEDS)
            readTEDS(cmdFunc,tdcn_num,argument);
        else
            invalidCommand();
        break;
    }
    case WRITETEDSBLCOK:
		{
        if(cmdFunc>=MODULE_META_TEDS && cmdFunc<=PHY_TEDS)
            writeTEDS(cmdFunc,tdcn_num,argument);
        else
            invalidCommand();
        break;
    }
    case UPDATETEDS:
		{
        if(cmdFunc>=MODULE_META_TEDS && cmdFunc<=PHY_TEDS)
            updateTEDS(cmdFunc,tdcn_num,argument);
        else
            invalidCommand();
        break;
    }
    case SETOPERATINGMODE:
		{
        if(cmdFunc>=1&&cmdFunc<=OPMODENUM)
            setOPmode(cmdFunc,tdcn_num,argument);
        else
            invalidCommand();
        break;
    }
    case READOPERATINGMODE:
		{
        readOPmode();
        break;
    }
    case RUNDIAGONSITICS:
		{
        rundiagnostics(cmdFunc);
        break;
    }
    default:
		{
        invalidCommand();
        break;
    }
   }
}
void trigger(void* argument)
{
	struct TBIM_arg* arg;
	
	printf("TBIM(): TBIM get trigger.\r\n");
	
	arg=(struct TBIM_arg*)argument;
	netconn_sendtrigger(arg->conn,arg->tbim_tdcn_num,arg->alias,arg->tdcn_num);	
}
void trigger_do()
{
	
}
