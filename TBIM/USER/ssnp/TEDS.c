
#include "TEDS.h"

static struct PHY_TEDS phy_teds={sizeof(struct PHY_TEDS),0,0,0,0,0,0,0};

u8t phy_teds_asy_flag()
{
	return phy_teds.asy_flag;
}
u8t phy_teds_payload_encoding()
{
	return phy_teds.payload_encoding;
}
f32 phy_teds_start_delay()
{
	return phy_teds.start_delay;
}
f32 phy_teds_reflect_delay()
{
	return phy_teds.reflect_delay;
}
f32 phy_teds_reflect_delay_uncertainty()
{
	return phy_teds.reflect_delay_uncertainty;
}
