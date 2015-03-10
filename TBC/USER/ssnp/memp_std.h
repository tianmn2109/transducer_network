//���ﲻ���Լ����ر����

#ifndef SSNP_PBUF_MEMPOOL
#define SSNP_PBUF_MEMPOOL(name,num,payload,desc) SSNP_MEMPOOL(name,num,(MEMP_ALIGN_SIZE(sizeof(struct pbuf))+MEMP_ALIGN_SIZE(payload)),desc)
#endif



//SSNP_MEMPOOL(RAW_PCB,          MEMP_NUM_RAW_PCB,         /*sizeof(struct raw_pcb)*/10,        "RAW_PCB")//#δ���#������Ĺ���
SSNP_MEMPOOL(DP_PCB,           MEMP_NUM_DP_PCB,          sizeof(struct  dp_pcb),                                        "DP_PCB")
SSNP_MEMPOOL(SDP_PCB,          MEMP_NUM_SDP_PCB,         sizeof(struct sdp_pcb),                                        "SDP_PCB")
SSNP_MEMPOOL(TP_PCB,           MEMP_NUM_TP_PCB,          sizeof(struct tp_pcb),                                         "TP_PCB")
SSNP_MEMPOOL(TP,               MEMP_NUM_TP,              (sizeof(struct tp_hdr) + PBUF_LINK_HLEN + sizeof(struct pbuf)),"TP")

SSNP_MEMPOOL(NETBUF,           MEMP_NUM_NETBUF,          sizeof(struct netbuf),                                         "NETBUF")
SSNP_MEMPOOL(NETCONN,          MEMP_NUM_NETCONN,         sizeof(struct netconn),                                        "NETCONN")
SSNP_MEMPOOL(NETPBUF_PAD,      MEMP_NUM_NETPBUF_PAD,     (sizeof(struct pbuf) + 3),                                     "NETPBUF_PAD")	

SSNP_MEMPOOL(SSNP_MSG_API,     MEMP_NUM_SSNP_MSG_API,     sizeof(struct ssnp_msg),                                      "TCPIP_MSG_API")
SSNP_MEMPOOL(SSNP_MSG_INPKT,   MEMP_NUM_SSNP_MSG_INPKT,  sizeof(struct ssnp_msg),                                       "SSNP_MSG_INPKT")

SSNP_MEMPOOL(SYS_TIMEOUT,      MEMP_NUM_SYS_TIMEOUT,     sizeof(struct sys_timeo),                                      "SYS_TIMEOUT")

SSNP_PBUF_MEMPOOL(PBUF,        MEMP_NUM_PBUF,                    0,   "PBUF_REF/ROM")//MEMP_NUM_PBUF��(16)ͷ����С(pbuf��С) ������С����pbuf��С
SSNP_PBUF_MEMPOOL(PBUF_POOL,   PBUF_POOL_SIZE,   PBUF_POOL_BUFSIZE,      "PBUF_POOL")//PBUF_POOL_SIZE��(16)�ڴ�أ�������СΪ���Э������+pbuf��С
                                                                                     //#δ���#�������bufsize������Ҫ��ô��
#undef SSNP_MEMPOOL
#undef SSNP_PBUF_MEMPOOL

