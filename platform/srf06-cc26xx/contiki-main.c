/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
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
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc26xx-platforms
 * @{
 *
 * \defgroup cc26xx-srf-tag SmartRF+CC13xx/CC26xx EM, CC2650 SensorTag and LaunchPads
 *
 * This platform supports a number of different boards:
 * - A standard TI SmartRF06EB with a CC26xx EM mounted on it
 * - A standard TI SmartRF06EB with a CC1310 EM mounted on it
 * - The new TI SensorTag2.0
 * - The TI CC2650 LaunchPad
 * - The TI CC1310 LaunchPad
 * @{
 */
#include "ti-lib.h"
#include "contiki.h"
#include "contiki-net.h"
#include "leds.h"
#include "lpm.h"
#include "gpio-interrupt.h"
#include "dev/watchdog.h"
#include "dev/oscillators.h"
#include "ieee-addr.h"
#include "vims.h"
#include "dev/cc26xx-uart.h"
#include "dev/soc-rtc.h"
#include "rf-core/rf-core.h"
#include "sys_ctrl.h"
#include "uart.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/node-id.h"
#include "lib/random.h"
#include "lib/sensors.h"
#include "button-sensor.h"
#include "dev/serial-line.h"
#include "net/mac/frame802154.h"

#include "driverlib/driverlib_release.h"

#include <stdio.h>

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
#if UIP_CONF_DS6_INTERFACES_NUMBER > 1
#include "core/net/eth/ethernet-defs.h"
#include "core/net/eth/ethernet-dev.h"
#include "tools/sicslow_ethernet.h"


// used to fill with adapted ll address from 802.15.4
// ethernet MAC address
// TODO: Currently we use it hardcoded.
// TODO: May be its a better idea generate it based on hardware serial number or someting like that
uip_eth_addr ethernet_if_addr;

// The default Neighbor
// TODO: Currently we use it hardcoded. In the future it needs to be calculated based on the ethernet's link-local address
uip_ipaddr_t default_neighbor_ip6_addr = {
		.u16[0] = 0x80FE,
		.u16[1] = 0x0021,
		.u16[2] = 0x0,
		.u16[3] = 0x0,
		.u16[4] = 0x1200,
		.u16[5] = 0xFF4B,
		.u16[6] = 0x27FE,
		.u16[7] = 0x0EC5};
#endif /* UIP_CONF_DS6_INTERFACES_NUMBER > 1 */
/*---------------------------------------------------------------------------*/
unsigned short node_id = 0;
/*---------------------------------------------------------------------------*/
/** \brief Board specific iniatialisation */
void board_init(void);
/*---------------------------------------------------------------------------*/




static void
fade(unsigned char l)
{
  volatile int i;
  int k, j;
  for(k = 0; k < 800; ++k) {
    j = k > 400 ? 800 - k : k;

    leds_on(l);
    for(i = 0; i < j; ++i) {
      __asm("nop");
    }
    leds_off(l);
    for(i = 0; i < 400 - j; ++i) {
      __asm("nop");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;
  uint8_t ext_addr[8];
  radio_value_t val = 0;

  ieee_addr_cpy_to(ext_addr, 8);

  short_addr = ext_addr[7];
  short_addr |= ext_addr[6] << 8;

  /* In case the OUI contain a broadcast bit, we mask that out here. */
  ext_addr[0] = (ext_addr[0] & 0xfe);
  /* Set the U/L bit, in order to create a locally administered MAC address
     as stated by the RFC 2464 */
  ext_addr[0] = (ext_addr[0] | 0x02);

  // Just to make it compatible with our ethernet translation.
  ext_addr[3] = 0xFF;
  ext_addr[4] = 0xFE;

#if UIP_CONF_DS6_INTERFACES_NUMBER > 1
  translate_lowpan_to_eth(&ethernet_if_addr, (uip_lladdr_t *)&ext_addr);
#endif /* UIP_CONF_DS6_INTERFACES_NUMBER > 1 */

  /* Populate linkaddr_node_addr. Maintain endianness */
  memcpy(&linkaddr_node_addr, &ext_addr[8 - LINKADDR_SIZE], LINKADDR_SIZE);

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, RF_CORE_CHANNEL);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &val);
  PRINTF(" RF: Channel %d\n", val);

  int i;
  PRINTF("* Link layer addr: ");
  for(i = 0; i < LINKADDR_SIZE - 1; i++) {
    PRINTF("%02x:", linkaddr_node_addr.u8[i]);
  }
  PRINTF("%02x\n", linkaddr_node_addr.u8[i]);

  /* also set the global node id */
  node_id = short_addr;
  PRINTF(" Node ID: %d\n", node_id);
}
/*---------------------------------------------------------------------------*/
/**
 *  \brief Initialises the ethernet interface
 */
#if UIP_CONF_DS6_INTERFACES_NUMBER > 1
static void init_eth_if(void) {
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
  PRINTF("* Ethernet link-local IPv6 address: ");
  PRINT6ADDR(&loc_fipaddr);
  PRINTF("\n");

  uip_create_linklocal_allnodes_mcast(&loc_fipaddr);
  uip_ds6_maddr_add(&loc_fipaddr);
#if UIP_CONF_ROUTER
  uip_create_linklocal_allrouters_mcast(&loc_fipaddr);
  uip_ds6_maddr_add(&loc_fipaddr);
#endif

  // Set site-local address and prefix
  uip_ip6addr(&loc_fipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
#if UIP_CONF_ROUTER
  // TODO: This is correct, but because nbr-table only supports one prefix per MAC address we can't add this prefix - link-local prefix have the same MAC address
  //uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0, 0, 0, 0);
#else // UIP_CONF_ROUTER
  // TODO: This is correct, but because nbr-table only supports one prefix per MAC address we can't add this prefix - link-local prefix have the same MAC address
  //uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0);
#endif // UIP_CONF_ROUTER
  uip_ip6addr(&loc_fipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, IPV6_CONF_ADDR_8);
  uip_ds6_addr_add(&loc_fipaddr, 0, ADDR_TENTATIVE);

  PRINTF("* Ethernet site-local IPv6 address: ");
  PRINT6ADDR(&loc_fipaddr);
  PRINTF("\n");

  // Initializes the ethernet interface
  ethernet_dev_init();
}
#endif /* UIP_CONF_DS6_INTERFACES_NUMBER > 1 */
/*---------------------------------------------------------------------------*/
/**
 * \brief Main function for CC26xx-based platforms
 *
 * The same main() is used for all supported boards
 */
int
main(void)
{
  /* Enable flash cache and prefetch. */
  ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_ENABLED);
  ti_lib_vims_configure(VIMS_BASE, true, true);

  ti_lib_int_master_disable();

  /* Set the LF XOSC as the LF system clock source */
  oscillators_select_lf_xosc();

  lpm_init();

  board_init();

  gpio_interrupt_init();

  leds_init();

  /*
   * Disable I/O pad sleep mode and open I/O latches in the AON IOC interface
   * This is only relevant when returning from shutdown (which is what froze
   * latches in the first place. Before doing these things though, we should
   * allow software to first regain control of pins
   */
  ti_lib_pwr_ctrl_io_freeze_disable();

  fade(LEDS_RED);

  ti_lib_int_master_enable();

  soc_rtc_init();
  clock_init();
  rtimer_init();

  watchdog_init();
  process_init();

  random_init(0x1234);

  /* Character I/O Initialisation */
#if CC26XX_UART_CONF_ENABLE
  cc26xx_uart_init();
#endif

  ///////serial_line_init();

  PRINTF("Starting " CONTIKI_VERSION_STRING "\n");
  PRINTF("With DriverLib v%u.%u\n", DRIVERLIB_RELEASE_GROUP,
         DRIVERLIB_RELEASE_BUILD);
  PRINTF(BOARD_STRING "\n");
  PRINTF("IEEE 802.15.4: %s, Sub-GHz: %s, BLE: %s, Prop: %s\n",
         ti_lib_chipinfo_supports_ieee_802_15_4() == true ? "Yes" : "No",
         ti_lib_chipinfo_chip_family_is_cc13xx() == true ? "Yes" : "No",
         ti_lib_chipinfo_supports_ble() == true ? "Yes" : "No",
         ti_lib_chipinfo_supports_proprietary() == true ? "Yes" : "No");

  process_start(&etimer_process, NULL);
  ctimer_init();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  fade(LEDS_YELLOW);

  PRINTF(" Net: ");
  PRINTF("%s\n", NETSTACK_NETWORK.name);
  PRINTF(" MAC: ");
  PRINTF("%s\n", NETSTACK_MAC.name);
  PRINTF(" RDC: ");
  PRINTF("%s", NETSTACK_RDC.name);

  if(NETSTACK_RDC.channel_check_interval() != 0) {
    PRINTF(", Channel Check Interval: %u ticks",
           NETSTACK_RDC.channel_check_interval());
  }
  PRINTF("\n");

  netstack_init();

  set_rf_params();

#if NETSTACK_CONF_WITH_IPV6
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  queuebuf_init();
  process_start(&tcpip_process, NULL);

#if UIP_CONF_DS6_INTERFACES_NUMBER > 1
  init_eth_if();
#endif /* UIP_CONF_DS6_INTERFACES_NUMBER > 1 */

  //
#if UIP_CONF_DS6_INTERFACES_NUMBER > 1
  uip_ds6_select_netif(UIP_RADIO_INTERFACE_ID);
#endif /* UIP_CONF_DS6_INTERFACES_NUMBER > 1 */

#endif /* NETSTACK_CONF_WITH_IPV6 */

  fade(LEDS_GREEN);

  process_start(&sensors_process, NULL);

  autostart_start(autostart_processes);

  // TODO: repor?
  //watchdog_start();

  fade(LEDS_ORANGE);

  while(1) {
    uint8_t r;
    do {
      r = process_run();
      // TODO: repor?
      //watchdog_periodic();
    } while(r > 0);

    /* Drop to some low power mode */
    lpm_drop();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
