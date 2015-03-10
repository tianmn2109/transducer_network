#include "init.h"
#include "mem.h"
#include "memp.h"
#include "pbuf.h"
#include "netif.h"
#include "dp.h"
#include "sdp.h"
#include "tp.h"
#include "ethbcp.h"
#include "timer.h"

void protocol_stack_init()
{
	sys_init();
	
	mem_init();
	memp_init();
	pbuf_init();

	netif_init();

  //#未完成#：这里还应该有数据链路层的初始化函数
#if NODE == TBC
	TBC_DLL_init();
#else
	TBIM_DLL_init();
#endif
	
	dp_init();
	sdp_init();
	tp_init();

	sys_timeouts_init();
}
