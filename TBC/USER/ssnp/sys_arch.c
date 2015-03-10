
#define SYS_ARCH_GLOBALS

#include "sys.h"
#include "sys_arch.h"
#include "debug.h"

/*
 *  ע���ʵ��ע��������ض��Ĳ���ϵͳ��ص����ݣ�����ϵͳ�޹صĲ�����sys.c��ʵ��
 */
 

const void* const pvNullPointer=(u32t*)0xffffffff;
 
static OS_MEM* pQueueMem;//ָ����Ϣ���е���ʼָ��
static u8t pcQueueMemoryPool[MAX_QUEUES * sizeof(TQ_DESCR) + MEM_ALIGNMENT -1];//ϵͳ��ȫ�������б�

void sys_init(void)
{
	u8t err;
	pQueueMem=OSMemCreate((void*)((u32t)((u32t)pcQueueMemoryPool+MEM_ALIGNMENT-1) & ~(MEM_ALIGNMENT-1)), MAX_QUEUES, sizeof(TQ_DESCR), &err );
	
}
/*
*********************************************************************************************************
*
*   ����ź���������ʾһ�����߶��ʱ�䷢���ģ���ô�ź�����ʼֵͨ����ֵΪ0            
*   ����ź������ڶԹ�����Դ�ķ�����ô�ź����ĳ�ʼֵӦ��Ϊ1                       
*********************************************************************************************************
*/
sys_sem_t sys_sem_new(u8t count)//�½��ź�����
{
	sys_sem_t sem;
	
	sem=OSSemCreate((u16t)count);
	
	return sem;
}

void sys_sem_free(sys_sem_t sem)//ɾ���ź�����
{
	 u8t     err;
	(void)OSSemDel( (OS_EVENT *)sem, OS_DEL_ALWAYS, &err );
}

u32t sys_arch_sem_wait(sys_sem_t sem,u32t timeout)//�ȴ��ź�����
{
	u8t ucErr;
	u32t ucos_timeout;
	u32t timeout_new;
	

	if(	timeout != 0)
	{
		ucos_timeout = (timeout * OS_TICKS_PER_SEC) / 1000;  /* convert to timetick */
		if(ucos_timeout < 1)
		{
			ucos_timeout = 1;
		}
		else if(ucos_timeout > 65536) /* uC/OS only support u16_t pend */ 
		{
			ucos_timeout = 65535;     
		}
	}
	else 
	{
		ucos_timeout = 0;
	}
	timeout = OSTimeGet();            
	
	OSSemPend ((OS_EVENT *)sem,(u16t)ucos_timeout, (u8t *)&ucErr);
	
	if(ucErr == OS_ERR_TIMEOUT)
	{
		timeout = SYS_ARCH_TIMEOUT;	  /* only when timeout! */
	}
	else
	{    
		/* LWIP_ASSERT( "OSSemPend ", ucErr == OS_ERR_NONE ); */
		/* for pbuf_free, may be called from an ISR */
		
		timeout_new = OSTimeGet();  
		if (timeout_new>=timeout)
		{
			timeout_new = timeout_new - timeout;
		}
		else 
		{
			timeout_new = 0xffffffff - timeout + timeout_new;
		}
		timeout = (timeout_new * 1000 / OS_TICKS_PER_SEC + 1);  /* convert to milisecond Ϊʲô��1 */
	}
	
	return timeout;
}

void sys_sem_signal(sys_sem_t sem)//�ͷ��ź�����
{
	OSSemPost((OS_EVENT*)sem);
}

/********************************************************************************************
 * ������غ���
 *********************************************************************************************/
sys_mbox_t sys_mbox_new(u32t size)
{
	u8t err;
	PQ_DESCR pQDesc;
	
	pQDesc=OSMemGet(pQueueMem,&err);
	if(err == OS_ERR_NONE)
	{
		if( size > MAX_QUEUE_ENTRIES ) 
		{
			size = MAX_QUEUE_ENTRIES;
		}
		pQDesc->pQ = OSQCreate( &(pQDesc->pvQEntries[0]), size );
		if(pQDesc->pQ != NULL)
		{
			return pQDesc;
		}
		else
		{  printf("error3\r\n");
			err = OSMemPut( pQueueMem, pQDesc );
			return SYS_MBOX_NULL;
		}
	}
	else
	{
		return SYS_MBOX_NULL;
	}
}
void sys_mbox_free(sys_mbox_t mbox)
{
		u8t     ucErr;
	
	SSNP_ASSERT( "sys_mbox_free ", mbox != SYS_MBOX_NULL );      
	    
	/* clear OSQ EVENT */
	OSQFlush( mbox->pQ );
	
	/* del OSQ EVENT */
	(void)OSQDel( mbox->pQ, OS_DEL_NO_PEND, &ucErr);
	SSNP_ASSERT( "OSQDel ", ucErr == OS_ERR_NONE );
	
	/* put mem back to mem queue */
	ucErr = OSMemPut( pQueueMem, mbox );
	SSNP_ASSERT( "OSMemPut ", ucErr == OS_ERR_NONE );  
}

