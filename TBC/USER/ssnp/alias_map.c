#include "alias_map.h"
#include "config.h"
#include "debug.h"
#if NODE == TBC
#define BITWORD   8
#define SHIFT	    3
#define MASK      0x07

static u8t alias_map[1 + TBIM_NUM/BITWORD];
static u8t first_free=1;

static u8t test(u8t i)
{
	return alias_map[i >> SHIFT] & (1 << (i & MASK));
}
/********************************************************
 * 注意这里不考虑分配不足的情况
 ********************************************************/
u8t alloc_alias_map()
{
	u8t r;
	u8t i;
	r=first_free;
	
	alias_map[r >> SHIFT] |= (1 << (r & MASK));
	for(i=first_free+1;i<=TBIM_NUM;i++)
	{
		if(!test(i))
		{
			first_free=i;
			break;
		}
	}
//	SSNP_DEBUG_PRINT("alloc_alias_map():alloc alias in alias map.\r\n");
//	printf("alloc_alias_map():the alloc alias is %d.\r\n",r);
	return r;
}
void del_alias_map(u8t alias)
{
	if(first_free >= alias)
		first_free=alias;
	
	alias_map[alias >> SHIFT] &= ~(1 << (alias & MASK));
}
#endif
