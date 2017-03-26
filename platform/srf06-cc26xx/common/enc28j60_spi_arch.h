#ifndef ENC28J60_SPI_ARCH_H
#define ENC28J60_SPI_ARCH_H

#define ENC28J60_BITRATE						10000000 /* 10Mbps */
#define ENC28J60_DATAWIDTH						8

/* ENC28J60 architecture-specific SPI functions that are called by the
   enc28j60 driver and must be implemented by the platform code */

void enc28j60_arch_spi_init(void);
uint8_t enc28j60_arch_spi_write(uint8_t data);
uint8_t enc28j60_arch_spi_read(void);
void enc28j60_arch_spi_select(void);
void enc28j60_arch_spi_deselect(void);

#endif /* ENC28J60_SPI_ARCH_H */
