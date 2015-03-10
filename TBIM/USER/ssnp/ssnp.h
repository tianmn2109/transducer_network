#ifndef SSNP_H
#define SSNP_H

#include "api_msg.h"
#include "timer.h"


#define SSNP_APIMSG(m)      ssnp_apimsg(m)
#define SSNP_APIMSG_ACK(m)  sys_sem_signal(m->conn->op_completed)

enum ssnp_msg_type
{
	SSNP_MSG_API,
	SSNP_MSG_INPKT,
	SSNP_MSG_TIMEOUT,
	SSNP_MSG_UNTIMEOUT
};

struct ssnp_msg
{
	enum ssnp_msg_type type;
	//sys_sem_t* sem;
	union
	{
		struct api_msg* apimsg;
		struct
		{
			struct pbuf* p;
			struct netif* netif;
		}inp;
		struct
		{
			u32t msecs;
			sys_timeout_handler h;
			void* arg;
		}tmo;
	}msg;
};

typedef void (*ssnp_init_done_fn)(void* arg);

void ssnp_init(ssnp_init_done_fn initfunc,void* arg);
err_t ssnp_apimsg(struct api_msg* apimsg);
err_t ssnp_input(struct pbuf* p,struct netif* inp);
err_t ssnp_timeout(u32t msecs,sys_timeout_handler h,void* arg);
err_t ssnp_untimeout(sys_timeout_handler h,void* arg);

#if NODE == TBIM
err_t ssnp_apimsg_alias(struct api_msg* apimsg);
#endif

#if NODE == TBC
err_t ssnp_apimsg_alias_list(struct api_msg* apimsg);
#endif
#endif

