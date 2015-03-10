#ifndef NETBUF_H
#define NETBUF_H

#include "opt.h"
#include "pbuf.h"
#include "err.h"

struct netbuf
{
	struct pbuf *p;
	u8t alias;//对方的别名
	u8t tdcn_num;//对方的端口号
	
	u8t tbim_tdcn_num;//自身的端口号
};

struct netbuf* netbuf_new(void);
void* netbuf_alloc(struct netbuf* buf,u16t size);
void netbuf_delete(struct netbuf* buf);
void netbuf_free(struct netbuf* buf);
err_t netbuf_ref(struct netbuf* buf,const void* dataptr, u16t size);
err_t netbuf_data(struct netbuf* buf,void** dataptr,u16t* len);



#endif
