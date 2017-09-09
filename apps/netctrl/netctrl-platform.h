#ifndef CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_PLATFORM_H_
#define CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_PLATFORM_H_

/**
 * \addtogroup netctrl
 * @{
 */

/*
 * @file     netctrl-platform.h
 * @brief    This header contains all the defenitions dependent from the OS/platform/etc
 * @version  0.1
 * @author   Claudio Prates & Ricardo Jesus & Tiago Costa
 */

#include "contiki.h"
#include "uip.h"

extern void *uip_sappdata;
extern uint32_t netctrl_node_data;

typedef clock_time_t netctrl_time_t;

#define netctrl_data_buffer   (&uip_buf[UIP_IPUDPH_LEN + UIP_LLH_LEN])
#define netctrl_sdata_buffer  uip_sappdata
#define ntohs  uip_ntohs
#define ntohl  uip_ntohl
#define htons  uip_htons
#define htonl  uip_htonl
/*---------------------------------------------------------------------------*/
/**
 * Responsible for initializes the necessary UDP connection for the server.
 */
void netctrl_server_init_network();
/*---------------------------------------------------------------------------*/
/**
 * Responsible for initializes the necessary UDP connection for the client.
 *
 * \param addr Server's address
 * \param port Listening port on the server.
 */
void netctrl_client_init_network(const uip_ipaddr_t *addr, uint16_t port);
/*---------------------------------------------------------------------------*/
/**
 * Used to build the node hash.
 *
 * \return Returns the calculated hash.
 */
uint32_t netctrl_calc_node_hash();
/*---------------------------------------------------------------------------*/
/**
 * Used to send messages to nodes.
 *
 * \param data Data to be sent. Used by server side to send messages to different destinations.
 * \param length Data length in bytes.
 */
void netctrl_send_messageto(uint8_t *data, uint16_t length);
/*---------------------------------------------------------------------------*/
/**
 * Used to send messages to controller.
 *
 * \param data Data to be sent. Used by client side to send messages olways for the same destination.
 * \param length Data length in bytes.
 */
void netctrl_send_message(uint8_t *data, uint16_t length);
/*---------------------------------------------------------------------------*/
/**
 * Get the node ID. It MUST be called when handling a received request.
 * This id need to be something that uniquely identifies the node in the network, i.e. its IP address.
 *
 * \return Returns the node ID that sent the request.
 */
void * netctrl_get_nodeId();

/**
 * @}
 */

#endif /* CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_PLATFORM_H_ */
