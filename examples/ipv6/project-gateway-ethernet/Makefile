CONTIKI_PROJECT=gateway
all: $(CONTIKI_PROJECT)
#APPS=

CONTIKI=../../..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS += -g
#PROJECT_SOURCEFILES += $(CONTIKI)/../core/net/ipv4/uip-neighbor.c $(CONTIKI)/../core/net/ipv4/uip_arp.c ethernet-drv.c ethernet-dev.c
PROJECT_SOURCEFILES += tools/sicslow_ethernet.c enc28j60_spi_arch.c $(CONTIKI)/../dev/enc28j60/enc28j60.c ethernet-drv.c ethernet-dev.c

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RPL = 0


# 0 - Sem optimização (-O0)
# 1 - Optimização (-Os)
# x - Optimização (-O2)
SMALL=0

include $(CONTIKI)/Makefile.include


