#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Garante que o teu nó atua como Router e reencaminha pacotes */
#undef UIP_CONF_IPV6_RPL
#define UIP_CONF_IPV6_RPL 1

#undef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER 1

/* Mantém a compatibilidade com o rádio da rede dos teus colegas */
#undef UNICAST
#define UNICAST 1

#undef UIP_CONF_ND6_DEF_MAXDADNS
#define UIP_CONF_ND6_DEF_MAXDADNS 0

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC nullrdc_driver

#undef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8

#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC csma_driver

#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER framer_802154

#undef ENERGEST_CONF_ON
#define ENERGEST_CONF_ON 1

#endif