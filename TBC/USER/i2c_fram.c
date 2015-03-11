																												  #include "i2c_fram.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define I2C_Speed 100000
#define I2C1_SLAVE_ADDRESS7 0xA0
#define I2C_PageSize 8

#define SCL_H GPIOB->BSRR = GPIO_Pin_6
#define SCL_L GPIOB->BRR = GPIO_Pin_6 

#define SDA_H GPIOB->BSRR = GPIO_Pin_7
#define SDA_L GPIOB->BRR = GPIO_Pin_7

#define SCL_read GPIOB->IDR & GPIO_Pin_6
#define SDA_read GPIOB->IDR & GPIO_Pin_7


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
vu8 FRAM_ADDRESS;

/* Private function prototypes -----------------------------------------------*/

/**/
void I2C_delay(void)
{
u16t i=500; //这里可以优化速度 ，经测试最低到5还能写入
u16t j = 20;
while(i) 
{ 
i--; 
j = 20;
while (j)
    j --;
} 
}

u8t I2C_Start(void)
{
SDA_H;
SCL_H;
I2C_delay();
if(!SDA_read)return 0; //SDA线为低电平则总线忙,退出
SDA_L;
I2C_delay();
if(SDA_read) return 0; //SDA线为高电平则总线出错,退出
SDA_L;
I2C_delay();
return 1;
}

void I2C_Stop(void)
{
SCL_L;
I2C_delay();
SDA_L;
I2C_delay();
SCL_H;
I2C_delay();
SDA_H;
I2C_delay();
}

void I2C_Ack(void)
{
SCL_L;
I2C_delay();
SDA_L;
I2C_delay();
SCL_H;
I2C_delay();
SCL_L;
I2C_delay();
}

void I2C_NoAck(void)
{
SCL_L;
I2C_delay();
SDA_H;
I2C_delay();
SCL_H;
I2C_delay();
SCL_L;
I2C_delay();
}

u8t I2C_WaitAck(void) //返回为:=1有ACK,=0无ACK
{
SCL_L;
I2C_delay();
SDA_H;
I2C_delay();
SCL_H;
I2C_delay();
if(SDA_read)
{
SCL_L;
return 0;
}
SCL_L;
return 1;
}

void I2C_SendByte(u8t SendByte) //数据从高位到低位//
{
u8t i=8;
while(i--)
{
SCL_L;
I2C_delay();
if(SendByte&0x80)
SDA_H; 
else 
SDA_L; 
SendByte<<=1;
I2C_delay();
SCL_H;
I2C_delay();
}
SCL_L;
}

u8t I2C_ReceiveByte(void) //数据从高位到低位//
{ 
u8t i=8;
u8t ReceiveByte=0;

SDA_H;
while(i--)
{
ReceiveByte<<=1; 
SCL_L;
I2C_delay();
SCL_H;
I2C_delay();
if(SDA_read)
{
ReceiveByte|=0x01;
}
}
SCL_L;
return ReceiveByte;
}

u8t I2C_FRAM_BufferWrite(u8t* pBuffer, u16t WriteAddr, u16t NumByteToWrite)
{
u8t Addr = 0, count = 0;

Addr = WriteAddr / I2C_PageSize;

count = WriteAddr % I2C_PageSize;

Addr = Addr << 1;

Addr = Addr & 0x0F; 

FRAM_ADDRESS = I2C1_SLAVE_ADDRESS7 | Addr;

if (!I2C_Start()) return 0;
I2C_SendByte(FRAM_ADDRESS);//设置器件地址+段地址 
if (!I2C_WaitAck())
{
I2C_Stop(); 
return 0;
}
I2C_SendByte(count); //设置段内地址 
I2C_WaitAck();

while(NumByteToWrite--)
{
I2C_SendByte(* pBuffer);
I2C_WaitAck();
pBuffer++;
}
I2C_Stop();
//注意：因为这里要等待EEPROM写完，可以采用查询或延时方式(10ms)
//Systick_Delay_1ms(10);
I2C_delay();
return 1;
}


//读出1串数据 
u8t I2C_FRAM_BufferRead(u8t* pBuffer, u16t WriteAddr, u16t NumByteToRead)
{
u8t Addr = 0, count = 0;

Addr = WriteAddr / I2C_PageSize;

count = WriteAddr % I2C_PageSize;

Addr = Addr << 1;

Addr = Addr & 0x0F; 

FRAM_ADDRESS = I2C1_SLAVE_ADDRESS7 | Addr;

if (!I2C_Start()) return 0;

I2C_SendByte(FRAM_ADDRESS);//设置器件地址+段地址 

if (!I2C_WaitAck()) 
{
I2C_Stop(); 
return 0;
}

I2C_SendByte(count); //设置低起始地址 
I2C_WaitAck();
I2C_Start();
I2C_SendByte(FRAM_ADDRESS | 0x01);
I2C_WaitAck();
while(NumByteToRead)
{
*pBuffer = I2C_ReceiveByte();
if(NumByteToRead == 1)I2C_NoAck();
else I2C_Ack(); 
pBuffer++;
NumByteToRead--;
}
I2C_Stop();
return 1;
}

u8t I2C_Fram_PageWrite(u8* pBuffer, u16 WriteAddr, u16 NumByteToWrite)
{

if (!I2C_Start()) return 0;
I2C_SendByte(0xa0);//设置器件地址+段地址 
if (!I2C_WaitAck())
{
I2C_Stop(); 
return 0;
}
I2C_SendByte(WriteAddr); //设置段内地址 
I2C_WaitAck();

while(NumByteToWrite--)
{
I2C_SendByte(* pBuffer);
I2C_WaitAck();
pBuffer++;
}
I2C_Stop();
//注意：因为这里要等待EEPROM写完，可以采用查询或延时方式(10ms)
//Systick_Delay_1ms(10);
I2C_delay();
return 1;
}

void I2C_BufferWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint16_t NumByteToWrite)
{
  uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;

  Addr = WriteAddr % I2C_PageSize;
  count = I2C_PageSize - Addr;
  NumOfPage =  NumByteToWrite / I2C_PageSize;
  NumOfSingle = NumByteToWrite % I2C_PageSize;
 
  /* If WriteAddr is I2C_PageSize aligned  */
  if(Addr == 0) 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage == 0) 
    {
      I2C_Fram_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      I2C_delay();
    }
    /* If NumByteToWrite > I2C_PageSize */
    else  
    {
      while(NumOfPage--)
      {
        I2C_Fram_PageWrite(pBuffer, WriteAddr, I2C_PageSize); 
		I2C_delay();
		WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;
      }

      if(NumOfSingle!=0)
      {
        I2C_Fram_PageWrite(pBuffer, WriteAddr, NumOfSingle);
        I2C_delay();
      }
    }
  }
  /* If WriteAddr is not I2C_PageSize aligned  */
  else 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage== 0) 
    {
      I2C_Fram_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      I2C_delay();
    }
    /* If NumByteToWrite > I2C_PageSize */
    else
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / I2C_PageSize;
      NumOfSingle = NumByteToWrite % I2C_PageSize;	
      
      if(count != 0)
      {  
        I2C_Fram_PageWrite(pBuffer, WriteAddr, count);
        I2C_delay();
        WriteAddr += count;
        pBuffer += count;
      } 
      
      while(NumOfPage--)
      {
        I2C_Fram_PageWrite(pBuffer, WriteAddr, I2C_PageSize);
        I2C_delay();
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;  
      }
      if(NumOfSingle != 0)
      {
        I2C_Fram_PageWrite(pBuffer, WriteAddr, NumOfSingle); 
        I2C_delay();
      }
    }
  }  
}







