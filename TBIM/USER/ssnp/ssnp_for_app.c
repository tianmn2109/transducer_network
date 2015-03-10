#include "ssnp_for_app.h"

#include <includes.h>

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */


/* Private define ****************************************************************/
#define DP83848_PHY            /* Ethernet pins mapped on HY-RedBull V3.0 Board */

#define PHY_ADDRESS       0x01 /* Relative to HY-RedBull V3.0 Board */
	
/* #define MII_MODE */

#define RMII_MODE              /* STM32F107 connect PHY using RMII mode	*/

#define MAX_DHCP_TRIES        4
	

//网络接口定义***************************************************************
static struct netif netif;



/*******************************************************************************
* NVIC也就是嵌套向量中断控制器，也就是在此函数中使能以太网中断，此函数执行后以太网
* 芯片就可以接受数据了
* Function Name  : NVIC_Configuration 
* Description    : Configures the nested vectored interrupt controller.
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure one bit for preemption priority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    /* Enable the ETH Interrupt 在这里使能以太网中断*/
   NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn;
    //NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
* Function Name  : USART_Configuration
* Description    : Configure USART2 
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void USART_Configuration(void)
{ 
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure; 

  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO ,ENABLE);

  RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2 ,ENABLE);

  GPIO_PinRemapConfig(GPIO_Remap_USART2,ENABLE); 

  /*
  *  USART2_TX -> PD5 , USART2_RX -> PD6
  */				
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	         
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOD, &GPIO_InitStructure);		   

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;	        
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART2, &USART_InitStructure); 
  USART_ClearFlag(USART2,USART_FLAG_TC);
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启接收中断
  USART_ITConfig(USART2, USART_IT_TXE, ENABLE); //开启发送中断
  USART_Cmd(USART2, ENABLE);
}
/*******************************************************************************
* Function Name  : Ethernet_Configuration
* Description    : Configures the Ethernet Interface
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void Ethernet_Configuration(void)
{
  ETH_InitTypeDef ETH_InitStructure;

  /* MII/RMII Media interface selection ------------------------------------------*/
#ifdef MII_MODE /* Mode MII with STM3210C-EVAL  */
  GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_MII);

  /* Get HSE clock = 25MHz on PA8 pin (MCO) */
  RCC_MCOConfig(RCC_MCO_HSE);

#elif defined RMII_MODE  /* Mode RMII with STM3210C-EVAL */
  GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_RMII);

  /* Set PLL3 clock output to 50MHz (25MHz /5 *10 =50MHz) */
  RCC_PLL3Config(RCC_PLL3Mul_10);
  /* Enable PLL3 */
  RCC_PLL3Cmd(ENABLE);
  /* Wait till PLL3 is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_PLL3RDY) == RESET)
  {}

  /* Get PLL3 clock on PA8 pin (MCO) */
  RCC_MCOConfig(RCC_MCO_PLL3CLK);
#endif

  /* Reset ETHERNET on AHB Bus */
  ETH_DeInit();

  /* Software reset */
  ETH_SoftwareReset();

  /* Wait for software reset */
  while (ETH_GetSoftwareResetStatus() == SET);

  /* ETHERNET Configuration ------------------------------------------------------*/
  /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
  ETH_StructInit(&ETH_InitStructure);

  /* Fill ETH_InitStructure parametrs */
  /*------------------------   MAC   -----------------------------------*/
  ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable  ;
  ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
  ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
  ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
  ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
  ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
  ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
  ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
  ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
