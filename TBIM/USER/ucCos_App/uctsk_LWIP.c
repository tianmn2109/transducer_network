/****************************************Copyright (c)****************************************************

**--------------File Info---------------------------------------------------------------------------------
** File name:               uctsk_LWIP.c
** Descriptions:            The uctsk_LWIP application function
**
**--------------------------------------------------------------------------------------------------------
** Created by:              AVRman
** Created date:            2011-3-2
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <includes.h> 
/* lwIP includes. */
#include "ip.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "netif/loopif.h"

#include "webpage.h"
#include "search.h"

/* Private variables ---------------------------------------------------------*/
static  OS_STK  AppLWIPTaskStk[APP_TASK_LWIP_STK_SIZE];
//static  uint8_t webpage [ sizeof(adc_WebSide) ];

/* Private function prototypes -----------------------------------------------*/
static  void    ADC_Configuration (void);
static  void    uctsk_LWIP        (void *pdata);
static  void    vHandler_HTTP     (struct netconn  *pstConn);
static  void    __Handler_HTTPGet (struct netconn  *pstConn);
static void __Handler_HTTP_LED(struct netconn  *pstConn,INT8S *__pbData);
static void __Handler_HTTP_ADC(struct netconn  *pstConn);
unsigned char led_web_flag = 1;

void  App_LWIPTaskCreate (void)
{
    CPU_INT08U  os_err;

	os_err = os_err; /* prevent warning... */

	os_err = OSTaskCreate((void (*)(void *)) uctsk_LWIP,				
                          (void          * ) 0,							
                          (OS_STK        * )&AppLWIPTaskStk[APP_TASK_LWIP_STK_SIZE - 1],		
                          (INT8U           ) APP_TASK_LWIP_PRIO  );							

	#if OS_TASK_NAME_EN > 0
    	OSTaskNameSet(APP_TASK_LWIP_PRIO, "Task LWIP", &os_err);
	#endif
}

static  void  uctsk_LWIP(void *pdata)
{

	struct netconn  *__pstConn, *__pstNewConn;
 
    Ethernet_Initialize();
	LED_GPIO_Configuration();
    Init_lwIP();

	ADC_Configuration();

    __pstConn = netconn_new(NETCONN_TCP);
    netconn_bind(__pstConn, NULL,80);
    netconn_listen(__pstConn);
    	/******熄灭所有的led***************/
		GPIO_SetBits(GPIOC , GPIO_Pin_6);
		GPIO_SetBits(GPIOC , GPIO_Pin_7);
		GPIO_SetBits(GPIOC , GPIO_Pin_8);
		GPIO_SetBits(GPIOC , GPIO_Pin_0);
	for(;;)
   	{
		__pstNewConn = netconn_accept(__pstConn);
		
		if(__pstNewConn != NULL)
		{			
			vHandler_HTTP(__pstNewConn);
			while(netconn_delete(__pstNewConn) != ERR_OK)
			{
			   OSTimeDlyHMSM(0, 0, 0, 10);
			}
		}
    }
}

/*******************************************************************************
* Function Name  : vHandler_HTTP
* Description    : HTTP处理
* Input          : - pstConn: 指向struct netconn结构的指针
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void vHandler_HTTP(struct netconn  *pstConn)
{
	struct netbuf 		*__pstNetbuf;
	INT8S			*__pbData;
	u16_t			__s32Len;

 	__pstNetbuf = netconn_recv(pstConn);
	if(__pstNetbuf != NULL)
	{
		netbuf_data (__pstNetbuf, (void *)&__pbData, &__s32Len );

		if (strncmp(__pbData, "GET /STM32F107ADC", 17) == 0)  //ADC页面请求
		{
		  __Handler_HTTP_ADC(pstConn);
    
	    }
		else if ((led_web_flag = (strncmp(__pbData, "GET /method=get", 15) == 0)) || ((strncmp(__pbData, "GET /STM32F107LED", 17) == 0)))
		{
		  __Handler_HTTP_LED(pstConn,__pbData);	  //按“发送命令”时led_web_flag =1.
    
	    }
		else if (strncmp(__pbData, "GET ", 4) == 0) //标题网址      
		{
		  __Handler_HTTPGet(pstConn);
    
	    }
	}
	netbuf_delete(__pstNetbuf);	
	netconn_close(pstConn);
}

/*******************************************************************************
* Function Name  : __Handler_HTTPGet
* Description    : 处理HTTP协议的GET请求
* Input          : - pstConn: 指向struct netconn结构的指针
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void __Handler_HTTPGet(struct netconn  *pstConn)
{
    static uint16_t pagecount = 0;
	uint16_t AD_value;
	int8_t *ptr;
			   
	netconn_write(pstConn, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n", strlen("HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n"), NETCONN_COPY);	//发送HTPP请求什么的
   	netconn_write(pstConn, data_STM32F107_html, sizeof(data_STM32F107_html), NETCONN_COPY);	/* HTTP网页 */

}

