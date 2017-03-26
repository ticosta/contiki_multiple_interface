/**
 *  Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * Driver Ethernet
 *
 * This file was based on rpl-border-router example for avr-rss2 platform.
 *
 */

#include <stdio.h>

#include "contiki-net.h"
#include "net/ipv4/uip-neighbor.h"

#include "ethernet-drv.h"
#include "tools/sicslow_ethernet.h"
#include "dev/enc28j60/enc28j60.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define BASE_BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define IPBUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])


PROCESS(ethernet_process, "Ethernet driver");



/* The Ethernet MAC address based on linkaddr_node_addr */
uint8_t ethernet_macaddr[6];
/* Used to check incoming packets */
static const uint8_t broadcast_ethaddr[6] =
  	  	  	  	 	 	 	 {0xff,0xff,0xff,0xff,0xff,0xff};


void
ethernet_drv_init(void) {

	/* Mangle the EUI64 into a 48-bit Ethernet address. */
	// Pode ser linkaddr_node_addr ou uip_lladdr
	memcpy(&ethernet_macaddr[0], &linkaddr_node_addr.u8[0], 3);
	memcpy(&ethernet_macaddr[3], &linkaddr_node_addr.u8[5], 3);

	/* debug */
	PRINTF("Ethernet MAC addr: ");
	PRINTLLADDR((uip_lladdr_t *)ethernet_macaddr);
	PRINTF("\n");

	enc28j60_init(ethernet_macaddr);

	process_start((struct process *)&ethernet_process, NULL);
}

/*---------------------------------------------------------------------------*/
uint8_t
ethernet_send(uint8_t *packet, uint16_t len) {
	PRINTF("* ethernet-drv: Sending packet over ethernet with len: %u\n", len);

	int ret = enc28j60_send(packet, len);

	return ret;
}
/*---------------------------------------------------------------------------*/
uint16_t
ethernet_poll(void) {
	// TODO: Adicionar um callback para informar quando foi recebido um pacote?

	return enc28j60_read(uip_buf, UIP_BUFSIZE);
}

/*---------------------------------------------------------------------------*/
static void
pollhandler(void)
{
	process_poll(&ethernet_process);
	int len = ethernet_poll();

	if(len > 0) {
		PRINTF("\n* Packet Received on Ethernet!\n");

		uip_len = len - sizeof(struct uip_eth_hdr);
		// Select this interface
		uip_ds6_select_netif(UIP_DEFAULT_INTERFACE_ID);
		// Packets like NS have link layer address inside (options field),
		// so we need to adjust them
		mac_translateIPLinkLayer(ll_802154_type);

		/* We don't want Broadcasts or garbage! */
		if(uip_ipaddr_cmp(&BASE_BUF->dest, broadcast_ethaddr)) {
			PRINTF("\n* Received Ethernet Broadcast frame. Dropping it!\n");
			uip_clear_buf();
			return;
		}

#if NETSTACK_CONF_WITH_IPV6
		/* Only accept IPv6 */
		if(BASE_BUF->type == uip_htons(UIP_ETHTYPE_IPV6)) {
			PRINTF("* Ethernet frame received to ");
			PRINTLLADDR((const uip_lladdr_t *)&BASE_BUF->dest);
			PRINTF(" from ");
			PRINTLLADDR((const uip_lladdr_t *)&BASE_BUF->src);
			PRINTF("\n");

			tcpip_input();
		} else
#endif /* NETSTACK_CONF_WITH_IPV6 */
			uip_clear_buf();
	}
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ethernet_process, ev, data)
{
	PROCESS_POLLHANDLER(pollhandler());

	PROCESS_BEGIN();
	PRINTF("* Ethernet Driver process starting...\n");

	process_poll(&ethernet_process);
	PROCESS_WAIT_UNTIL(ev == PROCESS_EVENT_EXIT);


	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
