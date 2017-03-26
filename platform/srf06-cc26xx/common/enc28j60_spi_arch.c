/*
 * enc_spi_stubs.c
 *
 *  Created on: Mar 24, 2017
 *      Author: user
 */

#include <stdint.h>

#include "enc28j60_spi_arch.h"
#include "ti-lib.h"
#include "common/board-spi.h"
#include "launchpad/cc1310/board.h"


/* ENC28J60 architecture-specific SPI functions that are called by the
   enc28j60 driver and must be implemented by the platform code */

void enc28j60_arch_spi_deselect(void);
void enc28j60_arch_spi_select(void);


void enc28j60_arch_spi_init(void){
	  uint32_t buf;

	  /* First, make sure the SERIAL PD is on */
	  ti_lib_prcm_power_domain_on(PRCM_DOMAIN_SERIAL);
	  while((ti_lib_prcm_power_domain_status(PRCM_DOMAIN_SERIAL)
	         != PRCM_DOMAIN_POWER_ON));

	  /* Enable clock in active mode */
	  ti_lib_rom_prcm_peripheral_run_enable(PRCM_PERIPH_SSI0);
	  ti_lib_prcm_load_set();
	  while(!ti_lib_prcm_load_get());

	  /* SPI configuration */
	  ti_lib_ssi_int_disable(SSI0_BASE, SSI_RXOR | SSI_RXFF | SSI_RXTO | SSI_TXFF);
	  ti_lib_ssi_int_clear(SSI0_BASE, SSI_RXOR | SSI_RXTO);
	  // TODO: passar o bitrate e o 8 por arg
	  ti_lib_ssi_config_set_exp_clk(SSI0_BASE, ti_lib_sys_ctrl_clock_get(),
	                                    SSI_FRF_MOTO_MODE_0,
	                                    SSI_MODE_MASTER, ENC28J60_BITRATE, ENC28J60_DATAWIDTH);
	  ti_lib_rom_ioc_pin_type_ssi_master(SSI0_BASE, BOARD_IOID_SPI_MISO,
	                                     BOARD_IOID_SPI_MOSI, IOID_UNUSED, BOARD_IOID_SPI_CLK_FLASH);
	  ti_lib_ssi_enable(SSI0_BASE);

	  /* Get rid of residual data from SSI port */
	  while(ti_lib_ssi_data_get_non_blocking(SSI0_BASE, &buf));


	  	ti_lib_ioc_pin_type_gpio_output(BOARD_IOID_CS);

	  	/* Default output to clear chip select */
	  	enc28j60_arch_spi_deselect();
}

uint8_t enc28j60_arch_spi_write(uint8_t output){
	  int len = 1;
	  while(len > 0) {
	    uint32_t ul;

	    ti_lib_ssi_data_put(SSI0_BASE, output);
	    ti_lib_ssi_data_get(SSI0_BASE, &ul);
	    len--;
	  }

	  return 1;
}

uint8_t enc28j60_arch_spi_read(void){
	int len = 1;
	uint32_t ul;
	  while(len > 0) {


	    if(!ti_lib_ssi_data_put_non_blocking(SSI0_BASE, 0)) {
	      /* Error */
	      return false;
	    }
	    ti_lib_ssi_data_get(SSI0_BASE, &ul);

	    len--;
	  }

	  return (uint8_t)ul;
}

void enc28j60_arch_spi_select(void){
	ti_lib_gpio_clear_dio(BOARD_IOID_CS);
}

void enc28j60_arch_spi_deselect(void){
	ti_lib_gpio_set_dio(BOARD_IOID_CS);
}