#ifdef CHECKSUM_BY_HARDWARE
  ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif

  /*------------------------   DMA   -----------------------------------*/  
  
  /* When we use the Checksum offload feature, we need to enable the Store and Forward mode: 
  the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum, 
  if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
  ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; 
  ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;         
  ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;     
 
  ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;       
  ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;   
  ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;                                                          
  ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;      
  ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;                
  ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;          
  ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;                                                                 
  ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;

  /* Configure Ethernet */
  ETH_Init(&ETH_InitStructure, PHY_ADDRESS);

  /* Enable the Ethernet Rx Interrupt */
  ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE);
}
/*******************************************************************************
* Function Name  : Ethernet_Initialize
* Description    : Ethernet Initialize function
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void ethernet_initialize(void)
{	 
  GPIO_InitTypeDef GPIO_InitStructure;
  /* Enable ETHERNET clock  */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ETH_MAC | RCC_AHBPeriph_ETH_MAC_Tx |
                        RCC_AHBPeriph_ETH_MAC_Rx, ENABLE);
    
  /* Enable GPIOs clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC |
                         RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO
                             , ENABLE);
  
  /* ETHERNET pins configuration */
  /* AF Output Push Pull:
  - ETH_MII_MDIO / ETH_RMII_MDIO: PA2
  - ETH_MII_MDC / ETH_RMII_MDC: PC1
  - ETH_MII_TXD2: PC2
  - ETH_MII_TX_EN / ETH_RMII_TX_EN: PB11
  - ETH_MII_TXD0 / ETH_RMII_TXD0: PB12
  - ETH_MII_TXD1 / ETH_RMII_TXD1: PB13
  - ETH_MII_PPS_OUT / ETH_RMII_PPS_OUT: PB5
  - ETH_MII_TXD3: PB8 */
      
  /* Configure PA2 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
      
  /* Configure PC1, PC2 and PC3 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
      
  /* Configure PB5, PB8, PB11, PB12 and PB13 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;                              
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
      
  /**************************************************************/
  /*               For Remapped Ethernet pins                   */
  /*************************************************************/
  /* Input (Reset Value):
  - ETH_MII_CRS CRS: PA0
  - ETH_MII_RX_CLK / ETH_RMII_REF_CLK: PA1
  - ETH_MII_COL: PA3
  - ETH_MII_RX_DV / ETH_RMII_CRS_DV: PD8
  - ETH_MII_TX_CLK: PC3
  - ETH_MII_RXD0 / ETH_RMII_RXD0: PD9
  - ETH_MII_RXD1 / ETH_RMII_RXD1: PD10
  - ETH_MII_RXD2: PD11
  - ETH_MII_RXD3: PD12
  - ETH_MII_RX_ER: PB10 */
      
  /* ETHERNET pins remapp in STM32F107-EK Ver1.0 board: RX_DV and RxD[3:0] */
  GPIO_PinRemapConfig(GPIO_Remap_ETH, ENABLE);
      
  /* Configure PA0, PA1 and PA3 as input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
      
  /* Configure PB10 as input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
      
  /* Configure PC3 as input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
      
  /* Configure PD8, PD9, PD10, PD11 and PD12 as input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOD, &GPIO_InitStructure); /**/

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);   

  GPIO_InitStructure.GPIO_Pin =GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
  GPIO_Init(GPIOE, &GPIO_InitStructure);
      
  /* MCO pin configuration------------------------------------------------- */
  /* Configure MCO (PA8) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
      
  Ethernet_Configuration();

  NVIC_Configuration();
}
void hardware_init()
{
	ethernet_initialize();
}
static void ssnp_init_done(void* arg)
{
	sys_sem_t* sem;
	sem=arg;
	sys_sem_signal(*sem);
}
//#未完成#：注意这里没有调用ssnp_init()开启内核线程的函数，在后面完整版中可以在这里添加上
void protocol_init(void)
{
	sys_sem_t sem;
#if NODE == TBIM
	uint8_t macaddress[6]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
#endif
#if NODE == TBC
	uint8_t macaddress[6]={ 0x40, 0xff, 0x00, 0x00, 0x00, 0x00 };
#endif
	sem=sys_sem_new(0);
	ssnp_init(ssnp_init_done,&sem);
	sys_arch_sem_wait(sem,0);
	sys_sem_free(sem);
	
	set_mac_address(macaddress);
	netif_add(&netif,0,NULL,ethernetif_init,ssnp_input);//#未完成#：这里不能写试用这个函数来接收，当总线配置完成后才可以
	netif_set_default(&netif);
	
	USART_Configuration();//开始接收数据
}
#if NODE ==TBIM
err_t tbim_send_reflect_reply_msg(void)
{
	return send_reflect_reply_msg(&netif);
}
#endif
#if NODE == TBC
void set_discovery_timer()
{
	ssnp_timeout(DISCOVERY_TMR_INTERVSL,discovery_timer,(void*)&netif);//内核外部所以使用此函数
}  
err_t send_discovery_msg(void)
{
	return InitiateDiscovery(&netif);
}
err_t tbc_send_timeslot_msg(u8t alias,u8t tbim_tdcn_num,u16t beign_time_slot,u8t time_slot_num)
{
	return send_assign_time_slot_msg(&netif,alias,tbim_tdcn_num,beign_time_slot,time_slot_num);
}
err_t tbc_send_define_epoch_msg(u16t syn_timeslot_num,u16t asyn_timeslot_num)
{
	return send_define_epoch_msg(&netif,syn_timeslot_num,asyn_timeslot_num);
}
err_t tbc_send_begin_of_epoch_msg(void)
{
	return send_begin_of_epoch_msg(&netif);
}
err_t tbc_send_reflect_msg(u8t alias)
{
	return send_reflect_msg(&netif,alias);
}
#endif
/*******************************************************************************
* Function Name  : ETH_IRQHandler
* Description    : Ethernet ISR
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void ETH_IRQHandler(void)
{	//int a;
//	int b;
	  CPU_SR         cpu_sr;
	  OS_ENTER_CRITICAL();                         /* Tell uC/OS-II that we are starting an ISR          */
    OSIntNesting++;
    OS_EXIT_CRITICAL();


    while(ETH_GetRxPktSize() != 0)
    {	//a=TIM_GetCounter(TIM5) ;
        ethernetif_input(&netif);
	//	b=TIM_GetCounter(TIM5) ;
	//	printf("ETH_IRQHandler():the time is %d.\r\n",a > b ? a - b : 0xffff - b + a);
    }
    
    /* Clear Rx Pending Bit */
    ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
    /* Clear the Eth DMA Rx IT pending bits */
    ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);  

    OSIntExit();                                 /* Tell uC/OS-II that we are leaving the ISR          */    
}

///**
//  * @brief  Retargets the C library printf function to the USART.
//  * @param  None
//  * @retval None
//  */
//PUTCHAR_PROTOTYPE
//{
//  /* Place your implementation of fputc here */
//  /* e.g. write a character to the USART */
//  USART_SendData(USART2, (uint8_t) ch);
//
//  /* Loop until the end of transmission */
//  while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
//  {}
//
//  return ch;
//}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
