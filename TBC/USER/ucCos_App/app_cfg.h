/****************************************Copyright (c)****************************************************

**--------------File Info---------------------------------------------------------------------------------
** File name:               app_cfg.h
** Descriptions:            ucosii configuration
**
**--------------------------------------------------------------------------------------------------------
** Created by:              AVRman
** Created date:            2010-11-9
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

#ifndef  __APP_CFG_H__
#define  __APP_CFG_H__
					  

/*
*********************************************************************************************************
*                                       MODULE ENABLE / DISABLE
*********************************************************************************************************
*/
#define  OS_VIEW_MODULE                  DEF_DISABLED	     	/* DEF_ENABLED = Present, DEF_DISABLED = Not Present        */


/*
*********************************************************************************************************
*                                              TASKS NAMES
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*********************************************************************************************************
*/	
#define  APP_TASK_START_PRIO             (                 1) 

#define  SSNP_TASK_START_PRIO            (                 2)
#define  SSNP_TASK_END_PRIO              (                 4)

#define  APP_TASK_BLINK_PRIO          	 (                 5) 

#define  APP_TASK_SSNP_PRIO              (                 6)//ssnp应用线程优先级
#define  APP_TASK_SSNP_DP_PRIO           (                 7)//ssnp应用程序dp优先级
#define  APP_TASK_SSNP_TP_PRIO           (                 8)//ssnp应用程序tp优先级
#define  APP_TASK_SSNP_SDP_PRIO          (                 9)//ssnp应用程序sdp优先级




#define  APP_TASK_OSVIEW_TERMINAL_PRIO	 (OS_LOWEST_PRIO - 6)

#define  OS_VIEW_TASK_PRIO               (OS_LOWEST_PRIO - 3)
#define  OS_TASK_TMR_PRIO                (OS_LOWEST_PRIO - 2)



/*
*********************************************************************************************************
*                                            TASK STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/
#define  APP_TASK_SSNP_STK_SIZE                          256u

#define  APP_TASK_START_STK_SIZE                          64u
#define  APP_TASK_BLINK_STK_SIZE                         128u
#define  APP_TASK_OSVIEW_TERMINAL_STK_SIZE   			 128u
#define  OS_VIEW_TASK_STK_SIZE                           128u

#define  APP_TASK_DP_STK_SIZE                            128u//ssnp应用程序dp线程栈大小
#define  APP_TASK_TP_STK_SIZE                            128u//ssnp应用程序tp线程栈大小
#define  APP_TASK_SDP_STK_SIZE                           128u//ssnp应用程序sdp线程栈大小




/*
*********************************************************************************************************
*                                                  LIB
*********************************************************************************************************
*/

#define  uC_CFG_OPTIMIZE_ASM_EN                 DEF_ENABLED
#define  LIB_STR_CFG_FP_EN                      DEF_DISABLED



#endif

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/

