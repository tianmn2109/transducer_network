#ifndef SSNP_OPT_H
#define SSNP_OPT_H

/*
   -----------------------------------------------
   ----------        平台类型声明     ----------
   -----------------------------------------------
*/
#include "stdio.h"
typedef unsigned   char      u8t;       //将来会改动的地方
typedef signed     char      s8t;
typedef unsigned   short     u16t;     //将来会改动的地方
typedef signed     short     s16t;
typedef unsigned   int       u32t;       //将来会改动的地方
typedef int                  s32t;
typedef float                f32;

/*
   -----------------------------------------------
   ----------        系统临界区保护     ----------
   -----------------------------------------------
*/
#if OS_CRITICAL_METHOD == 1
#define SYS_ARCH_DECL_PROTECT(lev)
#define SYS_ARCH_PROTECT(lev)		CPU_INT_DIS()
#define SYS_ARCH_UNPROTECT(lev)		CPU_INT_EN()
#endif

#if OS_CRITICAL_METHOD == 3  //method 3 is used in this port.
#define SYS_ARCH_DECL_PROTECT(lev)	u32t lev
#define SYS_ARCH_PROTECT(lev)		lev = OS_CPU_SR_Save()
#define SYS_ARCH_UNPROTECT(lev)		OS_CPU_SR_Restore(lev)
#endif
/*
   -----------------------------------------------
   ----------        内存对齐    ----------
   -----------------------------------------------
*/

#if defined (__ICCARM__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES

#elif defined (__CC_ARM)

#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__GNUC__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__TASKING__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif



#include "ssnpopt.h"
/*
   -----------------------------------------------
   ----------    协议控制信息字段长度     ----------
   -----------------------------------------------
*/

#define MEMP_NUM_RAW_PCB 6
#define MEMP_NUM_DP_PCB  6
#define MEMP_NUM_SDP_PCB 6
#define MEMP_NUM_TP_PCB  6
/**
 * MEMP_NUM_NETBUF: the number of struct netbufs.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#ifndef MEMP_NUM_NETBUF
#define MEMP_NUM_NETBUF                 20 //这里有三种连接dp，sdp，trigger
#endif



/*
 * MEMP_NUM_NETCONN: the number of struct netconns.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#ifndef MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN                4
#endif

/*
 * MEMP_NUM_SSNP_MSG_INPKT: the number of struct ssnp_msg, which are used
 * for incoming packets. 
 * (only needed if you use ssnp.c)
 */
#ifndef MEMP_NUM_SSNP_MSG_INPKT
#define MEMP_NUM_SSNP_MSG_INPKT        8
#endif


/**
 * MEMP_NUM_SYS_TIMEOUT: the number of simulateously active timeouts.
 * (requires NO_SYS==0)
 */
#ifndef MEMP_NUM_SYS_TIMEOUT
#define MEMP_NUM_SYS_TIMEOUT            3
#endif

/*
 * MEMP_NUM_SSNP_MSG_API: the number of struct ssnp_msg, which are used
 * for callback/timeout API communication. 
 * (only needed if you use ssnp.c)
 */
#ifndef MEMP_NUM_SSNP_MSG_API
#define MEMP_NUM_SSNP_MSG_API          8
#endif



//下面是默认值的宏定义，可以在ssnpopt.h中修改默认值


/*
 * MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high.
 */
#ifndef MEMP_NUM_PBUF
#define MEMP_NUM_PBUF                   16
#endif

//dp和sdp后面填充的数据
#ifndef MEMP_NUM_NETPBUF_PAD
#define MEMP_NUM_NETPBUF_PAD            16
#endif

//PBUF_POOL_SIZE: the number of buffers in the pbuf pool. 
#ifndef PBUF_POOL_SIZE
#define PBUF_POOL_SIZE                  16  
#endif
//trigger协议数据个数
#ifndef MEMP_NUM_TP
#define MEMP_NUM_TP                    16
#endif 
//
#ifndef MSS
#define MSS                   600//#未完成#：最大值设置
#endif

#ifndef PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE               SSNP_MEM_ALIGN_SIZE(MSS+10+4+PBUF_LINK_HLEN)
#endif



//如果有较好的内存copy策略可以替换掉
#ifndef MEMCPY
#define MEMCPY(dst,src,len) memcpy(dst,src,len)
#endif





#ifndef MEM_ALIGNMENT 
#define MEM_ALIGNMENT     4          //将来会改动的地方
#endif 

#ifndef MEM_SIZE
#define MEM_SIZE      1600       //可用内存的大小
#endif



/*
   -----------------------------------------------
   ----------    TBIM个数定义           ----------
   -----------------------------------------------
*/
#define TBIM_NUM  32

/*
   -----------------------------------------------
   ----------    超时时间间隔           ----------
   -----------------------------------------------
*/
#define DISCOVERY_TMR_INTERVSL        500 //每隔500ms发送一次发现msg
#define NORESPONSE_TMR_INTERVSL       3000 //如果为TBIM分配别名后没有响应那么超过3s后就回收此别名
#define ALIASCHECK_TMR_INTERVSL       2000 //每隔2s检测一次，回收没有响应的TBIM
#define TBC_ALIASALLOC_WAIT_INTERVSL  3000 //TBC应用层等待3s
#endif
