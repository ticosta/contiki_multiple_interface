/*
 * ethernet-drv.h
 *
 */

#ifndef ETHERNET_DRV_H_
#define ETHERNET_DRV_H_

#include "contiki-net.h"

/* The Ethernet MAC address based on linkaddr_node_addr */
extern uint8_t ethernet_macaddr[6];


/**
 * Send data over the ethernet interface.
 *
 * \param packet Pointer to the uip_buf or other buffer where the packet is.
 * \param len Size of the packet.
 *
 * \return len or -1 if there is an error during the transmission.
 */
uint8_t ethernet_send(uint8_t *packet, uint16_t len);

/**
 * Initialise the ethernet driver. Starts the process and configure the hardware.
 */
void ethernet_drv_init(void);

#endif /* ETHERNET_DRV_H_ */
