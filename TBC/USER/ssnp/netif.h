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
typedef err_t (* netif_output_fn)(struct netif* netif,struct pbuf* p,u8t dst_alias,u16t type);//此函数发送以太网帧
typedef err_t (* netif_linkoutput_fn)(struct netif* netif,struct pbuf* p);//最底层的发送函数和驱动相关，上面的发送函数就是调用此函数来实现的

struct netif
{
	struct netif* next;

	//当底层驱动接收的数据后使用此回调函数向上层协议提交数据，接收到数据后放入p，inp是接收到数据的网络接口
	netif_input_fn input;
	
	//这里的函数是由数据报、数据流和trigger协议发送函数时调用的，同时这里的addr就是直接的mac，不需要地址解析，而发送是调用的链路层发送函数实现的
    netif_output_fn output;

	//数据链路层的发送函数，所有的数据发送实际上都是调用此函数发送的。用于不需要上层应用参与的数据发送比如开始的地址分配就需要利用此函数。
	netif_linkoutput_fn linkoutput;

	//状态位，指向设备的状态信息。
	void* state;

	//TBIM别名,类似于ip地址
	u8t alias;

	//变送器号 #未完成#：这里可能并不需要变送器号，因为这里是看不到变送器内部的传感器号的，只能看到TBIM
	u8t tdcn_num;

	//最大网络传输单元
	u16t mtu;

	//硬件地址长度
	u8t hwaddr_len;

	//硬件地址
	u8t hwaddr[NETIF_MAX_HWADDR_LEN];

	//网卡功能使能位
	u8t flags;

	//网络接口种类
	char name[2];

	//网络接口数量
	u8t num;
};

//网络接口链表
extern struct netif* netif_list;

//默认网络接口
extern struct netif* netif_default;

//初始化函数
void netif_init(void);

//向网络接口链表中增加网络接口
struct netif* netif_add(struct netif* netif,u8t alias,void* state,netif_init_fn init,netif_input_fn input);

//设置网络接口地址
void netif_set_addr(struct netif* netif,u8t alias);

//网络链表中移除网络接口
void netif_remove(struct netif* netif);

//struct netif* ip_route(u8t alias);
struct netif* netif_find(char* name);
void netif_set_default(struct netif* netif);


//void netif_set_up(struct netif* netif);这些函数都没有实现
//void netif_set_down(struct netif* netif);
//void netif_set_link_up(struct netif* netif);
//void netif_set_link_down(struct netif* netif);

#endif


