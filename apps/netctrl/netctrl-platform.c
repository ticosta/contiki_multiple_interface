#include "netctrl-platform.h"
#include "netctrl.h"
#include "contiki-net.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define UIP_ETH_BUF                        ((struct uip_eth_hdr *)&uip_buf)
#define UIP_IP_BUF                         ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF                        ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])

static struct uip_udp_conn *netctrl_udp_conn = NULL;
/*---------------------------------------------------------------------------*/
void
netctrl_send_message(uint8_t *data, uint16_t length)
{
  uip_udp_packet_sendto(netctrl_udp_conn, data, length,
   &UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport);
}
/*---------------------------------------------------------------------------*/
void netctrl_init_network() {
  /* new connection with remote host */
  netctrl_udp_conn = udp_new(NULL, 0, NULL);
  udp_bind(netctrl_udp_conn, UIP_HTONS(NETCTRL_DEFAULT_LISTEN_PORT));
  PRINTF("Netctrl listening on port %u\n", ntohs(netctrl_udp_conn->lport));
}
/*---------------------------------------------------------------------------*/
// TODO Improve this function. Currently its easy to get duplicated values. For the same node, its result needs to be alwais the same, so it can not be based on time.
uint32_t netctrl_calc_node_hash() {
  uint32_t sum = 0;

  int i;
  for(i = sizeof(UIP_IP_BUF->srcipaddr) - 1; i >= 0; i--) {
    sum += UIP_IP_BUF->srcipaddr.u8[i];
  }

  for(i = sizeof(UIP_ETH_BUF->src.addr) - 1; i >= 0; i--) {
    sum += UIP_ETH_BUF->src.addr[i];
  }

  return (~sum) + 1;
}
/*---------------------------------------------------------------------------*/
void * netctrl_get_nodeId() {
	return (void *)&UIP_IP_BUF->srcipaddr;
}
