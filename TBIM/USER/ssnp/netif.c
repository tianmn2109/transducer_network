#include "netif.h"
#include "debug.h"

struct netif* netif_list;
struct netif* netif_default;

static u8t netif_num;//序号

/*
 *  注意：这里只是用一个netif。
 */

void netif_init()
{
	//不需要做什么
}

//向网络接口链表中增加一项
struct netif* netif_add(struct netif* netif,u8t alias,void* state,netif_init_fn init,netif_input_fn input)
{
	SSNP_ASSERT("netif_add():no init function given\n",init!=NULL);

	netif->flags=0;
	netif->alias=alias;
	netif->state=state;
	netif->num=netif_num++;//序号
	netif->input=input;

	if(init(netif)!=ERR_OK)
		return NULL;

	netif->next=netif_list;
	netif_list=netif;

	return netif;
}

void netif_set_addr(struct netif* netif,u8t alias)
{
	netif->alias=alias;
}

//由于ssnp没有向snmp等其他协议所以这里就非常简单了
void netif_remove(struct netif* netif)
{
	struct netif* i;

	if(netif == NULL)
		return ;

	if(netif_list == netif)//第一个就是需要找到的netif
	{
		netif_list=netif->next;
	}
	else
	{
		for(i=netif_list; i!=NULL;i=i->next)
		{
			if(i->next == netif)
			{
				i->next=netif->next;
				break;
			}
		}
		if(i==NULL)//没有找到
			return ;
	}

}

//按照名字进行查找
struct netif* netif_find(char* name)
{
	struct netif* i;
	u8t num;

	if(name==NULL)
		return NULL;
	num=name[2]-'0';

	for(i=netif_list; i!=NULL; i=i->next)
	{
		if(num==i->num && name[0]==i->name[0] && name[1]==i->name[1])
			return i;
	}
	return NULL;
}


//将此网络接口设置为默认接口
void netif_set_default(struct netif* netif)
{
	netif_default=netif;
}

