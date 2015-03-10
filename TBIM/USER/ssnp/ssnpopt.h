#ifndef SSNPOP_H
#define SSNPOP_H
 #include "config.h" 
#define MEM_ALIGNMENT           4
#define MEM_SIZE                (10*1024)
#define MEMP_NUM_PBUF           10

#if NODE == TBC
//#define PBUF_POOL_SIZE          20
//#define PBUF_POOL_BUFSIZE       750
#define PBUF_POOL_SIZE          80
#define MEMP_NUM_NETBUF         80 	
#define PBUF_POOL_BUFSIZE       150
#define MEMP_NUM_SSNP_MSG_INPKT        30
#endif

#if NODE == TBIM
#define PBUF_POOL_SIZE          10
#define PBUF_POOL_BUFSIZE       1500
#endif

#define DEFAULT_DP_RECVMBOX_SIZE    2
#define DEFAULT_SDP_RECVMBOX_SIZE    MAX_QUEUE_ENTRIES
//#define DEFAULT_SDP_RECVMBOX_SIZE    2
#define DEFAULT_TP_RECVMBOX_SIZE    2

#define MSS                 (1500 - 40)	  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

#endif
