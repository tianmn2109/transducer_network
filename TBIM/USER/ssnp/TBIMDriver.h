#ifndef TBIMDRIVER_H
#define TBIMDRIVER_H
#include "TEDS.h"

//����ַΪaddr�ı�����ͨ���Ƿ���ڣ�����˵�ַ��������ô����ĵ�ַҲ������
int findTdcn(int addr);

//��nAddr��nChnSn��Ӧ��tedsд��teds_buf��
int getTedsFromTdcn(struct TransducerChannel_TEDS* teds_buf,int nAddr,int nChnSn);

//��nAddr��nChnSn����д��data�У��������ͨ��TBC����Ĵ˱�����ͨ����teds������
int getDataFromChn(void** data,int nAddr,int nChnSn);

void actor_manage(int tdcn);
int sensor_manage(int tdcn);


#endif