/*******************************************************************
 *  ע�������post����ֻ����10��
 *******************************************************************/
void sys_mbox_post(sys_mbox_t mbox,void* msg)
{
	u8t i=0;
	
	if( msg == NULL ) 
	{
		msg = (void*)&pvNullPointer;
	}
	while((i<10) && (( OSQPost( mbox->pQ, msg)) != OS_ERR_NONE))
	{
		i++;    /* if full, try 10 times */
		OSTimeDly(5);
	}
}

/*******************************************************************
 *  ע�������post����ֻ����1��
 *******************************************************************/
err_t sys_mbox_trypost(sys_mbox_t mbox,void* msg)
{
	if(msg == NULL ) 
	{
		msg = (void*)&pvNullPointer;
	}
	if( ( OSQPost( mbox->pQ, msg)) != OS_ERR_NONE )
	{
	    return ERR_MEMORY;
	}
	return ERR_OK;
}

u32t sys_arch_mbox_fetch(sys_mbox_t mbox,void** msg,u32t timeout)
{
  u8t	ucErr;
	u32t	ucos_timeout; 
	u32t  timeout_new;
	void	*temp;
	
	if(timeout != 0)
	{
		ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000; /* convert to timetick */
		
		if(ucos_timeout < 1)
		{
			ucos_timeout = 1;
		}
		else if(ucos_timeout > 65535)	/* ucOS only support u16_t timeout */
		{
			ucos_timeout = 65535;
		}
	}
	else 
	{
		ucos_timeout = 0;
	}

	timeout = OSTimeGet();
	
	temp = OSQPend( mbox->pQ, (u16t)ucos_timeout, &ucErr );
	
	if(msg != NULL)
	{
		if( temp == (void*)&pvNullPointer )
		{
			*msg = NULL;
		}
		else
		{
			*msg = temp;
		}
	}   
	
	if ( ucErr == OS_ERR_TIMEOUT ) 
	{
		timeout = SYS_ARCH_TIMEOUT;
	}
	else
	{
		//SSNP_ASSERT( "OSQPend ", ucErr == OS_ERR_NONE );	
		timeout_new = OSTimeGet();
		if (timeout_new>timeout) 
		{
			timeout_new = timeout_new - timeout;
		}
		else 
		{
			timeout_new = 0xffffffff - timeout + timeout_new;
		}
		timeout = timeout_new * 1000 / OS_TICKS_PER_SEC + 1;  /* convert to milisecond */
	}
	
	return timeout;  
}


/*******************************************************************
 *  �������Ĳ�ѯ��Ϣ����
 *******************************************************************/
void* sys_arch_mbox_fetch_unblock(sys_mbox_t mbox)
{
	void* data;
	u8t	ucErr;
	
	data=NULL;
	data=OSQAccept(mbox->pQ,&ucErr);
//	SSNP_ASSERT( "OSQPend ", ucErr == OS_ERR_NONE );
	
	return data;
}	

/*
 * ����һ���߳�
 */
sys_thread_t sys_thread_new(const char* name,ssnp_thread_fn thread,void* arg,int stacksize,u32t prio)
{
    u8t ubPrio = 0;
    u8t ucErr;
    
    arg = arg;
    
    if((prio > 0) && (prio <= SSNP_TASK_MAX))
    {
        ubPrio = (u8t)(SSNP_START_PRIO + prio - 1);

        if(stacksize > SSNP_STK_SIZE)   /* �����ջ��С������LWIP_STK_SIZE */
            stacksize = SSNP_STK_SIZE;
        
#if (OS_TASK_STAT_EN == 0)
        OSTaskCreate(thread, (void *)arg, &SSNP_TASK_STK[prio-1][stacksize-1],ubPrio);
#else
        OSTaskCreateExt(thread, (void *)arg, &SSNP_TASK_STK[prio-1][stacksize-1],ubPrio
                        ,ubPrio,&SSNP_TASK_STK[prio-1][0],stacksize,(void *)0,OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#endif
        OSTaskNameSet(ubPrio, (u8t*)name, &ucErr);
        
    }
        return ubPrio;
}

u32t sys_time_get(void)
{
	return OSTimeGet();
}