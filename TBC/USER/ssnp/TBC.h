#ifndef TBC_H
#define TBC_H
#include "opt.h"
#include "api.h"

enum CMD_CATEGORY
{
	INITIALIZATION=1,
	OPERATIONAL,
	QUERY_REDS,
	READ_TEDS_BLOCK,
	WRITE_TEDS_BLOCK,
	UPDATE_TEDS,
	SET_OPERATING_MODE,
	READ_OPERATING_MODE,
	RUN_DIAGNOSTICS
};
enum INITIALIZATION_COMMAND
{
	SLEEP=1,
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
#define MAX_TBIM_NUM      32
#define MAX_SENSOR_NUM    16
#define SYN_TIMESLOT_NUM  512
#define TIMESLOT_LEN      100

struct sensor
{
	u8t state;//�˱�������ǰ״̬
	
	//������ͨ����ַ
	u8t nChnAddr;
	
	//���ݿ��
	u8t bitWidth;
	
	//�汾��
	u8t version;
	
	//��������
	u16t nPeriod;
	
	//���ݻ�����
	u32t dataBuf;
};
struct tbim
{
	u8t valid;
	
	//tbim����
	u8t alias;
	//ע�����ﵽuuid�ֶν���Ϊ����ʹ��
	u8t uuid;

	//��TBIM����������
	u8t sensor_num;

	//TBCͬ��TBIMͨ�ŵ���������������
	struct netconn* conn_dp;
	struct netconn* conn_sdp;
	struct netconn* conn_tp;

	//��TBIM�еĴ�������
	struct sensor sensor_table[MAX_SENSOR_NUM];
};
struct TBC_
{
	struct tbim tbim_table[MAX_TBIM_NUM];
	sys_mbox_t cmd_reply_recv_mbox;
	sys_mbox_t streaming_data_recv_mbox;
	u8t first_free;
	u16t max_period;
	u16t min_period;

	u16t time_slot_seq;
	u16t asyn_num;
	u16t syn_num;
};
enum tbc_msg_type
{
	TBC_MSG_UPPER=0,
	TBC_MSG_NEW_TBIM,
	TBC_MSG_RECV_TBIMSG//#δ���#����ʱ��mbox�м��볬ʱmsg
};

struct tbc_msg
{
	enum tbc_msg_type type;
	union 
	{
		struct
		{
			u8t tbim_alias;//tbim����
			u8t tdcn_num;//tbim������ͨ����
			u8t cmdclass;
			u8t cmdfunc;
			void* arg;
		}upper_msg;//��λ�����ݵ����ݣ���Ҫ��������
		
		void* new_tbim_list;//�¼����tbim����
	}msg;
};

struct schedultable_item
{	 
	u8t alias;
	u8t tdcn;
	u16t num;
	u16t begin_time_slot;
};
struct cmd_item;
/*
--------------------------------------------------------------------------------------------------------
					      	   Ӧ�ó�����Ҫʹ�õĽӿ�

ע�⣺��������ͽӿڲ�����˵�����������в�������ô�����߸�������Ͳ������cmd_data���������
      û�в�����ô���Ե��ò������������͹��ܵĽӿ�
--------------------------------------------------------------------------------------------------------
*/
void TBC_init(sys_mbox_t sdr_mbox,sys_mbox_t crr_mbox);

void TBIM_recongnition(void* alias_list);
void timeslot_alloc(void);
void start_epoch(void);
void get_epoch(void);
void set_syn_asyn_num(u16t syn,u16t asyn);
void send_schedul_table(struct schedultable_item* table,u16t table_len);
struct tbim* find_tbim(u8t alias);
err_t TBC_send_noArguCmd_to_tbim(struct tbim* t,u8t dst_tdcn_num,u8t cmdClass, u8t cmdFunc);
err_t TBC_send_upcomputer_cmd(struct cmd_item item);
err_t TBC_send_cmd_to_tbim(struct tbim* t,u8t dst_tdcn_num,void* cmd_data,u16t data_len);
err_t TBC_send_noArgucmd(u8t alias,u8t tbim_tdcn_num,u8t cmdclass,u8t cmdfunc);
err_t TBC_send_cmd(u8t alias,u8t tbim_tdcn_num,void* cmd_data,u16t data_len);
err_t TBC_send_trigger(u8t alias,u8t tbim_tdcn_num);
err_t TBC_send_trigger_to_tbim(struct tbim* t,u8t tbim_tdcn_num);

//err_t TBC_recv_streaming_data_unblock(u8t alias,u8t tbim_tdcn_num,struct pbuf** buf);
//err_t TBC_recv_streaming_data_from_tbim_unblock(struct tbim* t,u8t *tbim_tdcn_num,struct pbuf** buf);
#endif
