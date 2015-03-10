#ifndef SYS_ARCH_H
#define SYS_ARCH_H

#include <includes.h>

#ifdef SYS_ARCH_GLOBALS
#define SYS_ARCH_EXT
#else
#define SYS_ARCH_EXT extern
#endif


//**********空的信号量和邮箱定义********************************************
#define SYS_SEM_NULL          (void*)0 //空的信号量
#define SYS_MBOX_NULL         (void*)0 //空的邮箱
//**************************************************************************
#define SSNP_STK_SIZE          300 //ssnp内核线程栈大小
#define SSNP_TASK_MAX          SSNP_TASK_END_PRIO - SSNP_TASK_START_PRIO +1 //ssnp任务栈个数
#define SSNP_START_PRIO        SSNP_TASK_START_PRIO

#define MAX_QUEUES             10 //最大邮箱个数

#define MAX_QUEUE_ENTRIES      200	//每个邮箱的最大容量


#define SSNP_MBOX_SIZE         MAX_QUEUE_ENTRIES//ssnp每个邮箱的最大容量

#define SSNP_THREAD_STACKSIZE  300 //ssnp中线程栈大小
 
#define SSNP_THREAD_PRIO       1 //ssnp内核线程优先级

#define sys_arch_mbox_tryfetch(mbox,msg) sys_arch_mbox_fetch(mbox,msg,1)


//**********信号量和邮箱定义***********************************************
typedef struct
{
	OS_EVENT* pQ;
	void* pvQEntries[MAX_QUEUE_ENTRIES];
}TQ_DESCR,*PQ_DESCR;

typedef OS_EVENT*  sys_sem_t; //信号量
typedef PQ_DESCR  sys_mbox_t;//邮箱

typedef INT8U sys_thread_t;//线程id
//*************************************************************************

//全局ssnp任务栈
SYS_ARCH_EXT OS_STK SSNP_TASK_STK[SSNP_TASK_MAX][SSNP_STK_SIZE];
#endif
