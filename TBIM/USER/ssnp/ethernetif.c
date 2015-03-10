#include "sys.h"
#include "ethernetif.h"
#include "debug.h"
#include "mem.h"
#include "ethbcp.h"
#include <includes.h>


u8t mac_address[NETIF_MAX_HWADDR_LEN];//mac地址

#define IFNAME0 'h'
#define IFNAME1 'w'

#define  ETH_DMARxDesc_FrameLengthShift           (         16)
#define  ETH_ERROR                                ((uint32_t)0)
#define  ETH_SUCCESS                              ((uint32_t)1)

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint32_t length;
  uint32_t buffer;
  ETH_DMADESCTypeDef *descriptor;
}
FrameTypeDef;

/* Private define ------------------------------------------------------------*/
#define ETH_RXBUFNB        4    /* DMA接收缓冲区数目 */
#define ETH_TXBUFNB        2    /* DMA发送缓冲区数目 */
/* Private variables ---------------------------------------------------------*/
uint8_t MACaddr[6];
ETH_DMADESCTypeDef  DMARxDscrTab[ETH_RXBUFNB], DMATxDscrTab[ETH_TXBUFNB];/* Ethernet Rx & Tx DMA Descriptors 以太网接收发送DMA描述符*/
uint8_t Rx_Buff[ETH_RXBUFNB][ETH_MAX_PACKET_SIZE], Tx_Buff[ETH_TXBUFNB][ETH_MAX_PACKET_SIZE];/* Ethernet buffers 以太网接收发送缓冲区*/

extern ETH_DMADESCTypeDef  *DMATxDescToSet;
extern ETH_DMADESCTypeDef  *DMARxDescToGet;

/* Private function prototypes -----------------------------------------------*/
uint32_t ETH_GetCurrentTxBuffer(void); 
FrameTypeDef ETH_RxPkt_ChainMode(void);
uint32_t ETH_TxPkt_ChainMode(u16 FrameLength);


//辅助信息
struct ethernetif
{
	struct eth_addr* ethaddr;
};



//设置硬件mac地址
void set_mac_address(u8t* mac_addr)
{
	u8t i;
	for(i=0;i<NETIF_MAX_HWADDR_LEN;i++)
		mac_address[i]=mac_addr[i];


	ETH_MACAddressConfig(ETH_MAC_Address0,mac_addr);
}

//硬件相关的初始化工作
static void low_level_init(struct netif* netif)
{
	u8t i;
	SYS_ARCH_DECL_PROTECT(sr);

	netif->hwaddr_len=NETIF_MAX_HWADDR_LEN;

	for(i=0;i<NETIF_MAX_HWADDR_LEN;i++)
		netif->hwaddr[i]=mac_address[i];

	//maximum transfer unit
	netif->mtu=1500;//因为这里使用以太网发送数据的因此必须遵循以太网的标准


	SYS_ARCH_PROTECT(sr);
	/* Initialize Tx Descriptors list: Chain Mode */
  ETH_DMATxDescChainInit(DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);
  /* Initialize Rx Descriptors list: Chain Mode  */
  ETH_DMARxDescChainInit(DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);
	
	 /* Enable Ethernet Rx interrrupt */
	for(i=0; i<ETH_RXBUFNB; i++)
	{
		ETH_DMARxDescReceiveITConfig(&DMARxDscrTab[i], ENABLE);
	}
	SYS_ARCH_UNPROTECT(sr);
  
  /* Enable MAC and DMA transmission and reception */
  ETH_Start();
}

//packet的发送是通过此函数完成的。
static err_t low_level_output(struct netif* netif,struct pbuf* p)
{
  struct pbuf *q;
  int l = 0;
  uint8_t *buffer = NULL;

  SYS_ARCH_DECL_PROTECT(sr);
  /* Interrupts are disabled through this whole thing to support multi-threading
	   transmit calls. Also this function might be called from an ISR. */
  SYS_ARCH_PROTECT(sr);

  buffer = (uint8_t *)ETH_GetCurrentTxBuffer();
  
  for(q = p; q != NULL; q = q->next) 
  {
    memcpy((u8t*)&buffer[l], q->data, q->len);
	l = l + q->len;
  }

  ETH_TxPkt_ChainMode(l);

  SYS_ARCH_UNPROTECT(sr);

  return ERR_OK;
}

//packet的实际接收是通过此函数实现的。
static struct pbuf* low_level_input(struct netif* netif)
{
  struct pbuf *p, *q;
  uint16_t len;
  int l =0;
  FrameTypeDef frame;
  uint8_t *buffer;
  
  p = NULL;
  frame = ETH_RxPkt_ChainMode();
  /* Obtain the size of the packet and put it into the "len"
     variable. */
  len = frame.length;
  
  buffer = (uint8_t *)frame.buffer;

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);//注意这里可以保证只需要一个pbuf就可以容纳所有的数据，因为接收pbuf是按照最大数据帧设计的

  if (p != NULL)
  {
    for (q = p; q != NULL; q = q->next)
    {
	  memcpy((u8t*)q->data, (u8t*)&buffer[l], q->len);
      l = l + q->len;
    }    
  }
  //else
  //	SSNP_DEBUG_PRINT("low_level_input():alloc pbuf is NULL.\r\n");
  	

  /* Set Own bit of the Rx descriptor Status: gives the buffer back to ETHERNET DMA */
  frame.descriptor->Status = ETH_DMARxDesc_OWN; 
 
  /* When Rx Buffer unavailable flag is set: clear it and resume reception */
  if ((ETH->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET)  
  {
    /* Clear RBUS ETHERNET DMA flag */
    ETH->DMASR = ETH_DMASR_RBUS;
    /* Resume DMA reception */
    ETH->DMARPDR = 0;
  }
  return p;
}


