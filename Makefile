CONTIKI_PROJECT = node8 udp_server udp_clientF
all: $(CONTIKI_PROJECT)

CONTIKI_WITH_IPV6 = 1

CONTIKI = ../contiki

CFLAGS += -DPROJECT_CONF_H=\"project_conf.h\"


include $(CONTIKI)/Makefile.include