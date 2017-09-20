#ifndef CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_CLIENT_H_
#define CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_CLIENT_H_

/**
 * \addtogroup netctrl
 * @{
 */

/*
 * @file     netctrl-client.h
 * @brief    Implementation of Network Controll protocol for client side.
 * @version  0.1
 * @author   Claudio Prates & Ricardo Jesus & Tiago Costa
 */

#include <stdio.h>
//
#include "netctrl.h"
#include "netctrl-platform.h"

#define NETCTRL_CLIENT_UNKNOWN_STATE          (-1)
#define NETCTRL_CLIENT_INITIAL_REQ_ID         0
//
#define NETCTRL_CLIENT_EVT_TIMER              0x1
#define NETCTRL_CLIENT_EVT_NET                0x2
// Client states
#define NETCTRL_CLIENT_UNREGISTERED           0X0
#define NETCTRL_CLIENT_WAIT_CONFIRM           0X1
#define NETCTRL_CLIENT_REGISTERED             0X2
// timers
#define REG_TIMEOUT                           NETCTRL_IDLETIME_MAX_VALUE
#define REG_RETRY_TIME                        (REG_TIMEOUT / 2)
//
#define MAX_RENEW_RETRIES                     2
//
#ifndef NODE_EQUIPEMENT_TYPE
#define NODE_EQUIPEMENT_TYPE                  0x0
#endif /* NODE_EQUIPEMENT_TYPE */
/*---------------------------------------------------------------------------*/
/**
 * Client event's handler.
 *
 * \return Return the time until the next event should be generated.
 */
netctrl_time_t netctrl_client_handle_event();
/*---------------------------------------------------------------------------*/
/**
 *  Used to know if the node is registered or not.
 *
 *  \return Return 1 if registered, 0 if not.
 */
uint8_t netctrl_is_registered();
/**
 * @}
 */
#endif /* CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_CLIENT_H_ */
