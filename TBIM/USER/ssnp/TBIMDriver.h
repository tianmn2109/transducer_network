#ifndef TBIMDRIVER_H
#define TBIMDRIVER_H
#include "TEDS.h"

//检测地址为addr的变送器通道是否存在，如果此地址不存在那么后面的地址也不存在
int findTdcn(int addr);

//将nAddr和nChnSn对应的teds写入teds_buf中
int getTedsFromTdcn(struct TransducerChannel_TEDS* teds_buf,int nAddr,int nChnSn);

//将nAddr和nChnSn的数写入data中，具体解析通过TBC保存的此变送器通道的teds来决定
int getDataFromChn(void** data,int nAddr,int nChnSn);

void actor_manage(int tdcn);
int sensor_manage(int tdcn);


#endif

