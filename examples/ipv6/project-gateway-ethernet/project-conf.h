#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_


#define UIP_CONF_IPV6_RPL							0
#define NETSTACK_CONF_WITH_IPV6						1
#define UIP_CONF_ND6_SEND_NA						1
#define UIP_CONF_ROUTER								0
#define UIP_CONF_UDP								1
// Multi Interfaces
#define UIP_CONF_DS6_INTERFACES_NUMBER				2
#define UIP_CONF_DS6_DEFAULT_PREFIX					0xFEC0

// Link Layer header size - Ethernet - 14
//#define UIP_CONF_LLH_LEN							14  // defenida no contiki-conf.h da plataforma
// Used to have an user handler for ICMP packets - core/net/tcpip.c icmp6_new()
// 	- Associates the process
// /core/net/ipv6/uip6.c - uip_process()
//	- UIP_ICMP6_APPCALL() - call the handler
//#define UIP_CONF_ICMP6								1	// 0 as default
//#define UIP_CONF_IPV6_QUEUE_PKT 					1 // ALTERAR NO CONTIKI-CONF DA PLATAFORMA - 0 por defeito
//#define UIP_CONF_ROUTER							0 // defenido em platform/contiki-conf.h
//#define UIP_CONF_TCP								1
//#define UIP_CONF_ND6_SEND_RA						1 // defenido em platform/contiki-conf.h
//#define UIP_CONF_ND6_SEND_RS						1 // defenido em platform/contiki-conf.h



/**
 * Está aqui para resolver o problema do Hard Fault por desalinhamento na execução de store/load multiplos.
 * uip_ip6addr_copy é utilizada nos sockets HTTP
 */
#define uip_ipaddr_copy(dest, src) 		memcpy(dest, src, sizeof(uip_ip6addr_t));
#define uip_ip6addr_copy(dest, src)		memcpy(dest, src, sizeof(uip_ip6addr_t));

#endif
