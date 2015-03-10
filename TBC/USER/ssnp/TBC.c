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
//static sys_mbox_t mbox;//�����ݴ洢����
//static u8t first_free;

static void Delay(u16t time)
{
	OSTimeDly(time/(1000/OS_TICKS_PER_SEC));
}
//#δ���#������Ӧ���ж����connΪ�պ�firstfree����������ֵ
/*
      ����ķ��͵�һ��ʹ��ʱ��û������ģ����ǵ�ϵͳ�������к��ٴ����µ�TBIM����������
   һЩ��ʼ������ֻ�����첽ʱ����ڽ��д��䣬��������ķ��ͺ���û�а취��֤������Щ��ʼ��
   ����һ�����첽ʱ����ڣ�������ֵ��������ͬ���첽״̬������Ӧ����Э����������Ӧ����Ӧ��
   ��������
       ��ô�о����״̬��Ӧ����Ӧ�ò���ά������������������Ļ������һ��������ǣ�����Ҳ��������
    Э���ں˵Ĵ���취��һ��mbox��ά��������Ҫ�������Ϣ,����ͬ��(����TBC��˵�ǽ�������)
     ���첽��Ϣ(����TBC��˵����λ�����͵�����)�Լ������¼����TBIM��Ϣ�����ͬһ��mbox�У�������
    û�а취��֤�����������첽ʱ����ڣ�ʱ����ڣ�ʱ�����.......����û��ʲô���⡣

       �������ǿ��Եģ���״̬����Ӧ�ò�ά����������ͬ��ʱ����ڲ�ѯ���е�����TP���ӿ��Ƿ������ݣ�ͬʱ
    ����TBIM����������TBC��������Ҳ����dp���ݺ�trigger�������ͼ��ˣ����첽����TBC�ͼ���Ƿ���
    ��λ�����͵����ͬʱ����Ƿ����¼����TBIM�Ϳ����ˡ�

        ������������ʱ�侫ȷ�ȵ������ˣ���������ͬ��ʱ���Ϊ5.12ms,����������Ծ�ȷ�Ľ��м�ʱ��
    ����ľ�ȷ�������������������ĸ�������һ�����Ծ�ȷ�ж��ģ���������timer��ʵ�ַ��������ǲ�̫����
    ��ȷ��ʱ��0.01ms���������ĸ�ʱ�����һ������׼ȷ�жϵģ������Ϳ����ˡ�
        ���ﻹ��һ�����⣬����TBC�ڴ���������ʱ�п��ܻᳬ��ͬ��ʱ��Σ����Ƕ���TBC��˵������ֻ�����ڽ���
    ����ʱ���Ҳû��ʲô���⡣���Ƕ����첽ʱ��� �ķ�����λ����������������첽ʱ���Ӧ����ô����Ҳ
    ����˵��ʼʱ�䲻һ��Ҫ�ྫȷֻҪ�ܱ�֤�����ʱ����ھͿ����ˣ����Ƕ��ڽ�ֹʱ����˵һ��Ҫ��ȷ��
		����Ӧ��ʱ�����ֹͣ�������Ժ���һ��ʱ��γ�ͻ����ξ�ȷ�Ľ�ֹ�������Ӧ����δ���һ�ַ���������
		������ǰ����һ�·��ʹ���������Ҫ�೤ʱ�䣬�������������һʱ��β����ͣ������ͣ�����Ҳ�ǿ��Եġ�
		
		  ����ʱ��ε�ά�����ǿ����Լ�������ʹ��OSTimeGet()����ÿ�λ�ȡʱ�䣬����ÿ��ticket��ʱ�䳤����10ms
		�������ͬ��ʱ���ֻ��5ms,��˲�����ʹ��������������ʱ�����
		  ����о�ȷ��ʱ����,�����Ȳ�Ҫ��TBIM��һ�����׼ȷ��TBCͬ�����������Ƚ��TBC�Ͼ�ȷ��ʱ�����⡣��������
		�ҵ��˾�ȷ��ʱ�İ취����ô�����ڵõ�TBIM��Ϣ���ҷ�����ʱ��Ƭ��
		//΢���ӳ� http://blog.csdn.net/liuyu60305002/article/details/6942722
		
		   ����Ҳ����������ƾ���TBC�ڷ���begin epoch��TBC��TBIM֮���̶��ȴ�һ��ʱ���T��ʼ����TBIM���յ�����Tʱ���
		�ڿ������㹻��ʱ�����������жϣ������Ϳ��Զ������ݵķ��ͽ���һ��ʱ�����ã����ٴν��յ�begin epoch�����½���
		�жϾͿ����ˡ�
		   http://zhidao.baidu.com/link?url=uPFWjvuVr_GkMgRGRhJlXS1CnJaSnU-f7awBz55LfYZYGSDeElzX1eXDGuIImbsPNwKgofO9cuuj4zWg54qRaa
		����ʹ������жϵķ�ʽ��
		
		   ������һ��Ӧ�ó���ӿ�ʼ���÷��ͺ�����������ȫ������Ҫ92us������trigger������Ҫ42us����100us������ôÿ��ʱ���
		Ӧ����100us��512����������Ҫ51200us=51.2ms��ÿ��epochΪ100ms�������ǱȽϺ���ġ�
		   ���������ľ�ȷʱ����㺯����TBC״̬����ά���Ϳ�����ȫ��Ӧ�ó�����ʵ���ˣ���ʼ״̬�ǿ���״̬�ڷ���epoch��ȴ�һ��ʱ��
		����һ��ʱ����趨Ϊ200us�����������Ǳ�֤���е�TBIM���ϲ�Ӧ�õõ�����Ϣ��TBC����ͬ��ʱ���״̬�������״̬��ֻȥ��������Ȼ��
		��ѯ�ǲ��ǳ�����51200us���������������첽״̬����ѯmbox�����������ÿ�η�������֮ǰ����������ʹ�����᲻���ͬ��ʱ��γ�ͻ
		���ʱ�仹Ҫ���ǵ�TBIM���ܲ��ظ���ʱ�䣬������Է�����ô���;Ϳ����ˣ�Ȼ��ʱ���ǲ��ǽ�����ͬ��״̬���������״̬����ά���Ϳ���
		��TBC��Ӧ�ó�����ʵ���ˡ�
		
*/

