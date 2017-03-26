/*
 * ethernet-dev.c
 *
 */

#include "ethernet-dev.h"
#include "tools/sicslow_ethernet.h"


//#define DEBUG DEBUG_NONE
//#define DEBUG DEBUG_PRINT
#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"


#define BASE_BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define IPBUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])



/*---------------------------------------------------------------------------*/
static void build_ethaddr(const uip_lladdr_t *lladdr) {
	// Destination is multicast?
	if(IPBUF->destipaddr.u8[0] == 0xFF) {
	    /* Multicast. */
		/* Build the destination address as defined by the RFC 2464 */
		BASE_BUF->dest.addr[0] = 0x33;
		BASE_BUF->dest.addr[1] = 0x33;
		BASE_BUF->dest.addr[2] = 0xFF;
		BASE_BUF->dest.addr[3] = IPBUF->destipaddr.u8[13];
		BASE_BUF->dest.addr[4] = IPBUF->destipaddr.u8[14];
		BASE_BUF->dest.addr[5] = IPBUF->destipaddr.u8[15];
	} else {
		/* Destination is Unicast - just convert and copy the Neighbor ll address */
		translate_lowpan_to_eth(&BASE_BUF->dest, lladdr);
	}

	/* Just convert and copy our ll address */
	translate_lowpan_to_eth(&BASE_BUF->src, &uip_lladdr);

	BASE_BUF->type = UIP_HTONS(UIP_ETHTYPE_IPV6);
}

/*---------------------------------------------------------------------------*/
static uint8_t outputfunc(const uip_lladdr_t *a) {
	PRINTF("* ethernet-dev outputfunc: Processing packet to send!\n");

	// Set link layer headers
	build_ethaddr(a);

	// Packets with NS have link layer address inside (options field),
	// so we need to adjust them
	uint8_t ret = mac_translateIPLinkLayer(ll_8023_type);
	if(ret != 0) {
		PRINTF("* ethernet-dev outputfunc: Error translating ll addr: %d\n", ret);
		PRINTF("MAC: ");
		PRINTLLADDR(a);
		PRINTF("\n");
		return -1;
	}

	PRINTF("* Sending packet over Ethernet from ");
    PRINT6ADDR(&IPBUF->srcipaddr);
    PRINTF(" to ");
    PRINT6ADDR(&IPBUF->destipaddr);
    PRINTF("\n");

	return ethernet_send(uip_buf, uip_len + UIP_LLH_LEN);
}

/*---------------------------------------------------------------------------*/
void ethernet_dev_init(void)
{
	// set our output function
	tcpip_set_outputfunc(outputfunc);

	// start ethernet driver
	ethernet_drv_init();
}

