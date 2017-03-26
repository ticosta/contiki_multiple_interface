CONTIKI_PROJECT=gateway
all: $(CONTIKI_PROJECT)
#APPS=


CONTIKI=../../..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
# TODO: retirar
CFLAGS += -g
#PROJECT_SOURCEFILES += $(CONTIKI)/../core/net/ipv4/uip-neighbor.c $(CONTIKI)/../core/net/ipv4/uip_arp.c ethernet-drv.c ethernet-dev.c
PROJECT_SOURCEFILES += tools/sicslow_ethernet.c enc28j60_spi_arch.c $(CONTIKI)/../dev/enc28j60/enc28j60.c ethernet-drv.c ethernet-dev.c

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RPL = 0


# 0 - Sem optimização (-O0)
# 1 - Optimização (-Os)
# x - Optimização (-O2)
SMALL=0






# automatically build RESTful resources
REST_RESOURCES_DIR = ./resources
ifndef TARGET
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
ifeq ($(TARGET), native)
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c' ! -name 'res-plugtest*'))
endif
endif

PROJECTDIRS += $(REST_RESOURCES_DIR)
PROJECT_SOURCEFILES += $(REST_RESOURCES_FILES)

# REST Engine shall use Erbium CoAP implementation
APPS += er-coap er-http
APPS += rest-engine

include $(CONTIKI)/Makefile.include


