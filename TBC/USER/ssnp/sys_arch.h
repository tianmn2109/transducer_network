#ifndef SYS_ARCH_H
#define SYS_ARCH_H

#include <includes.h>

#ifdef SYS_ARCH_GLOBALS
#define SYS_ARCH_EXT
#else
#define SYS_ARCH_EXT extern
#endif


//**********�յ��ź��������䶨��********************************************
#define SYS_SEM_NULL          (void*)0 //�յ��ź���
#define SYS_MBOX_NULL         (void*)0 //�յ�����
//**************************************************************************
#define SSNP_STK_SIZE          300 //ssnp�ں��߳�ջ��С
#define SSNP_TASK_MAX          SSNP_TASK_END_PRIO - SSNP_TASK_START_PRIO +1 //ssnp����ջ����
#define SSNP_START_PRIO        SSNP_TASK_START_PRIO

#define MAX_QUEUES             10 //����������

#define MAX_QUEUE_ENTRIES      200	//ÿ��������������


#define SSNP_MBOX_SIZE         MAX_QUEUE_ENTRIES//ssnpÿ��������������

#define SSNP_THREAD_STACKSIZE  300 //ssnp���߳�ջ��С
 
#define SSNP_THREAD_PRIO       1 //ssnp�ں��߳����ȼ�

#define sys_arch_mbox_tryfetch(mbox,msg) sys_arch_mbox_fetch(mbox,msg,1)


//**********�ź��������䶨��***********************************************
typedef struct
{
	OS_EVENT* pQ;
	void* pvQEntries[MAX_QUEUE_ENTRIES];
}TQ_DESCR,*PQ_DESCR;

typedef OS_EVENT*  sys_sem_t; //�ź���
typedef PQ_DESCR  sys_mbox_t;//����

typedef INT8U sys_thread_t;//�߳�id
//*************************************************************************

//ȫ��ssnp����ջ
SYS_ARCH_EXT OS_STK SSNP_TASK_STK[SSNP_TASK_MAX][SSNP_STK_SIZE];
#endif
