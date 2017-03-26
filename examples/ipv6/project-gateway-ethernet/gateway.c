#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include "mac.h"

#include "ethernet-dev.h"

#include <string.h>
#include <stdio.h>


#include "dev/leds.h"
#include "board-peripherals.h"
#include "ti-lib.h"




#define MACDEBUG 1

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#endif

#define PING6_NB 500
#define PING6_DATALEN 16

#define UIP_IP_BUF					((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF				((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
//
#define IP_LINK_LOCAL_PREFIX_BYTE	(0x21)
//
#define IPV6_CONF_ADDR_8			0xA

static struct etimer ping6_periodic_timer;
static uint8_t count = 0;
static char command[20];
static uint16_t addr[8];
uip_ipaddr_t dest_addr;


PROCESS(gateway_process, "Gateway process");
AUTOSTART_PROCESSES(&gateway_process);


/*---------------------------------------------------------------------------*/
static uint8_t
ping6handler(process_event_t ev, process_data_t data)
{
  if(count == 0){
#if MACDEBUG
    // Setup destination address.
	  // fe80::212:7401:1:101
    addr[0] = 0xFb80;
    addr[4] = 0x0212;
    addr[5] = 0x7408;
    addr[6] = 0x0008;
    addr[7] = 0x0808;
    uip_ip6addr(&dest_addr, addr[0], addr[1],addr[2],
                addr[3],addr[4],addr[5],addr[6],addr[7]);

    // Set the command to fool the 'if' below.
    memcpy(command, (void *)"ping6", 5);

#else
/* prompt */
    printf("> ");
    /** \note the scanf here is blocking (the all stack is blocked waiting
     *  for user input). This is far from ideal and could be improved
     */
    scanf("%s", command);

    if(strcmp(command,"ping6") != 0){
      PRINTF("> invalid command\n");
      return 0;
    }

    if(scanf(" %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
             &addr[0],&addr[1],&addr[2],&addr[3],
             &addr[4],&addr[5],&addr[6],&addr[7]) == 8){

      uip_ip6addr(&dest_addr, addr[0], addr[1],addr[2],
                  addr[3],addr[4],addr[5],addr[6],addr[7]);
    } else {
      PRINTF("> invalid ipv6 address format\n");
      return 0;
    }
#endif

  }

  if((strcmp(command,"ping6") == 0) && (count < PING6_NB)){

    UIP_IP_BUF->vtc = 0x60;
    UIP_IP_BUF->tcflow = 1;
    UIP_IP_BUF->flow = 0;
    UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
    UIP_IP_BUF->ttl = uip_ds6_if.cur_hop_limit;
    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &dest_addr);
    // TODO: Para não enviar pings com endereço ::, deve seleccionar a interface default (ethernet)
    uip_ds6_select_netif(UIP_DEFAULT_INTERFACE_ID);
    uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

    UIP_ICMP_BUF->type = ICMP6_ECHO_REQUEST;
    UIP_ICMP_BUF->icode = 0;
    /* set identifier and sequence number to 0 */
    memset((uint8_t *)UIP_ICMP_BUF + UIP_ICMPH_LEN, 1, 4);
    /* put one byte of data */
    memset((uint8_t *)UIP_ICMP_BUF + UIP_ICMPH_LEN + UIP_ICMP6_ECHO_REQUEST_LEN,
           count, PING6_DATALEN);


    uip_len = UIP_ICMPH_LEN + UIP_ICMP6_ECHO_REQUEST_LEN + UIP_IPH_LEN + PING6_DATALEN;
    UIP_IP_BUF->len[0] = (uint8_t)((uip_len - 40) >> 8);
    UIP_IP_BUF->len[1] = (uint8_t)((uip_len - 40) & 0x00FF);

    UIP_ICMP_BUF->icmpchksum = 0;
    UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();


    PRINTF("Sending Echo Request to");
    PRINT6ADDR(&UIP_IP_BUF->destipaddr);
    PRINTF("from");
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("\n");
    UIP_STAT(++uip_stat.icmp.sent);

    tcpip_ipv6_output();

    count++;
    etimer_set(&ping6_periodic_timer, 5 * CLOCK_SECOND);
    return 1;
  }
  return 0;
}

