#ifndef TIMER_H
#define TIMER_H

#include "err.h"
#include "sys.h"

//超时回调函数原型
typedef void (*sys_timeout_handler)(void* arg);

struct sys_timeo
{
	struct sys_timeo* next;
	u32t time;
	sys_timeout_handler h;
	void* arg;
};
#if NODE == TBC
struct netif;//原型声明
void discovery_timer(void* arg);
void alias_check_timer(void* arg);
#endif

void sys_timeouts_init(void);
void sys_timeout(u32t msecs,sys_timeout_handler handler,void* arg);
void sys_untimeout(sys_timeout_handler handler,void* arg);
void sys_timeouts_mbox_fetch(sys_mbox_t mbox,void** msg);

#endif
