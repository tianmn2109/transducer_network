#ifndef __COMMON_DEFINE_H__
#define __COMMON_DEFINE_H__
#pragma pack(1)		
struct tdcn_teds  //变送器通道TEDS
{
   unsigned char type;//数据类型
   char tbim;
};
struct tdcn_value  //变送器通道数据
{
   unsigned char type;//数据类型
   unsigned char tbim;//tbim标号
   unsigned char num;//变送器通道号
   unsigned char data[4];//数据
};
struct tbc_cmd  //上位机向tbc发送的命令
{
   unsigned char tbim;//tbim标号
   unsigned char num;//变送器通道号
   unsigned char cmdclass;//命令类别
   unsigned char cmdfunc;//命令功能
};
#pragma pack()
#endif 

