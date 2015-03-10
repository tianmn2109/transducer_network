#ifndef PBUF_H
#define PBUF_H

#include "opt.h"
#include "err.h"
//#δ���#����Ϊ���ﻹ��̫���������ôʹ�ã����������ʱ���䣬���ǰ���ԭ����д
#define PBUF_LINK_HLEN                      14
#define PBUF_IP_HLEN                        10  //���Ҳ�ǲ���Ҫ��
#define PBUF_IP_AND_TRANSPORT_HLEN          10 //ע�����ﰴ��������
typedef enum
{
	PBUF_TRANSPORT,
	PBUF_IP,
	PBUF_LINK,
	PBUF_RAW
}pbuf_layer;

typedef enum
{
	PBUF_RAM,
	PBUF_ROM,
	PBUF_REF,
	PBUF_POOL
}pbuf_type;

struct pbuf
{
	struct pbuf* next;
	void* data ;
	u16t tot_len;
	u16t len;
	u8t type;
	u8t flags;
	u16t ref;
};

typedef void (*pbuf_free_custom_fn)(struct pbuf* p);

struct pbuf_custom
{
	struct pbuf pbuf;
	pbuf_free_custom_fn custom_free_function;
};

#define pbuf_init()

struct pbuf* pbuf_alloc(pbuf_layer layer,u16t length,pbuf_type type);
void pbuf_realloc(struct pbuf* p,u16t new_len);
u8t pbuf_header(struct pbuf* p,s16t header_size_increment);
void pbuf_ref(struct pbuf* p);
//void pbuf_ref_chain(struct pbuf* p);�˺���û��ʵ��
u8t pbuf_free(struct pbuf* p);
u8t pbuf_clen(struct pbuf* p);
void pbuf_cat(struct pbuf* head,struct pbuf* tail);
void pbuf_chain(struct pbuf* head,struct pbuf* tail);
struct pbuf* pbuf_dechain(struct pbuf* p);
err_t pbuf_copy(struct pbuf* p_to,struct pbuf* p_from);
u16t pbuf_copy_partical(struct pbuf* buf,void* dataptr,u16t len,u16t offset);
err_t pbuf_take(struct pbuf* buf,const void* dataptr,u16t len);
struct pbuf* pbuf_coalesce(struct pbuf* p,pbuf_layer layer);


#endif
