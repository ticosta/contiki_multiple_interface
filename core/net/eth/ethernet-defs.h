/*
 * ethernet-defs.h
 *
 *  Created on: Mar 26, 2017
 *
 *  Defines and constants for Ethernet interface configuration.
 */

#ifndef ETH_ETHERNET_DEFS_H_
#define ETH_ETHERNET_DEFS_H_

#include "contiki-net.h"


#define ETH_LINK_MTU				UIP_LINK_MTU
#define ETH_TTL						UIP_TTL
#define ETH_ND6_REACHABLE_TIME		UIP_ND6_REACHABLE_TIME
#define ETH_ND6_RETRANS_TIMER		UIP_ND6_RETRANS_TIMER
#define ETH_ND6_DEF_MAXDADNS		UIP_ND6_DEF_MAXDADNS
// Used to make eth ll address different from radio interface
#define IP_LINK_LOCAL_PREFIX_BYTE	(0x21)
//
#define IPV6_CONF_ADDR_8			0xA


/* The ethernet MAC address*/
extern uip_eth_addr ethernet_if_addr;
/* The default Neighbor used to forward traffic through the ethernet interface */
extern uip_ipaddr_t default_neighbor_ip6_addr;


#endif /* ETH_ETHERNET_DEFS_H_ */
