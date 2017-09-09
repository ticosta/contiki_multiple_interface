/**
 * \defgroup netctrl
 * @{
 */

/*
 * @file     netctrl.h
 * @brief    Implementation of Network Controll protocol.
 * @version  0.1
 * @author   Claudio Prates & Ricardo Jesus & Tiago Costa
 */

#ifndef CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_H_
#define CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_H_

#include <stdio.h>

// TODO: In the future, these values ​​should be stored in a flash memory to allow adding or removing new types dynamically.
#define NETCTRL_EQUIPEMENT_TYPE_SMART_SOCKET  0x01
#define NETCTRL_EQUIPEMENT_TYPE_LAMP          0x02

#define NETCTRL_REQUEST_TYPE_REG    0x00
#define NETCTRL_REQUEST_TYPE_RENEW  0x01

#define NETCTRL_NOT_INITIALIZED               (-1)
#define NETCTRL_INVALID_REQUEST               (-2)
#define NETCTRL_RESPONSE_RESULT_REG_OK        0x00
#define NETCTRL_RESPONSE_RESULT_REG_FAILED    0x01
#define NETCTRL_RESPONSE_RESULT_RENEW_OK      0x02
#define NETCTRL_RESPONSE_RESULT_RENEW_FAILED  0x03
/**
 * Periodicity used by nodes to renew its registration.
 * (NETCTRL_DEFAULT_PERIODICITY * 2) should be lesser then NETCTRL_IDLETIME_MAX_VALUE. This way a node can transmit a renew request twice in case the first fails.
 */
#define NETCTRL_DEFAULT_PERIODICITY  8
#define NETCTRL_IDLETIME_MAX_VALUE  20
/** \name "Netctrl" Data structures */
/** @{ */

/** Defenition of request header */
typedef struct {
	uint16_t id; /*!< Request's ID */
	uint8_t type; /*!< Request's type */
	uint8_t eq_type; /*!< Equipement's type that make the resquest */
	uint32_t data; /*!< Data sent by the nodes */
} netctrl_req_header_t;

/** Definition of response header */
typedef struct {
	uint16_t id; /*<! Request's ID */
	uint8_t result; /*<! Request's result */
	uint8_t periodicity; /*<! Periodicity which equipments MUST use to send renew requests in seconds */
	uint16_t conf_len; /*<! Configuration length in bytes. */
} netctrl_rsp_header_t;
/** @} */

#define NETCTRL_REQ_HEADER_SIZE sizeof(netctrl_req_header_t)
#define NETCTRL_RSP_HEADER_SIZE sizeof(netctrl_rsp_header_t)

#define netctrl_next_id(current_id)  ((current_id) + 1)

#define NETCTRL_DEFAULT_LISTEN_PORT  10100


#endif /* CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NETCTRL_H_ */
/**
 * @}
 */
