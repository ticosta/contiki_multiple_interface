/*
 * ethernet-dev.h
 *
 */

#ifndef ETHERNET_DEV_H_
#define ETHERNET_DEV_H_

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include <string.h>

#include "ethernet-drv.h"



void ethernet_dev_init(void);

#endif /* ETHERNET_DEV_H_ */