/*******************************************************************************
* Function Name  : __Handler_HTTPGet
* Description    : 处理HTTP协议的LED页面请求
* Input          : - pstConn: 指向struct netconn结构的指针
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void __Handler_HTTP_LED(struct netconn  *pstConn,INT8S *__pbData)
{
    static uint16_t pagecount = 0;
	uint16_t AD_value = 0;
	unsigned int i = 15;
	int8_t *ptr;
    	/******熄灭所有的led***************/
	if(led_web_flag == 1)//如果按了“发送命令”，熄灭所有的led
	{
		GPIO_SetBits(GPIOC , GPIO_Pin_6);
		GPIO_SetBits(GPIOC , GPIO_Pin_8);
		GPIO_SetBits(GPIOC , GPIO_Pin_7);
		GPIO_SetBits(GPIOC , GPIO_Pin_0);
	}
	led_web_flag = 0;
    while(__pbData[i]!=0x20/*空格*/)
    {
	  i++; 
      if (__pbData[i] == 0x6C /* l */)
      {
        i++;
        if (__pbData[i] ==  0x65 /* e */)
        {
          i++;
          if (__pbData[i] ==  0x64 /* d*/)
          {
            i+=2; 
            if(__pbData[i]==0x31 /* 1 */)
            {
				GPIO_ResetBits(GPIOC , GPIO_Pin_6); 
            }

            if(__pbData[i]==0x32 /* 2 */)
            {
                GPIO_ResetBits(GPIOC , GPIO_Pin_8);  
            }
    
            if(__pbData[i]==0x33 /* 3 */)
            {
                GPIO_ResetBits(GPIOC , GPIO_Pin_0);  
            }
        
            if(__pbData[i]==0x34 /* 4 */)
            {
                GPIO_ResetBits(GPIOC , GPIO_Pin_7); 
            } 
          }   
        }
      } 
    } 
	netconn_write(pstConn, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n", strlen("HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n"), NETCONN_COPY);	//发送HTPP请求什么的
    netconn_write(pstConn, data_STM32F107LED_html, sizeof(data_STM32F107LED_html), NETCONN_COPY);	/* HTTP网页 */
}
/*******************************************************************************
* Function Name  : __Handler_HTTPGet
* Description    : 处理HTTP协议的adc页面请求
* Input          : - pstConn: 指向struct netconn结构的指针
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void __Handler_HTTP_ADC(struct netconn  *pstConn)
{
    static uint16_t pagecount = 0;
	uint16_t AD_value;
	float adc_data = 0;
	int8_t *ptr;
    char Digit1=0, Digit2=0, Digit3=0; 
    int ADCVal = 0;        


    ADCVal = ADC_GetConversionValue(ADC1);
    ADCVal = ADCVal/8;
    Digit1= ADCVal/100;
    Digit2= (ADCVal-(Digit1*100))/10;
    Digit3= ADCVal-(Digit1*100)-(Digit2*10);
    /* Update the ADC value in STM32F107ADC.html 注意第2310为adc  bar 宽度的位置*/
    *((data_STM32F107ADC_html) + 2310) = 0x30 + Digit1; /* ADC value 1st digit */
    *((data_STM32F107ADC_html) + 2311) = 0x30 + Digit2; /* ADC value 2nd digit */ 
    *((data_STM32F107ADC_html) + 2312) = 0x30 + Digit3; /* ADC value 3rd digit*/
	/****电压值显示****/
	adc_data = (float)ADCVal / 50 * 33;
    Digit1= (char)(adc_data/100);
    Digit2= (char)((adc_data-(Digit1*100))/10);
    Digit3= (char)(adc_data-(Digit1*100)-(Digit2*10));
	*((data_STM32F107ADC_html) + 2234) = 0x30 + Digit1; /* ADC value 1st digit */
    *((data_STM32F107ADC_html) + 2236) = 0x30 + Digit2; /* ADC value 2nd digit */ 
    *((data_STM32F107ADC_html) + 2237) = 0x30 + Digit3; /* ADC value 3rd digit*/
     
	netconn_write(pstConn, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n", strlen("HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n"), NETCONN_COPY);	//发送HTPP请求什么的
    netconn_write(pstConn, data_STM32F107ADC_html, sizeof(data_STM32F107ADC_html), NETCONN_COPY);	/* HTTP网页 */
}



/*******************************************************************************
* Function Name  : ADC_Configuration
* Description    : Configure the ADC.
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void ADC_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  ADC_InitTypeDef ADC_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
  
  /* Configure PB.1 (ADC Channel9) as analog input -------------------------*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);   
   
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	   /* 独立模式 */
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;			   /* 连续多通道模式 */
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	   /* 连续转换 */
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;  /* 转换不受外界决定 */
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;		       /* 右对齐 */
  ADC_InitStructure.ADC_NbrOfChannel = 1;					   /* 扫描通道数 */
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channel9 configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, ADC_SampleTime_239Cycles5);	
  ADC_Cmd(ADC1, ENABLE);                 /* Enable ADC1 */                      
  ADC_SoftwareStartConvCmd(ADC1,ENABLE);    /* 使能转换开始 */
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/

