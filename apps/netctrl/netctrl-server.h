#ifndef CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_SERVER_H_
#define CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_SERVER_H_
/**
 * \addtogroup netctrl
 * @{
 */

/*
 * @file     netctrl-server.h
 * @brief    Implementation of Network Controll protocol for server side.
 * @version  0.1
 * @author   Claudio Prates & Ricardo Jesus & Tiago Costa
 */

#include "netctrl.h"

extern uint8_t netctrl_periodicity;

#define netctrl_rsp_conf(base)  ((uint8_t *)((base) + NETCTRL_RSP_HEADER_SIZE)) /*!< Used to retreive the configuration addr field from rsponse packets. */

/*---------------------------------------------------------------------------*/
/**
 * Initializes all necessary data structures and mechanisms of the protocoll.
 */
void netctrl_server_init();
/*---------------------------------------------------------------------------*/
/**
 * Server's handler for network events.
 *
 * \return Returns the result of the operation correonding to the response sent.
 */
int netctrl_server_handle_net_event();
/*---------------------------------------------------------------------------*/
/**
 * Used to set new periodicity values.
 */
void netctrl_set_periodicity(uint8_t periodicity);

/**
 * @}
 */

#endif /* CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_SERVER_H_ */
