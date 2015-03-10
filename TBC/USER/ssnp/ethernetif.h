#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#include "netif.h"

err_t ethernetif_init(struct netif* netif);
err_t ethernetif_input(struct netif* netif);
void set_mac_address(u8t* mac_addr);//ÉèÖÃÓ²¼şµØÖ·

#endif