static void init_default_interface(void) {
	uip_ipaddr_t loc_fipaddr;

	// Select ethernet interface
	uip_ds6_select_netif(UIP_DEFAULT_INTERFACE_ID);

	memset(uip_ds6_prefix_list, 0, sizeof(uip_ds6_prefix_list));
	memset(&uip_ds6_if, 0, sizeof(uip_ds6_if));
	// Set interface parameters
	uip_ds6_if.link_mtu = UIP_LINK_MTU;
	uip_ds6_if.cur_hop_limit = UIP_TTL;
	uip_ds6_if.base_reachable_time = UIP_ND6_REACHABLE_TIME;
	uip_ds6_if.reachable_time = uip_ds6_compute_reachable_time();
	uip_ds6_if.retrans_timer = UIP_ND6_RETRANS_TIMER;
	uip_ds6_if.maxdadns = UIP_ND6_DEF_MAXDADNS;

	  // Create link local address, prefix, multicast addresses, anycast addresses
	  uip_create_linklocal_prefix(&loc_fipaddr);
	  // TODO: Need to make it different from Radio interface. Is this the right way?
	  loc_fipaddr.u8[2] = IP_LINK_LOCAL_PREFIX_BYTE;
#if UIP_CONF_ROUTER
	  uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0, 0, 0, 0);
#else // UIP_CONF_ROUTER
	  uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0);
#endif // UIP_CONF_ROUTER
	  uip_ds6_set_addr_iid(&loc_fipaddr, &uip_lladdr);
	  uip_ds6_addr_add(&loc_fipaddr, 0, ADDR_AUTOCONF);

	  uip_create_linklocal_allnodes_mcast(&loc_fipaddr);
	  uip_ds6_maddr_add(&loc_fipaddr);
#if UIP_CONF_ROUTER
	  uip_create_linklocal_allrouters_mcast(&loc_fipaddr);
	  uip_ds6_maddr_add(&loc_fipaddr);
#endif

if(!UIP_CONF_IPV6_RPL) {
  uip_ipaddr_t ipaddr;
  int i;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, IPV6_CONF_ADDR_8);
  //uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_TENTATIVE);
  printf("* Ethernet: Tentative global IPv6 address ");
  for(i = 0; i < 7; ++i) {
	  printf("%02x%02x:",
		   ipaddr.u8[i * 2], ipaddr.u8[i * 2 + 1]);
  }
  printf("%02x%02x\n",
		 ipaddr.u8[7 * 2], ipaddr.u8[7 * 2 + 1]);
}


	PRINTF("* Ethernet Tentative link-local IPv6 address ");
	{
	uip_ds6_addr_t *lladdr;
	int i;
	lladdr = uip_ds6_get_link_local(-1);
	for(i = 0; i < 7; ++i) {
	  PRINTF("%02x%02x:", lladdr->ipaddr.u8[i * 2],
			 lladdr->ipaddr.u8[i * 2 + 1]);
	}
	PRINTF("%02x%02x\n", lladdr->ipaddr.u8[14], lladdr->ipaddr.u8[15]);
	}
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gateway_process, ev, data)
{

  uint8_t cont = 1;

  PROCESS_BEGIN();
  PRINTF("\n\n*Process PING6\n");

  // Initializes ethernet interface data structures
  init_default_interface();
  // Initializes the ethernet interface
  ethernet_dev_init();

  etimer_set(&ping6_periodic_timer, 10*CLOCK_SECOND);

  while(cont) {
    PROCESS_YIELD();

    printf("\nPing Handler\n");
    cont = ping6handler(ev, data);
    leds_toggle(LEDS_RED);
  }

  PRINTF("END Gateway\n");
  PROCESS_END();
}


#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

/* Some example here */
#pragma message(VAR_NAME_VALUE(NETSTACK_CONF_RADIO))
#pragma message(VAR_NAME_VALUE(NETSTACK_CONF_RDC))
#pragma message(VAR_NAME_VALUE(UIP_FALLBACK_INTERFACE))
#pragma message(VAR_NAME_VALUE(UIP_CONF_ROUTER))
#pragma message(VAR_NAME_VALUE(UIP_ND6_SEND_RA))
#pragma message(VAR_NAME_VALUE(LINKADDR_CONF_SIZE))
#pragma message(VAR_NAME_VALUE(UIP_LLH_LEN))
#pragma message(VAR_NAME_VALUE(UIP_IPH_LEN))
#pragma message(VAR_NAME_VALUE(UIP_CONF_BUFFER_SIZE))
#pragma message(VAR_NAME_VALUE(BOARD_IOID_CS))
#pragma message(VAR_NAME_VALUE(BOARD_IOID_SPI_CLK_FLASH))
#pragma message(VAR_NAME_VALUE(NBR_TABLE_CONF_MAX_NEIGHBORS))




/*---------------------------------------------------------------------------*/
