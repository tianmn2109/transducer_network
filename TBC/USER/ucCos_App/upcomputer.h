#ifndef __UPCOMPUTER__
#define __UPCOMPUTER__
#include "commondefine.h"
#include "ssnp/netbuf.h"
#include  <stm32f10x.h>

#define SIZE 10
struct cmd_item
{	
	u8t alias;
	u8t tdcn;
	u8t cmd_class;
	u8t cmd_func;
};

struct cmd_queue
{
	u8t count;
	u8t head;
	u8t tail;
	struct cmd_item queue[SIZE];
};

static struct cmd_queue queue;

void init_queue(void);
u8t has_upcomputer_cmd(void);
u8t isQempty(void);
u8t isQfull(void);
u8t enQueue(struct cmd_item item);
struct cmd_item deQueue(u8t* ret);


void send_sdp_upcomputer(struct netbuf* buf);
void send_upcomputer(char* str,int len);
void send_TEDS_upcomputer(char* str,char tbim);

#endif 