//接收数据中断处理函数调用此函数来对数据的处理
err_t ethernetif_input(struct netif* netif)
{
	err_t err;
	struct pbuf* p;

  SYS_ARCH_DECL_PROTECT(sr);
  
  SYS_ARCH_PROTECT(sr);
  /* move received packet into a new pbuf */
  p = low_level_input(netif);
  SYS_ARCH_UNPROTECT(sr);

	if(p==NULL)
	{
		//SSNP_DEBUG_PRINT("ethernetif_input():pbuf is NULL.\r\n");
		return ERR_MEMORY;
	}

	err=netif->input(p,netif);
	if(err != ERR_OK)//这里一定是返回ERR_OK的
	{
		SSNP_DEBUG_PRINT("ethernetif_input():input error.\r\n");
		pbuf_free(p);
		p=NULL;
	}

	return err;
}

//初始化
err_t ethernetif_init(struct netif* netif)
{
	struct ethernetif* ethernetif;

	SSNP_ASSERT("ethernetif_init():error,netif==NULL\n",(netif!=NULL));

	ethernetif=(struct ethernetif*)mem_alloc(sizeof(struct ethernetif));
	if(ethernetif == NULL)
		return ERR_MEMORY;

	netif->state=ethernetif;
	netif->name[0]=IFNAME0;
	netif->name[1]=IFNAME1;

	netif->output=ethbcp_output;
	netif->linkoutput=low_level_output;

	ethernetif->ethaddr=(struct eth_addr*)&(netif->hwaddr);

	low_level_init(netif);

	return ERR_OK;
}

/*******************************************************************************
* Function Name  : ETH_RxPkt_ChainMode
* Description    : Receives a packet.
* Input          : None
* Output         : None
* Return         : frame: farme size and location
*******************************************************************************/
FrameTypeDef ETH_RxPkt_ChainMode(void)
{ 
  u32 framelength = 0;
  FrameTypeDef frame = {0,0}; 

  /* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
  if((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) != (u32)RESET)
  {	
	frame.length = ETH_ERROR;

    if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET)  
    {
      /* Clear RBUS ETHERNET DMA flag */
      ETH->DMASR = ETH_DMASR_RBUS;
      /* Resume DMA reception */
      ETH->DMARPDR = 0;
    }

	/* Return error: OWN bit set */
    return frame; 
  }
  
  if(((DMARxDescToGet->Status & ETH_DMARxDesc_ES) == (u32)RESET) && 
     ((DMARxDescToGet->Status & ETH_DMARxDesc_LS) != (u32)RESET) &&  
     ((DMARxDescToGet->Status & ETH_DMARxDesc_FS) != (u32)RESET))  
  {      
    /* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
    framelength = ((DMARxDescToGet->Status & ETH_DMARxDesc_FL) >> ETH_DMARxDesc_FrameLengthShift) - 4;
	
	/* Get the addrees of the actual buffer */
	frame.buffer = DMARxDescToGet->Buffer1Addr;	
  }
  else
  {
    /* Return ERROR */
    framelength = ETH_ERROR;
  }

  frame.length = framelength;

  frame.descriptor = DMARxDescToGet;
  
  /* Update the ETHERNET DMA global Rx descriptor with next Rx decriptor */      
  /* Chained Mode */    
  /* Selects the next DMA Rx descriptor list for next buffer to read */ 
  DMARxDescToGet = (ETH_DMADESCTypeDef*) (DMARxDescToGet->Buffer2NextDescAddr);    
  
  /* Return Frame */
  return (frame);  
}

/*******************************************************************************
* Function Name  : ETH_TxPkt_ChainMode
* Description    : Transmits a packet, from application buffer, pointed by ppkt.
* Input          : - FrameLength: Tx Packet size.
* Output         : None
* Return         : ETH_ERROR: in case of Tx desc owned by DMA
*                  ETH_SUCCESS: for correct transmission
*******************************************************************************/
u32 ETH_TxPkt_ChainMode(u16 FrameLength)
{   
  /* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
  if((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (u32)RESET)
  {  
	/* Return ERROR: OWN bit set */
    return ETH_ERROR;
  }
        
  /* Setting the Frame Length: bits[12:0] */
  DMATxDescToSet->ControlBufferSize = (FrameLength & ETH_DMATxDesc_TBS1);

  /* Setting the last segment and first segment bits (in this case a frame is transmitted in one descriptor) */    
  DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS;

  /* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
  DMATxDescToSet->Status |= ETH_DMATxDesc_OWN;

  /* When Tx Buffer unavailable flag is set: clear it and resume transmission */
  if ((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET)
  {
    /* Clear TBUS ETHERNET DMA flag */
    ETH->DMASR = ETH_DMASR_TBUS;
    /* Resume DMA transmission*/
    ETH->DMATPDR = 0;
  }
  
  /* Update the ETHERNET DMA global Tx descriptor with next Tx decriptor */  
  /* Chained Mode */
  /* Selects the next DMA Tx descriptor list for next buffer to send */ 
  DMATxDescToSet = (ETH_DMADESCTypeDef*) (DMATxDescToSet->Buffer2NextDescAddr);    

  /* Return SUCCESS */
  return ETH_SUCCESS;   
}


/*******************************************************************************
* Function Name  : ETH_GetCurrentTxBuffer
* Description    : Return the address of the buffer pointed by the current descritor.
* Input          : None
* Output         : None
* Return         : Buffer address
*******************************************************************************/
u32 ETH_GetCurrentTxBuffer(void)
{ 
  /* Return Buffer address */
  return (DMATxDescToSet->Buffer1Addr);   
}

