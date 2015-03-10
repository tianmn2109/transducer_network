#include "bcp_msg.h"
#include "ethbcp.h"
#include "debug.h"
#include "def.h"
#include "ethernetif.h"
#include "netif.h"
#include "tp.h"
#include "dp.h"
#include "sdp.h"


enum //��Щ���������������ط�ʹ�õ����ֻ��Ҫ�����ﶨ��
{
	Initiate_discovery=1,
	Discovery_reply,
	Assign_alias,
	Assign_alias_response,
	Assign_timeslot,
	Assing_timeslot_reply,
	Define_epoch,
	Disarm_streaming,
	Beginning_of_epoch,
	Start_asy_interval,
	Reflect,
	Reflect_reply,
	Enable_transmitter,
	Disable_transmitter
};
enum
{
	UUID_LEN=10
};


/*******************************************TBC���д���*****************************************************************
 *
 * A �����TBC����λ����Ҫ��TBC�����ǹ̶�255
 * B ����Ҳ����Ҫ��һ������-macӳ���ֱ����mac��ַ��
 *   ��ȡ��������Ȼ����λͼ�в��ҾͿ����ˣ�����ÿһ��
 *   ������״̬ά������Ҫ�е�
 *
 *
 ***********************************************************************************************************************/

#if NODE == TBC
#include "alias_map.h"
#include "alias_state.h"

#define TBC_ALIAS           0xff
#define TIME_SLOT_MSG_LEN   64
static u8t alias_alloc_done;
static struct pbuf* time_slot_msg;//�ײ����ڷ��ͷ���ʱ�����ʹ�õ��ڴ�
struct TBC_DLL_state
{
	u8t tbc_mac_addr[ETHBCP_HWADDR_LEN];//TBC��mac��ַ
	u8t tbim_reflect_msg_done;
};
static struct TBC_DLL_state tbc_dll_state;

void TBC_DLL_init()
{
	tbc_dll_state.tbc_mac_addr[0]=0x40;
	tbc_dll_state.tbc_mac_addr[1]=TBC_ALIAS;
	tbc_dll_state.tbc_mac_addr[2]=0x00;
	tbc_dll_state.tbc_mac_addr[3]=0x00;
	tbc_dll_state.tbc_mac_addr[4]=0x00;
	tbc_dll_state.tbc_mac_addr[5]=0x00;
	
	tbc_dll_state.tbim_reflect_msg_done=0;

	time_slot_msg=pbuf_alloc(PBUF_RAW,TIME_SLOT_MSG_LEN,PBUF_RAM);
	if(time_slot_msg == NULL)
		SSNP_DEBUG_PRINT("TBC_DLL_init():run out of memory.\n");
	
	set_mac_address(tbc_dll_state.tbc_mac_addr);//#δ���#�����ﻹ�е����⣬��������������mac��ַ��
}
void* get_done_list(void)
{
	void* list=	get_alias_done_list();		
	if(list == NULL)
		SSNP_DEBUG_PRINT("get_done_list():empty list.\n");	
	return list;
}
u8t is_reflect_reply_ok(void)
{
	return tbc_dll_state.tbim_reflect_msg_done;
}
u8t get_alias_alloc_state()
{
	return alias_alloc_done;
}
void set_alias_alloc_done()
{
	alias_alloc_done=1;
}
void set_reflect_reply_unfinished(void)
{
	tbc_dll_state.tbim_reflect_msg_done=0;
}
/***********************************************************************************
 * #δ���#��ע����������ÿ�����ݱ��ĳ�����64�ֽ����Կ���ʹ���ڴ�صķ��䷽��
 ***********************************************************************************/
