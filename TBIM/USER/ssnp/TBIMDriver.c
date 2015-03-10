#include "TBIMDriver.h"
#include "stm32f10x.h"

int tdcn1;
int findTdcn(int addr)
{
	return 1;
}


int getTedsFromTdcn(struct TransducerChannel_TEDS* teds_buf,int nAddr,int nChnSn)
{
	return 1;
}

static void LED1_on()
{
	GPIO_ResetBits(GPIOD , GPIO_Pin_13);	
}
static void LED1_off()
{
	GPIO_SetBits(GPIOD , GPIO_Pin_13);	
}
int read_tdcn2()
{
	return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_15)== 1? 0 : 1;
}
int getDataFromChn(void** data,int nAddr,int nChnSn)
{
	return 1;
}
void actor_manage(int tdcn)
{
	switch(tdcn)
	{
		case 1:
		{
			if(tdcn1)
			{
				LED1_on();
				tdcn1=0;
			}
			else
			{
				LED1_off();
				tdcn1=1;
			}
			break;
		}
		default:break;
	}
}
int sensor_manage(int tdcn)
{
	switch(tdcn)
	{
		case 2:return read_tdcn2();
		case 1:case 3:case 4:case 5:case 6:case 7:case 8:case 9:case 10:
		case 11:case 12:case 13:case 14:case 15:case 16:return rand()%10+1;
		default:break;
	}
	return -1;	
}