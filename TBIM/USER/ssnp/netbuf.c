
#include "netbuf.h"
#include "memp.h"

#define PAD_LEN   1
#define CHK_LEN   2
struct netbuf* netbuf_new()
{
	struct netbuf* buf;
	buf=(struct netbuf*)memp_alloc(MEMP_NETBUF);
	if(buf != NULL)
	{
		buf->p=NULL;
		buf->alias=0;
		buf->tdcn_num=0;
		return buf;
	}
	else
	{
		return NULL;
	}

}
void* netbuf_alloc(struct netbuf* buf,u16t size)
{
	if(buf->p != NULL)
		pbuf_free(buf->p);
	

	buf->p = pbuf_alloc(PBUF_TRANSPORT,size, PBUF_RAM);
	if(buf->p == NULL)
		return NULL;
	
	return buf->p->data;
}
void netbuf_delete(struct netbuf* buf)
{
	if(buf != NULL)
	{
		if(buf->p != NULL)
		{
			pbuf_free(buf->p);
			buf->p=NULL;
		}
		memp_free(MEMP_NETBUF,buf);
	}
}
void netbuf_free(struct netbuf* buf)
{
	if(buf->p != NULL)
		pbuf_free(buf->p);
	buf->p=NULL;
}

err_t netbuf_ref(struct netbuf* buf,const void* dataptr, u16t size)
{	//printf("netbuf_ref():here\r\n");
	if(buf->p != NULL)
		pbuf_free(buf->p);
	//printf("netbuf_ref():allocing new pbuf\r\n");
	buf->p=pbuf_alloc(PBUF_TRANSPORT,0,PBUF_REF);
	if(buf->p == NULL)
		return ERR_MEMORY;
   	//printf("netbuf_ref():alloc new pbuf done.\r\n");
	buf->p->data=(void*)dataptr;
	buf->p->tot_len=size;
	buf->p->len=size;

	return ERR_OK;
}


err_t netbuf_data(struct netbuf* buf,void** dataptr,u16t* len)
{
	if(buf->p == NULL)
		return ERR_BUFFER;
	*dataptr=buf->p->data;
	*len=buf->p->len;
	return ERR_OK;
}