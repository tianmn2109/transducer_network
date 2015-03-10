#ifndef UCTSK_SSNP_TBIM_H
#define UCTSK_SSNP_TBIM_H
struct TBIM_arg
{
	u8t alias;
	u8t tdcn_num;
	u8t tbim_tdcn_num;
	struct netconn* conn;
	void* arg;
};

#endif
