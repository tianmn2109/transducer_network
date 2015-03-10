#ifndef BCPMSG_H
#define BCPMSG_H

#include "sys.h"
#include "opt.h"
#define PACK_BEGIN  #ifdef PACK_STRUCT_USE_INCLUDES \
	                #include "bpstruct.h" \
                    #endif

#define PACK_END    #ifdef PACK_STRUCT_USE_INCLUDES \
	                #include "epstruct.h" \
                    #endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct uuid
{
	u8t uid[10];
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

struct dcv_msg     //������Ϣ
{
	u8t type;
};



#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct dcv_rpl_msg //���ֻظ���Ϣ
{
	u8t type;
	struct uuid uid;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif


#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct ass_alias_msg //����������Ϣ
{
	u8t type;
	struct uuid uid;
	u8t alias;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct ass_alias_rps_msg //����������Ӧ��Ϣ
{
	u8t type;
	u8t alias;
	u8t flag;
	u8t payload;
	f32 start_delay;
	f32 reflect_delay;
	f32 reflect_delay_uctn;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct ass_timeslot_msg// ʱ��۷�����Ϣ
{
	u8t type;
	u8t alias;
	u8t tbim_tdcn_num;
	u16t bgn_timeslot;
	u8t slot_num;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct ass_timeslot_rpl_msg//��������ظ���Ϣ
{
	u8t type;
	u8t status;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct def_epoch_msg //����ʱ��Ƭ��Ϣ
{
	u8t type;
	u16t iso_itv;
	u16t asy_itv;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct bgn_epoch_msg_hdr//ʱ��Ƭ��ʼmsg�ײ�
{
	u8t type;
	u16t epoch_num_next;
	u8t rtran_num;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct start_asy_msg//�첽��ʼmsg
{
	u8t type;
	s32t time_error;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif

#ifdef PACK_STRUCT_USE_INCLUDES 
#include "bpstruct.h" 
#endif
PACK_STRUCT_BEGIN
struct rfl_msg//reflect msg
{
	u8t type;
	u8t alias;
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES 
#include "epstruct.h" 
#endif
#endif
