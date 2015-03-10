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

//************************************信号量相关函数**************************************************
sys_sem_t sys_sem_new(u8t count);//获取新的信号量
void sys_sem_free(sys_sem_t sem);//删除信号量

u32t sys_arch_sem_wait(sys_sem_t sem,u32t timeout);//阻塞在信号量上直到超时，注意这里的信号量也是指针的形式因此这里不需要用指针了
void sys_sem_signal(sys_sem_t sem);
//*****************************************************************************************************




//************************************邮箱相关函数*****************************************************

sys_mbox_t sys_mbox_new(u32t size);//获取新的邮箱
void sys_mbox_free(sys_mbox_t mbox);//删除邮箱

void sys_mbox_post(sys_mbox_t mbox,void* msg);//注意这里的mbox必须是指针形式，这里为了和ucos一致所以这样写，sys_mbox_t的宏定义是指针形式
err_t sys_mbox_trypost(sys_mbox_t mbox,void* msg);

u32t sys_arch_mbox_fetch(sys_mbox_t mbox,void** msg,u32t timeout);
void* sys_arch_mbox_fetch_unblock(sys_mbox_t mbox);

#define sys_mbox_tryfetch(mbox,msg) sys_arch_mbox_tryfetch(mbox,msg)
//*****************************************************************************************************


//**********************************系统时间相关函数***************************************************
u32t sys_time_get(void);
//*****************************************************************************************************

void sys_init(void);


//ssnp主线程原型
typedef void (* ssnp_thread_fn)(void* arg);
sys_thread_t sys_thread_new(const char* name,ssnp_thread_fn thread,void* arg,int stacksize,u32t prio);


#endif
