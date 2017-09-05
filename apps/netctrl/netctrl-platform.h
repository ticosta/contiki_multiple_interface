/*
 * @file     netctrl-platform.h
 * @brief    This header contains all the defenitions dependent from the OS/platform/etc
 * @version  0.1
 * @author   Claudio Prates & Ricardo Jesus & Tiago Costa
 */

#ifndef CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_PLATFORM_H_
#define CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_PLATFORM_H_

#include "contiki.h"
#include "uip.h"

extern void *uip_sappdata;

#define netctrl_data_buffer  uip_appdata
#define netctrl_sdata_buffer  uip_sappdata
#define ntohs  uip_ntohs
#define ntohl  uip_ntohl
/*---------------------------------------------------------------------------*/
/**
 * Responsible for initializes the necessary UDP connection for the server.
 */
void netctrl_init_network();
/*---------------------------------------------------------------------------*/
/**
 * Used to build the node hash.
 *
 * \return Returns the calculated hash.
 */
uint32_t netctrl_calc_node_hash();
/*---------------------------------------------------------------------------*/
/**
 * Used to send messages to nodes or controller.
 *
 * \param data Data to be sent.
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

#endif /* CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_PLATFORM_H_ */
