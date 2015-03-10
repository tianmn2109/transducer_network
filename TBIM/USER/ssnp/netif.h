#ifndef NETIF_H
#define NETIF_H

#include "opt.h"
#include "err.h"
#include "pbuf.h"
#include "ethbcp.h"


#define NETIF_MAX_HWADDR_LEN 6

struct netif;

typedef err_t (* netif_init_fn)(struct netif *netif);
typedef err_t (* netif_input_fn)(struct pbuf* p,struct netif* inp);
typedef err_t (* netif_output_fn)(struct netif* netif,struct pbuf* p,u8t dst_alias,u16t type);//�˺���������̫��֡
typedef err_t (* netif_linkoutput_fn)(struct netif* netif,struct pbuf* p);//��ײ�ķ��ͺ�����������أ�����ķ��ͺ������ǵ��ô˺�����ʵ�ֵ�

struct netif
{
	struct netif* next;

	//���ײ��������յ����ݺ�ʹ�ô˻ص��������ϲ�Э���ύ���ݣ����յ����ݺ����p��inp�ǽ��յ����ݵ�����ӿ�
	netif_input_fn input;
	
	//����ĺ����������ݱ�����������triggerЭ�鷢�ͺ���ʱ���õģ�ͬʱ�����addr����ֱ�ӵ�mac������Ҫ��ַ�������������ǵ��õ���·�㷢�ͺ���ʵ�ֵ�
    netif_output_fn output;

	//������·��ķ��ͺ��������е����ݷ���ʵ���϶��ǵ��ô˺������͵ġ����ڲ���Ҫ�ϲ�Ӧ�ò�������ݷ��ͱ��翪ʼ�ĵ�ַ�������Ҫ���ô˺�����
	netif_linkoutput_fn linkoutput;

	//״̬λ��ָ���豸��״̬��Ϣ��
	void* state;

	//TBIM����,������ip��ַ
	u8t alias;

	//�������� #δ���#��������ܲ�����Ҫ�������ţ���Ϊ�����ǿ������������ڲ��Ĵ������ŵģ�ֻ�ܿ���TBIM
	u8t tdcn_num;

	//������紫�䵥Ԫ
	u16t mtu;

	//Ӳ����ַ����
	u8t hwaddr_len;

	//Ӳ����ַ
	u8t hwaddr[NETIF_MAX_HWADDR_LEN];

	//��������ʹ��λ
	u8t flags;

	//����ӿ�����
	char name[2];

	//����ӿ�����
	u8t num;
};

//����ӿ�����
extern struct netif* netif_list;

//Ĭ������ӿ�
extern struct netif* netif_default;

//��ʼ������
void netif_init(void);

//������ӿ���������������ӿ�
struct netif* netif_add(struct netif* netif,u8t alias,void* state,netif_init_fn init,netif_input_fn input);

//��������ӿڵ�ַ
void netif_set_addr(struct netif* netif,u8t alias);

//�����������Ƴ�����ӿ�
void netif_remove(struct netif* netif);

//struct netif* ip_route(u8t alias);
struct netif* netif_find(char* name);
void netif_set_default(struct netif* netif);


//void netif_set_up(struct netif* netif);��Щ������û��ʵ��
//void netif_set_down(struct netif* netif);
//void netif_set_link_up(struct netif* netif);
//void netif_set_link_down(struct netif* netif);

#endif