/*
---------------------------------------------------------------------------------------------
         ע��TBC����һ��Ҫ����helper��������Ҫ�����漰������Ӧ�ó��������
         Ӧ�ó�����Ҫ���ľ��ǽ�����λ�������Ȼ����������������ݣ�����ʱ����Ϳ����ˡ�
     ���о���TBC�����ݽṹ��ƺã���������Щ������TBC��Ҫ֪������Ӧ�ó�����Ҫ֪���ģ�
   

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


	buf.alias=t->alias;//�Է��ı���
	buf.tdcn_num=tdcn_num;  
	
	buf.tbim_tdcn_num=0;
	buf.p=NULL;	//����һ��Ҫע�⽫p����Ϊ��
	
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

	buf.alias=t->alias;//�Է��ı���
	buf.tdcn_num=tdcn_num;  
	
	buf.tbim_tdcn_num=0;
	buf.p=NULL;	//����һ��Ҫע�⽫p����Ϊ��
	
	netbuf_ref(&buf,(void*)data,2);
	err=netconn_sendto(t->conn_dp,&buf,buf.alias,buf.tdcn_num);
//	printf("send_noArguCmd_to_tbim():send cmdclass %d cmdfunc %d to tbim %d.\r\n",cmdClass,cmdFunc,t->alias);
	netbuf_free(&buf);
	
	return err;
}
/*
  TBC��TBIM�������ݱ�����Ҫָ��TBC�Լ���
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
	if(TBC_send_noArguCmd_to_tbim(t,0,INITIALIZATION,READ_TBIM_STRUCTURE)!=ERR_OK)//�����0�������TBIM��������ָTBC����
	{
		SSNP_DEBUG_PRINT("get_TBIM_struct():send get tbim struct msg failed.\r\n");
		return ERR_VALUE;
	}
//	SSNP_DEBUG_PRINT("get_TBIM_struct():send tbim struct msg done.\r\n");
	for(n =0;n<RECV_TRY_NUM;n++)
	{//	printf("get_TBIM_struct():try %d.\r\n",n);
		if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
		{//	SSNP_DEBUG_PRINT("get_TBIM_struct():get tbim struct reply msg.\r\n");
			data=(u8t*)buf->p->data;//����ֻ��Ҫ֪��һ��TBIM�д�����������,��Ϊ���涼�������ģ�
			t->sensor_num=(*(data+3));//��������ظ�Э������ͷ����Ϣ
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
	u8t read_teds_cmd[2+6];//����ͷ2(TBC��һ��û��status octetλ)+������С6
	
	
	isOk=0;
	if(TBC_send_noArguCmd_to_tbim(t,0,QUERY_REDS,MODULE_META_TEDS)!=ERR_OK)//�����0�������TBIM��������ָTBC����
		return ERR_VALUE;
	
	for(n =0;n<RECV_TRY_NUM;n++)
	{
		if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
		{
			//#δ���#�����ﲻ������ֱ����Ϊmeta_teds�Ǵ��ڵġ�
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
	*offset=35;//meta_teds��С��λ35�ֽ�
	max_block_size=(u16t*)(iset+4);
	*max_block_size=35;
//	SSNP_DEBUG_PRINT("get_TBIM_meta_TEDS():sending read meta teds cmd.\r\n");
	if(TBC_send_cmd_to_tbim(t,0,read_teds_cmd,8)!=ERR_OK)//�����0�������TBIM��������ָTBC����
		return ERR_VALUE;
	
	isOk=0;
	for(n =0;n<RECV_TRY_NUM;n++)
	{
		if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
		{
			//#δ���#�����ﲻ���������յ�meta_teds��ֱ��ɾ���ˡ�
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
	u8t read_teds_cmd[2+6+1];//����ͷ2(TBC��һ��û��status octetλ)+������С7�����а�������һ��������ͨ����Ѱַ
	
	
	read_teds_cmd[0]=READ_TEDS_BLOCK;
	read_teds_cmd[1]=TDCN_TEDS;
	iset=(u8t*)read_teds_cmd;
	offset=(u32t*)(iset+2);
	*offset=118;
	max_block_size=(u16t*)(iset+4);
	*max_block_size=118;
	iset+=8;//ָ�������ͨ��������λ
	
	
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
				//#δ���#�����ﲻ������ֱ����Ϊtdcn_teds�Ǵ��ڵġ�
				isOk=1;
				netbuf_delete(buf);
				break;
			}
			Delay(CMD_RESPONSE_WAIT_TIME);	
		}
		if(!isOk)
			return ERR_VALUE;	
		
		*iset=n;//д����Ҫ��ȡ�ĸ�������ͨ����tdcn_teds
		if(TBC_send_cmd_to_tbim(t,0,read_teds_cmd,9)!=ERR_OK)
			return ERR_VALUE;
		
		isOk=0;
		for(i=0;i<RECV_TRY_NUM;i++)
		{
			if(netconn_recv(t->conn_dp,&buf) == ERR_OK)
			{
				//#δ���#:���ﲻ���������յ�tdcn_teds��ֱ��ɾ���ˡ�
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
	netconn_bind(t->conn_dp,TBC_ADDRESS,0);//TBC�����Լ��ĵ�ַ
	netconn_connect(t->conn_dp,alias,0);//�B�ӵ�����
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
	set_connection_recvmbox(t->conn_sdp,tbc.streaming_data_recv_mbox);//��mbox�ҽӵ�����sdp���ӵĽ��ջ���
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
TBIM_recongnition:��ɵ������ǣ�

 	  ���ݱ�������������������
		����Read TBIM structure����
		��ȡTBIM��meta_TEDS
		��ȡTBIM����������ͨ��TEDS
		����TBIM��TEDS��Ϣ
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
	//ʹ��tbc_send_timeslot_msg()��������ʱ��۷���msg
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
	//����ʱ���.
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