#ifndef SYS_H
#define SYS_H

/** Return code for timeouts from sys_arch_mbox_fetch and sys_arch_sem_wait */
#define SYS_ARCH_TIMEOUT 0xffffffffUL

/* sys_mbox_tryfetch returns SYS_MBOX_EMPTY if appropriate.
 * For now we use the same magic value, but we allow this to change in future.
 */
#define SYS_MBOX_EMPTY SYS_ARCH_TIMEOUT 


#include "sys_arch.h"
#include "err.h"

//************************************�ź�����غ���**************************************************
sys_sem_t sys_sem_new(u8t count);//��ȡ�µ��ź���
void sys_sem_free(sys_sem_t sem);//ɾ���ź���

u32t sys_arch_sem_wait(sys_sem_t sem,u32t timeout);//�������ź�����ֱ����ʱ��ע��������ź���Ҳ��ָ�����ʽ������ﲻ��Ҫ��ָ����
void sys_sem_signal(sys_sem_t sem);
//*****************************************************************************************************




//************************************������غ���*****************************************************

sys_mbox_t sys_mbox_new(u32t size);//��ȡ�µ�����
void sys_mbox_free(sys_mbox_t mbox);//ɾ������

void sys_mbox_post(sys_mbox_t mbox,void* msg);//ע�������mbox������ָ����ʽ������Ϊ�˺�ucosһ����������д��sys_mbox_t�ĺ궨����ָ����ʽ
err_t sys_mbox_trypost(sys_mbox_t mbox,void* msg);

u32t sys_arch_mbox_fetch(sys_mbox_t mbox,void** msg,u32t timeout);
void* sys_arch_mbox_fetch_unblock(sys_mbox_t mbox);

#define sys_mbox_tryfetch(mbox,msg) sys_arch_mbox_tryfetch(mbox,msg)
//*****************************************************************************************************


//**********************************ϵͳʱ����غ���***************************************************
u32t sys_time_get(void);
//*****************************************************************************************************

void sys_init(void);


//ssnp���߳�ԭ��
typedef void (* ssnp_thread_fn)(void* arg);
sys_thread_t sys_thread_new(const char* name,ssnp_thread_fn thread,void* arg,int stacksize,u32t prio);


#endif
