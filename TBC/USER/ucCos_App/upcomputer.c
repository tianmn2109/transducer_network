#include "upcomputer.h"
void send_upcomputer(char* str,int len)
{	
	int i=0;
	for(i=0;i<len;i++)
	{
		USART_SendData(USART2,str[i]);
		while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);		
	}
}			
void send_sdp_upcomputer(struct netbuf* buf)
{
	struct tdcn_value tv;
	char* data;
	tv.type=3+'0';
	tv.tbim=buf->alias;
	tv.num=buf->tdcn_num;
	data=(char*)buf->p->data;
	tv.data[0]=data[0];
	tv.data[1]=data[1];
	tv.data[2]=data[2];
	tv.data[3]=data[3];
	send_upcomputer((char*)&tv,sizeof(struct tdcn_value));
}
void send_TEDS_upcomputer(char* str,char tbim)
{
	char data[34];
	char upcpt[4];
	int i;
	data[0]=2+'0';
	data[1]=tbim;
	upcpt[0]=2+'0';
	upcpt[1]=tbim;

	for(i=2;i<34;i++)
		data[i]=str[i-2];
	//send_upcomputer(data,34);
	for(i=0;i<16;i++)
	{
		upcpt[2]=data[2+i*2];
		upcpt[3]=data[2+i*2+1];
	//	printf("%d,%d,%d,%d\r\n",upcpt[0],upcpt[1],data[2+i*2],data[2+i*2+1]);
	//	printf
		//
		send_upcomputer(upcpt,4);
	}
}

void init_queue(void)
{
	queue.head=0;
	queue.tail=0;
	queue.count=0;
}

u8t isQempty(void)
{
	return queue.count==0;
}

u8t isQfull(void)
{
	return queue.head==(queue.tail+1)%SIZE;
}
u8t enQueue(struct cmd_item item)
{
	if(isQfull())
	{
		printf("Q full.\r\n");
		return 0;
	}
	++queue.count;
	queue.queue[queue.tail]=item;

	//printf("enQ,%d\r\n",queue.count);
	queue.tail=(queue.tail+1)%SIZE;
	return 1;
}
struct cmd_item deQueue(u8t* ret)
{
	struct cmd_item item;
	item.alias=0;
	item.tdcn=0;
	item.cmd_class=0;
	item.cmd_func=0;
	if(isQempty())
	{
		//printf("Q empty.\r\n");
		*ret=0;
		return item;
	}
	item=queue.queue[queue.head];
	queue.head=(queue.head+1)%SIZE;
	*ret=1;
	//printf("deQ,%d\r\n",queue.count);
	--queue.count;
	return item;
}
u8t has_upcomputer_cmd(void)
{
	return queue.count;
}