err_t InitiateDiscovery(struct netif* netif)
{
	struct ethbcp_hdr* bcphdr;
	struct pbuf* p;
	u16t buf_len;
	err_t err;
	u8t* iset;
	u8t* inti;
	u8t i;

	buf_len=SIZEOF_EHTBCP_HDR + 1 + 2 + 40 + 4;
	p=pbuf_alloc(PBUF_RAW,buf_len,PBUF_RAM);
	if(p == NULL)
	{
		SSNP_DEBUG_PRINT("ethbcp_discover_reply():run out of memory.\n");
		return ERR_MEMORY;
	}

	bcphdr=(struct ethbcp_hdr*)p->data;
	for(i=0;i<ETHBCP_HWADDR_LEN;i++)
	{
		bcphdr->eth_dest_addr.addr[i]=0xff;
		bcphdr->eth_src_addr.addr[i]=tbc_dll_state.tbc_mac_addr[i];
	}
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;

	iset=(u8t*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
	*iset=Initiate_discovery;

	++iset;
	for(i=0;i<2+40+4;i++)
		iset[i]=0;
	inti=((u8t*)p->data + SIZEOF_EHTBCP_HDR);
	//printf("InitiateDiscovery():the msg code is %d.\r\n",*inti);
	err=netif->linkoutput(netif,p);
	pbuf_free(p);
	
	return err;
}

static err_t send_assign_alias_msg(struct netif* netif,u8t tbim_alias,struct uuid* tbim_uuid)
{
	struct ass_alias_msg* assign_alias_msg;
	struct ethbcp_hdr* bcphdr;
	struct pbuf* p;
	err_t err;
	u16t buf_len;
	u8t* iset;
	u8t i;

	buf_len=SIZEOF_EHTBCP_HDR+12+2+29+4;
	p=pbuf_alloc(PBUF_RAW,buf_len,PBUF_RAM);
	if(p == NULL)
	{
		SSNP_DEBUG_PRINT("ethbcp_discover_reply():run out of memory.\n");
		return ERR_MEMORY;
	}
	bcphdr=(struct ethbcp_hdr*)p->data;
	for(i=0;i<ETHBCP_HWADDR_LEN;i++)
	{
		bcphdr->eth_dest_addr.addr[i]=0xff;
		bcphdr->eth_src_addr.addr[i]=tbc_dll_state.tbc_mac_addr[i];
	}
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;

	assign_alias_msg=(struct ass_alias_msg*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
	assign_alias_msg->type=Assign_alias;
	for(i=0;i<UUID_LEN;i++)
		assign_alias_msg->uid.uid[i]=tbim_uuid->uid[i];
	assign_alias_msg->alias=tbim_alias;

	iset=(u8t*)p->data+SIZEOF_EHTBCP_HDR+12;

	for(i=0;i<2+29+4;i++)
		iset[i]=0;

	err=netif->linkoutput(netif,p);
	pbuf_free(p);
	
	return err;
}

err_t send_assign_time_slot_msg(struct netif* netif,u8t alias,u8t tbim_tdcn_num,u16t beign_time_slot,u8t time_slot_num)
{
	struct ethbcp_hdr* bcphdr;
	struct ass_timeslot_msg* assign_timeslot_msg;
	u8t i;
	u8t* iset;
	
	if(time_slot_msg == NULL)
	{
		SSNP_DEBUG_PRINT("send_assign_time_slot_msg():time_slot_msg is empty.\r\n");
		return ERR_MEMORY;
	}
	bcphdr=(struct ethbcp_hdr*)time_slot_msg->data;
	bcphdr->eth_dest_addr.addr[0]=0x40;
	bcphdr->eth_dest_addr.addr[1]=alias;
	bcphdr->eth_src_addr.addr[0]=0x40;
	bcphdr->eth_src_addr.addr[1]=TBC_ALIAS;
	for(i=2;i<6;i++)
	{
		bcphdr->eth_dest_addr.addr[i]=0x00;
		bcphdr->eth_src_addr.addr[i]=0x00;
	}
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;
	
	assign_timeslot_msg=(struct ass_timeslot_msg*)((u8t*)time_slot_msg->data + SIZEOF_EHTBCP_HDR);
	assign_timeslot_msg->type=Assign_timeslot;
	assign_timeslot_msg->alias=alias;
	assign_timeslot_msg->tbim_tdcn_num=tbim_tdcn_num;
	assign_timeslot_msg->bgn_timeslot=HTONS(beign_time_slot);
	assign_timeslot_msg->slot_num=time_slot_num;
	
	iset=(u8t*)time_slot_msg->data+SIZEOF_EHTBCP_HDR+6;
	for(i=0;i<2+35+4;i++)
		iset[i]=0;
	//printf("send_assign_time_slot_msg():begin time slot is %d.\r\n",beign_time_slot);
//	printf("send_assign_time_slot_msg():begin time slot is %d.\r\n",beign_time_slot);
	return netif->linkoutput(netif,time_slot_msg);
}
//ע������ʹ�õ�pbuf��ȫ�ֱ���time_slot_msg��
err_t send_define_epoch_msg(struct netif* netif,u16t syn_timeslot_num,u16t asyn_timeslot_num)
{
	struct ethbcp_hdr* bcphdr;
	struct def_epoch_msg* define_epoch_msg;
	u8t i;
	u8t* iset;
	u16t* syn;
	u16t* asyn;
	
	if(time_slot_msg == NULL)
	{
		SSNP_DEBUG_PRINT("send_define_epoch_msg():time_slot_msg is empty.\r\n");
		return ERR_MEMORY;
	}
	bcphdr=(struct ethbcp_hdr*)time_slot_msg->data;
	for(i=0;i<6;i++)
	{
		bcphdr->eth_dest_addr.addr[i]=0xff;
		bcphdr->eth_src_addr.addr[i]=0x00;
	}
	bcphdr->eth_src_addr.addr[0]=0x40;
	bcphdr->eth_src_addr.addr[1]=TBC_ALIAS;	
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;
//	iset=(u8t*)((u8t*)time_slot_msg->data+SIZEOF_EHTBCP_HDR);
//	*iset=Define_epoch;
//	syn=(u16t*)(iset+1);
//	*syn=syn_timeslot_num;
//	asyn=(u16t*)(iset+3);
//	*asyn=asyn_timeslot_num;
	define_epoch_msg=(struct def_epoch_msg*)((u8t*)time_slot_msg->data + SIZEOF_EHTBCP_HDR);
	define_epoch_msg->type=Define_epoch;
	define_epoch_msg->iso_itv=HTONS(syn_timeslot_num);
	define_epoch_msg->asy_itv=HTONS(asyn_timeslot_num);
	iset=(u8t*)((u8t*)time_slot_msg->data+SIZEOF_EHTBCP_HDR+5);
	for(i=0;i<2+36+4;i++)
		iset[i]=0;
	//printf("send_define_epoch_msg():send define epoch msg syn is %d asyn is %d.\r\n",syn_timeslot_num,asyn_timeslot_num);
	return netif->linkoutput(netif,time_slot_msg);	
}
err_t send_reflect_msg(struct netif* netif,u8t alias)
{
	struct ethbcp_hdr* bcphdr;
	u8t i;
	u8t* iset;
	err_t err;	
	
	if(time_slot_msg == NULL)
	{
		SSNP_DEBUG_PRINT("send_reflect_msg():time_slot_msg is empty.\r\n");
		return ERR_MEMORY;
	}
	bcphdr=(struct ethbcp_hdr*)time_slot_msg->data;
	
	bcphdr->eth_src_addr.addr[0]=0x40;
	bcphdr->eth_src_addr.addr[1]=TBC_ALIAS;
	bcphdr->eth_src_addr.addr[2]=0x00;
	bcphdr->eth_src_addr.addr[3]=0x00;		//Ϊ�˺�TBIM��ʱ�������һ��
	bcphdr->eth_src_addr.addr[4]=0x00;
	bcphdr->eth_src_addr.addr[5]=0x00;		
	
	bcphdr->eth_dest_addr.addr[0]=0x40;
	bcphdr->eth_dest_addr.addr[1]=alias;
	bcphdr->eth_dest_addr.addr[2]=0x00;
	bcphdr->eth_dest_addr.addr[3]=0x00;
	bcphdr->eth_dest_addr.addr[4]=0x00;
	bcphdr->eth_dest_addr.addr[5]=0x00;
	
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;
	iset=(u8t*)((u8t*)time_slot_msg->data + SIZEOF_EHTBCP_HDR);
	*iset=Reflect;
	*(++iset)=alias;
	for(i=0;i<2+39+4;i++)
		iset[i]=0;	
	//printf("send_reflect_msg():send reflect msg to tbim %d.\r\n",alias);
	err=netif->linkoutput(netif,time_slot_msg);	
	//SSNP_DEBUG_PRINT("send_reflect_msg():send reflect msg done.\r\n");
	return err;
}
//ע������ʹ�õ�pbuf��ȫ�ֱ���time_slot_msg
err_t send_begin_of_epoch_msg(struct netif* netif)
{
	struct ethbcp_hdr* bcphdr;
	u8t i;
	u8t* iset;	
	
	if(time_slot_msg == NULL)
	{
		SSNP_DEBUG_PRINT("send_define_epoch_msg():time_slot_msg is empty.\r\n");
		return ERR_MEMORY;
	}
	bcphdr=(struct ethbcp_hdr*)time_slot_msg->data;
	
	bcphdr->eth_src_addr.addr[0]=0x40;
	bcphdr->eth_src_addr.addr[1]=TBC_ALIAS;
	bcphdr->eth_src_addr.addr[2]=0x00;
	bcphdr->eth_src_addr.addr[3]=0x00;		
	bcphdr->eth_src_addr.addr[4]=0x00;
	bcphdr->eth_src_addr.addr[5]=0x00;		
	
	bcphdr->eth_dest_addr.addr[0]=0xff;
	bcphdr->eth_dest_addr.addr[1]=0xff;
	bcphdr->eth_dest_addr.addr[2]=0xff;
	bcphdr->eth_dest_addr.addr[3]=0xff;
	bcphdr->eth_dest_addr.addr[4]=0xff;
	bcphdr->eth_dest_addr.addr[5]=0xff;
	
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;
	iset=(u8t*)((u8t*)time_slot_msg->data + SIZEOF_EHTBCP_HDR);
	*iset=Beginning_of_epoch;
	*(++iset)=0;
	for(i=0;i<1+40+4;i++)
		iset[i]=0;	
//	printf("send_begin_of_epoch_msg():TBC send send_begin_of_epoch_msg.\r\n");
	return netif->linkoutput(netif,time_slot_msg);	
}
#endif
//*****************************************************************************************************************************





/*******************************************TBIM���д���*********************************************************************
 *
 *
 *
 *
 ****************************************************************************************************************************/
#if NODE == TBIM
#include "TBIM.h"
#define ETH_BCP_ENTRY_LEN    6
#define TBC_ALIAS            0xff
#define TIME_SLOT_MSG_LEN    64
static struct pbuf* time_slot_reply_msg;//�ײ����ڷ��ͷ���ʱ��ۻظ���ʹ�õ��ڴ�
struct ethbcp_entry
{
	u8t alias;
	struct eth_addr ethaddr;
	u8t state;
};
struct TBIM_DLL_state
{
	struct ethbcp_entry bcp_table[ETH_BCP_ENTRY_LEN];//TBIM������arpӳ���
	u8t new_mac_addr[ETHBCP_HWADDR_LEN];//TBIM�µ�mac��ַ
	u8t assign_alias_done;//�Ƿ�õ�����
	u8t define_epoch_done;
	u8t begin_timeslot_done;
	u8t reflect_done;
	u8t begin_streaming_ok;
	u8t alias;//TBIM����
	
	u16t syn_itv;
	u16t asyn_itv;
	u16t begin_timeslot;
};
static struct TBIM_DLL_state tds;//TBIM��������·���ȫ��״̬�ṹ��



u8t getalias(void)
{
	return  tds.alias;
}
void TBIM_DLL_init()//������·���ʼ������
{
	tds.assign_alias_done=0;
	tds.bcp_table[0].ethaddr.addr[0]=0x40;
	tds.bcp_table[0].ethaddr.addr[1]=TBC_ALIAS;
	tds.bcp_table[0].ethaddr.addr[2]=0x00;
	tds.bcp_table[0].ethaddr.addr[3]=0x00;
	tds.bcp_table[0].ethaddr.addr[4]=0x00;
	tds.bcp_table[0].ethaddr.addr[5]=0x00;
	time_slot_reply_msg=pbuf_alloc(PBUF_RAW,TIME_SLOT_MSG_LEN,PBUF_RAM);
	if(time_slot_reply_msg == NULL)
		SSNP_DEBUG_PRINT("TBIM_DLL_init():run out of memory.\n");
}
u8t is_assign_time_slot_done(void)
{
	return tds.begin_timeslot_done;
}
u8t is_streaming_ok(void)
{
	return tds.begin_streaming_ok;
}
void set_reflect_ok_unfinished_TEST(void)
{
	tds.reflect_done=0;
}
u8t is_reflect_ok(void)
{
	return tds.reflect_done;
}
void get_syn_asyn_begin_timeslot(u16* syn,u16t* asyn,u16t* begin_timeslot)
{
	*syn=tds.syn_itv;
	*asyn=tds.asyn_itv;
	*begin_timeslot=tds.begin_timeslot;
}
/*
 *  TBIM��TBC���͵�discovery��Ϣ�Ļظ�
 *  ����netif������ӿ�
 *  ����p,���ɵ��ú��������ָ�룬���ú������������һ�����ڴ�
 *  ����ӳ�����ȫ�ֱ�����������ﲻ��Ҫ������������ڴ�ʱTBIM��û��mac��ַ�������һλ����ȫ1��table[0]������TBC��mac��ַ�ͱ���
 *             ���Ҵ˻ظ���Ϣֻ��Ҫ֪��TBC��mac��ַ�Ϳ�����
 */
static err_t ethbcp_discover_reply(struct netif* netif)
{
	struct eth_hdr* ethhdr;
	struct pbuf* p;
	struct ethbcp_hdr* bcphdr;
	struct dcv_rpl_msg* data;
	struct uuid* uid;
	err_t err;
	u16t buf_len;
	u8t i;
	u8t* iset;

	buf_len=SIZEOF_EHTBCP_HDR+sizeof(struct dcv_rpl_msg)+2+30+4;
	p=pbuf_alloc(PBUF_RAW,buf_len,PBUF_RAM);
	if(p == NULL)
	{
		SSNP_DEBUG_PRINT("ethbcp_discover_reply():run out of memory.\n");
		return ERR_MEMORY;
	}

	ethhdr=(struct eth_hdr*)p->data;
	bcphdr=(struct ethbcp_hdr*)p->data;
	for(i=0;i<ETHBCP_HWADDR_LEN;i++)
	{
		ethhdr->dest.addr[i]=tds.bcp_table[0].ethaddr.addr[i];
		ethhdr->src.addr[i]=0xff;//���ڴ�ʱTBIM�������mac��ַ��������Ϊȫ1��ע���������Դ��ַ��ȫ255�Ļ�����ô�ᱻ�ز�ģ�鶪����
	}
	ethhdr->src.addr[0]=0x00;
//	ethhdr->src.addr[1]=0x00;
//	ethhdr->src.addr[2]=0x00;
//	ethhdr->src.addr[3]=0x00;
//	ethhdr->src.addr[4]=0x00;
//	ethhdr->src.addr[5]=0x01;
	ethhdr->type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;

	data=(struct dcv_rpl_msg*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
	data->type=Discovery_reply;
	uid=get_uuid();
	for(i=0;i<UUID_LEN;i++)
		data->uid.uid[i]=uid->uid[i];

	iset=(u8t*)p->data + SIZEOF_EHTBCP_HDR + sizeof(struct dcv_rpl_msg);
	for(i=0;i<2+30+4;i++)
		iset[i]=0;//#δ���#��ע���������ȫ��Ϊ0��FCSλû����

	err=netif->linkoutput(netif,p);
	pbuf_free(p);
	
	return err;
}

/*
 *  ����netif������ӿ�
 *  ����p��������ָ�룬�����߸�����մ��ڴ�
 *
 *
 */
static err_t ethbcp_assign_alias_respose(struct netif* netif)
{
	struct ethbcp_hdr* bcphdr;
	struct pbuf* p;
	struct ass_alias_rps_msg* data;
	err_t err;
	u16t buf_len;
	u8t* iset;
	u8t i;

	buf_len=SIZEOF_EHTBCP_HDR+sizeof(struct ass_alias_rps_msg)+2+25+4;
	p=pbuf_alloc(PBUF_RAW,buf_len,PBUF_RAM);
	if(p == NULL)
	{
		SSNP_DEBUG_PRINT("ethbcp_discover_reply():run out of memory.\n");
		return ERR_MEMORY;
	}

	bcphdr=(struct ethbcp_hdr*)p->data;
	for(i=0;i<ETHBCP_HWADDR_LEN;i++)
	{
		bcphdr->eth_dest_addr.addr[i]=tds.bcp_table[0].ethaddr.addr[i];
		bcphdr->eth_src_addr.addr[i]=tds.new_mac_addr[i];
	}
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;
	
	data=(struct ass_alias_rps_msg*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
	data->type=Assign_alias_response;
	data->alias=tds.alias;
	data->flag=get_asy_flag();
	data->payload=get_payload_encoding();
	data->start_delay=get_start_delay();//#δ���#����������Ҫ�����ֽ�˳�������𣿻���ֱ��ת��Ϊ�ַ������д���
	data->reflect_delay=get_reflect_delay();//#δ���#��ֻҪ������֧��IEEE��׼�Ͳ���Ҫ���Ǹ������������ֽ�˳������
	data->reflect_delay_uctn=get_reflect_delay_uncertainty();//#δ���#������ʹ��union{float a;unsigned char b[4]}ת��Ϊ�ֽڽ��з���

	iset=(u8t*)p->data + SIZEOF_EHTBCP_HDR + sizeof(struct ass_alias_rps_msg);
	for(i=0;i<2+25+4;i++)
		iset[i]=0; 

	err=netif->linkoutput(netif,p);
	pbuf_free(p);
	
	return err;
}

static u8t is_alias_eq(u8t als)
{
	if(tds.alias == als)
		return 1;
	return 0;
}

err_t send_reflect_reply_msg(struct netif* netif)
{
	struct ethbcp_hdr* bcphdr;
	u8t i;
	u8t* iset;	
	
	if(time_slot_reply_msg == NULL)
	{
		SSNP_DEBUG_PRINT("send_define_epoch_msg():time_slot_msg is empty.\r\n");
		return ERR_MEMORY;
	}
	bcphdr=(struct ethbcp_hdr*)time_slot_reply_msg->data;

	bcphdr->eth_src_addr.addr[0]=0x40;
	bcphdr->eth_src_addr.addr[1]=tds.alias;
	bcphdr->eth_src_addr.addr[2]=0x00;    
	bcphdr->eth_src_addr.addr[3]=0x00;		
	bcphdr->eth_src_addr.addr[4]=0x00;
	bcphdr->eth_src_addr.addr[5]=0x00;		
	
	bcphdr->eth_dest_addr.addr[0]=0x40;
	bcphdr->eth_dest_addr.addr[1]=TBC_ALIAS;
	bcphdr->eth_dest_addr.addr[2]=0x00;
	bcphdr->eth_dest_addr.addr[3]=0x00;
	bcphdr->eth_dest_addr.addr[4]=0x00;
	bcphdr->eth_dest_addr.addr[5]=0x00;
	
	bcphdr->eth_type=HTONS(ETHTYPE_BCP);
	bcphdr->SSType=0;
	bcphdr->SSLength=0;
	bcphdr->SSVersion=0;
	iset=(u8t*)((u8t*)time_slot_reply_msg->data + SIZEOF_EHTBCP_HDR);
	*iset=Reflect_reply;
	*(++iset)=0;//Ϊ�˺�TBC�Ĳ�����һ��
	for(i=0;i<2+39+4;i++)
		iset[i]=0;	
	return netif->linkoutput(netif,time_slot_reply_msg);	
}
#endif
//*****************************************************************************************************************************

//ע��pbuf�Ĵ���û���ڴ˺������ͷ�
err_t ethbcp_output(struct netif* netif,struct pbuf* p,u8t dst_alias,u16t type)
{
	struct eth_hdr* ethhdr;
	struct pbuf* ethhdr_pbuf;
	err_t err;
	//u8t i;

	//SSNP_DEBUG_PRINT("ethbcp_output():remove pbuf dptr to right place.\r\n");
	if(pbuf_header(p,sizeof(struct eth_hdr))!=0)//�ж��Ƿ����㹻��ʣ��ռ䣬ͬʱ������ָ���ƶ������ʵ�λ��
	{
	   // SSNP_DEBUG_PRINT("ethbcp_output():no room for ethhdr, allocating memory for pbuf.\r\n");
	    ethhdr_pbuf=pbuf_alloc(PBUF_RAW,SIZEOF_ETH_HDR,PBUF_RAM);
		if(ethhdr_pbuf == NULL)
		{
			SSNP_DEBUG_PRINT("pbuf_header():could not allocate room for header.\r\n");
			return ERR_BUFFER;
		}
		pbuf_chain(ethhdr_pbuf,p);
	}
	else
	{
		ethhdr_pbuf=p;
	}
//#δ���#��ʵ�������ﻹӦ���и����ƵĻ��ƣ��������������Ӧ��mac��ַ��û�и���δ���ȣ���������ı������ڿ�ʼ��ȷ��������arp
	ethhdr=(struct eth_hdr*)ethhdr_pbuf->data;
	/*
		if(dst_alias==0)
		{
			ethhdr->dest.addr[0]=0xff;
			ethhdr->dest.addr[1]=0xff;
			ethhdr->dest.addr[2]=0xff;      
			ethhdr->dest.addr[3]=0xff;
			ethhdr->dest.addr[4]=0xff;
			ethhdr->dest.addr[5]=0xff;			
		}
	*/
	ethhdr->dest.addr[0]=0x40;//����mac��ַֻ�ͱ����йأ����еõ�������ֱ����дmac��ַ
	ethhdr->dest.addr[1]=dst_alias;//ע������֮���������ǽ�����������ʵʱ�ϣ�������mac��ַ��һһ��Ӧ�ģ����֪���˶Է��ı���Ҳ����֪���˶Է���
	ethhdr->dest.addr[2]=0x00;       //mac��ַ����������û�б�Ҫ����һ��ӳ���ֱ��ת����ʡ�ڴ�ռ�
	ethhdr->dest.addr[3]=0x00;
	ethhdr->dest.addr[4]=0x00;
	ethhdr->dest.addr[5]=0x00;

	ethhdr->src.addr[0]=0x40;
	ethhdr->src.addr[1]=netif->alias;
	ethhdr->src.addr[2]=0x00;
	ethhdr->src.addr[3]=0x00;
	ethhdr->src.addr[4]=0x00;
	ethhdr->src.addr[5]=0x00;


	//for(i=0;i<ETHBCP_HWADDR_LEN;i++)
		//ethhdr->src.addr[i]=netif->hwaddr[i];

	ethhdr->type=HTONS(type);
	//printf("ethbcp_output():send msg to tbim %d.\r\n",dst_alias);
	err=netif->linkoutput(netif,ethhdr_pbuf);
	if(ethhdr_pbuf != p)
	{
		pbuf_free(ethhdr_pbuf);
		ethhdr=NULL;
	}
	return err;
}


//����Ӳ��bit״̬
static void set_status_word_bad_frame_received()
{
	//#δ���#������Ӧ������Ӳ��bit״̬
}


//�����ҪFCSУ�飬��ɴ˺������Լ�����ӿ�
static u8t ethbcp_FCS_check()
{
	//#δ���#��У�麯��
	return 1;
}


//��֤���������Ƿ���ȷ,������ݴ�����ֱ��ɾ��
static u8t ethbcp_input_check(struct pbuf* p)
{
	struct ethbcp_hdr* bcp;
	bcp=(struct ethbcp_hdr*)p->data;

	if((bcp->eth_type!=0 || bcp->SSLength!=0 ||bcp->SSVersion!=0) && !ethbcp_FCS_check())//��׼�涨
	{
		SSNP_DEBUG_PRINT("ethbcp_input_check():incorrect data frame.\n");
		pbuf_free(p);
		set_status_word_bad_frame_received();//����Ӳ��bad frame received bit
		return 0;
	}

	return 1;
}


//#δ���#��ע�������bcp���պ�����TBC��TBIM�Ƿǳ���һ���ġ�
//���е�������Ϣ����ͨ���˺������մ���ġ�ע�������ethaddr��netif��Ӳ����ַ
static void ethbcp_input(struct netif* netif,struct eth_addr* ethaddr,struct pbuf* p)
{
	struct ethbcp_hdr* bcp;
	struct ass_alias_msg* assign_alias_msg;
	struct def_epoch_msg* define_epoch_msg;
	struct dcv_rpl_msg* discry_rpl_msg;
	u8t* msg_code;
	u8t alias;
	u8t* assigned_alias;
	u8t i;
	u16t *begin_timeslot;

	if(!ethbcp_input_check(p))//#δ���#������������еļ��
		return ;

	//��ִ�е�����Ŀ��Ա�֤���յ������ݵ���ȷ��
	bcp=(struct ethbcp_hdr*)p->data;
	msg_code=(u8t*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);

	if((*msg_code) < Initiate_discovery || (*msg_code) > Disable_transmitter)
	{   
		printf("prev is %d.\r\n",*(msg_code -4));
		printf("prev is %d.\r\n",*(msg_code -3));
		printf("prev is %d.\r\n",*(msg_code -2));
	    printf("error msg code is %d,prev is %d.\r\n",*msg_code,*(msg_code -1));
	printf("after is %d.\r\n",*(msg_code +1));
	printf("after is %d.\r\n",*(msg_code +2));
	printf("after is %d.\r\n",*(msg_code +3));
		SSNP_DEBUG_PRINT("ethbcp_input():incorrect bus management messages code.\r\n");
		pbuf_free(p);
		return ;
	}
	switch(*msg_code)
	{
//*******************************************TBIM���д���******************************************************************
#if NODE == TBIM

	case Initiate_discovery:
		{	 SSNP_DEBUG_PRINT("ethbcp_input():TBIM get discovery msg.\r\n");
			if(!tds.assign_alias_done)//���TBIM��û�б��������յ�����msg
			{
				tds.bcp_table[0].alias=bcp->eth_src_addr.addr[1];//�õ�TBC����
				for(i=0;i<ETHBCP_HWADDR_LEN;i++)             //�õ�TBC��mac��ַ�����ﲻ��Ҫ�����ֽ�˳�����⣬��Ϊ������1byteΪ��λ��
					tds.bcp_table[0].ethaddr.addr[i]=bcp->eth_src_addr.addr[i];
				tds.bcp_table[0].state=1;

				if(ethbcp_discover_reply(netif)!=ERR_OK)
				{
					SSNP_DEBUG_PRINT("ethbcp_input():send discovery reply failed.\r\n");
					break ;//����Ӧ��������
				}
				break;
			}
			else//TBIM�Ѿ������֣����账��
			{
				break;
			}

		}
	case Assign_alias:
		{  //SSNP_DEBUG_PRINT("ethbcp_input():TBIM get alias assign msg.\r\n");
			if(!tds.assign_alias_done)
			{
				assign_alias_msg=(struct ass_alias_msg*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
				if(!is_uuid_eq(&(assign_alias_msg->uid)))//���Ƿ��͸��˽ڵ�ı�����ֵ #δ���#������Ӧ���Ǳ������
					break;

				tds.alias=assign_alias_msg->alias;
				netif->alias=tds.alias;
				tds.new_mac_addr[0]=0x40;
				tds.new_mac_addr[1]=tds.alias;
				tds.new_mac_addr[2]=0x00;
				tds.new_mac_addr[3]=0x00;
				tds.new_mac_addr[4]=0x00;
				tds.new_mac_addr[5]=0x00;
				set_mac_address(tds.new_mac_addr);//��������mac��ַ
				
				if(ethbcp_assign_alias_respose(netif)!=ERR_OK)
				{
					SSNP_DEBUG_PRINT("ethbcp_input():TBIM send assign alias response msg failed.\n");
					break ;//����Ӧ��������
				}
				tds.assign_alias_done=1;//TBIM��������������
				break;
			}
			else//�������
			{
				assign_alias_msg=(struct ass_alias_msg*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
				if(is_alias_eq(assign_alias_msg->alias))
				{
					SSNP_DEBUG_PRINT("ethbcp_input():TBIM receive the same alias.\n");
					tds.assign_alias_done=0;//#δ���#��������ܻ���Ҫ�����������������TBIM��һЩ������
				}
				break;
			}
		}
	case Define_epoch:
	{  printf("ethbcp_input():TBIM recv define epoch msg.\r\n");
		if(tds.assign_alias_done && !tds.define_epoch_done)
		{
			define_epoch_msg=(struct def_epoch_msg*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
			tds.syn_itv=HTONS(define_epoch_msg->iso_itv);
			tds.asyn_itv=HTONS(define_epoch_msg->asy_itv);
			tds.define_epoch_done=1;
			break;
		}
		else
		{
			break;
		}
	}
	case Assign_timeslot:
	{	 printf("ethbcp_input():TBIM recv assign time slot msg.\r\n");
		if(tds.assign_alias_done && tds.define_epoch_done && !tds.begin_timeslot_done)
		{  /* #δ���#�� ע�����ﲢû�з���assign time slot reply*/
			begin_timeslot=(u16t*)((u8t*)p->data + SIZEOF_EHTBCP_HDR+3);
			tds.begin_timeslot=HTONS(*begin_timeslot);
			tds.begin_timeslot_done=1;
			break;
		}
		else
			break;
	}
	case Beginning_of_epoch:
	{	 printf("ethbcp_input():TBIM recv beginning of epoch msg.\r\n");
		if(!tds.begin_streaming_ok)
			tds.begin_streaming_ok=1;		
		break;
	}
	case Reflect:
	{
		//SSNP_DEBUG_PRINT("ethbcp_input():TBIM receive the reflect msg.\r\n");
		if(!tds.reflect_done)
			tds.reflect_done=1;
		break;
	}

#endif
//***********************************************************************************************************************
		
		
//*******************************************TBC���д���*****************************************************************
#if NODE == TBC
	case Discovery_reply:
		{
			//SSNP_DEBUG_PRINT("ethbcp_input():TBC recv discovery reply msg.\r\n");
			discry_rpl_msg=(struct dcv_rpl_msg*)((u8t*)p->data + SIZEOF_EHTBCP_HDR);
			alias=alloc_alias_map();//�õ�����
		//	printf("ethbcp_input():alloc alias is %d.",alias);
			if(add_alias_pending(alias,&discry_rpl_msg->uid))//���һ���������
				break;
			
	//		SSNP_DEBUG_PRINT("ethbcp_input():TBC send alias.\r\n");
			if(send_assign_alias_msg(netif,alias,&discry_rpl_msg->uid)!=ERR_OK)
				SSNP_DEBUG_PRINT("ethbcp_input():TBC send assign alias msg failed.\r\n");

			break;

		}
	case Assign_alias_response:
		{ 
			assigned_alias=(u8t*)((u8t*)p->data + SIZEOF_EHTBCP_HDR +1);//�������λ��
			//printf("ethbcp_input():TBC recv assign alias response msg from alias %d.\r\n",*assigned_alias);
			if(!is_exist_in_pending_list(*assigned_alias))//���������������û�д˱�������ô�������
			{
				printf("ethbcp_input():ERROR(unknown alias %d).\r\n",*assigned_alias);
				//SSNP_DEBUG_PRINT("ethbcp_input():ERROR(unknown alias).\r\n");
				break;
			}
			//printf("ethbcp_input():remove alias %d to done list.\r\n",*assigned_alias);
			alias_done(*assigned_alias,assigned_alias+1);
			break;
		}
	case Reflect_reply:
	{
		if(!tbc_dll_state.tbim_reflect_msg_done)
			tbc_dll_state.tbim_reflect_msg_done=1;
		break;
	}
#endif
//**********************************************************************************************************************
	default:
	{
		//#δ���#
			SSNP_DEBUG_PRINT("ethbcp_input():incorrect bcp type.\r\n");
	}
	}

	pbuf_free(p);
}


/**************************************************************************************************************
 * 
 * A ��̫���������ݺ�����������̫������֡,�������е�input����Ҫ�Լ�����pbuf
 * B ��̫��оƬ���Լ��Ĺ��˲��ԣ�������Ƿ����Լ�����̫��֡�ᱻ�����ģ��������յ��Ļ����ǹ㲥֡�����Ƿ��͵�
 *    �Լ�������
 **************************************************************************************************************/

err_t ethernet_input(struct pbuf* p,struct netif* netif)
{
	struct eth_hdr* ethhdr;
	u16t type;

	if(p->len <= SIZEOF_ETH_HDR)//ע�������ǲ�Ҫ�����ֽ�ת���ģ���Ϊ�������ڴ˽ڵ��Ϸ����pbuf�����ȵȶ�����ȷ��
	{
		SSNP_DEBUG_PRINT("ethernet_input():the ethernet frame is too small.\n");
		pbuf_free(p);
		return ERR_OK;//����һ�ɷ���ok����Ϊ����ʹ������ķ�����Ϣ��
	}
 
	ethhdr=(struct eth_hdr*)p->data;
	type=NTOHS(ethhdr->type);

	switch(type)
	{
	case ETHTYPE_BCP:
	{
		ethbcp_input(netif,(struct eth_addr*)(netif->hwaddr),p);
		break;
	}
	case ETHTYPE_DP:
	{	 
	//	printf("ethernet_input(): get  dp data. this tbim is %d\r\n",tds.alias);
		if(pbuf_header(p,-(s16t)SIZEOF_ETH_HDR))//ȥ����̫��ͷ����Ϣ
		{ 
			printf("ethernet_input():bad dp data delete it.\r\n");
			pbuf_free(p);
			p=NULL;
		}
		else
		{//	printf("ethernet_input():right dp data.\r\n");
			dp_input(p,netif);
		}
		break;
	}
	case ETHTYPE_SDP:
	{
		if(pbuf_header(p,-(s16t)SIZEOF_ETH_HDR))//ȥ����̫��ͷ����Ϣ
		{
			pbuf_free(p);
			p=NULL;
		}
		else
		{
			sdp_input(p,netif);
		}
		break;
	}
	case ETHTYPE_TP:
	{  
		SSNP_DEBUG_PRINT("ethernet_input():get tp type data,\r\n");
		if(pbuf_header(p,-(s16t)SIZEOF_ETH_HDR))//ȥ����̫��ͷ����Ϣ
		{
			pbuf_free(p);
			p=NULL;
		}
		else
		{
			SSNP_DEBUG_PRINT("ethernet_input():get right tp data,\r\n");
			tp_input(p,netif);
		}
		break;
	}
	default:
	{
		SSNP_DEBUG_PRINT("ethernet_input():incorrect ethernet type.\r\n");
		pbuf_free(p);
	}
	}
	return ERR_OK;
}

//��̫����������أ�ʹ��ethbcp_output���������


