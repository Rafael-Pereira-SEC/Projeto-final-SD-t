#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/simple-udp.h"
#include "net/rpl/rpl.h"
#include "sys/node-id.h"

#include "sensor_common.h"
#include "dev/tmp102.h"
#include "dev/adxl345.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SEND_INTERVAL (5 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static uip_ipaddr_t sink_ip;

/* Dá ao cliente um endereço dentro da rede aaaa::/64 */
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}



/* Calcula a mediana de 3 valores */
static int32_t
calcular_mediana(int32_t t1, int32_t t2, int32_t t3)
{
  int32_t arr[3] = {t1, t2, t3};
  int i, j;

  for(i = 0; i < 2; i++) {
    for(j = 0; j < 2 - i; j++) {
      if(arr[j] > arr[j + 1]) {
        int32_t temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }

  return arr[1];
}

PROCESS(node8_process, "Node 8 - Filtro de Ruido");
AUTOSTART_PROCESSES(&node8_process);

PROCESS_THREAD(node8_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer startup_timer;
  static int32_t limiar_dinamico;
  static int32_t soma_vibracao = 0;
  static int i;

  static sensor_data msg;

  PROCESS_BEGIN();

  set_global_address();

  /* Define o Sink comum */
  uip_ip6addr(&sink_ip, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);

  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);

  printf("No %u iniciado. Destino Sink aaaa::1\n", node_id);

  /* Espera para a rede RPL estabilizar */
  etimer_set(&startup_timer, CLOCK_SECOND * 60);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&startup_timer));

  printf("No %u: rede estabilizada.\n", node_id);

  SENSORS_ACTIVATE(tmp102);
  SENSORS_ACTIVATE(adxl345);

  printf("No %u: A iniciar auto-calibracao...\n", node_id);

  for(i = 0; i < 10; i++) {
    etimer_set(&periodic_timer, CLOCK_SECOND / 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    soma_vibracao += abs(adxl345.value(X_AXIS));
  }

  limiar_dinamico = (soma_vibracao / 10) + 50;

  printf("No %u: Calibracao concluida. Limiar=%ld\n",
         node_id,
         (long)limiar_dinamico);

  etimer_set(&periodic_timer, SEND_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    int32_t vibracao = abs(adxl345.value(X_AXIS));

    if(vibracao <= limiar_dinamico) {
      int32_t leitura_real;

      msg.node_id = node_id;
      msg.task_id = 8;

      leitura_real = tmp102.value(TMP102_READ);

      msg.val_1 = calcular_mediana(leitura_real,
                                   leitura_real + 15,
                                   leitura_real - 10);

      msg.val_2 = vibracao;
      strncpy(msg.msg, "ESTAVEL", sizeof(msg.msg) - 1);
      msg.msg[sizeof(msg.msg) - 1] = '\0';

      if(rpl_get_any_dag() != NULL) {
        printf("No %u: pacote enviado | Temp=%ld.%02ld C | Vib=%ld\n",
               node_id,
               (long)msg.val_1 / 100,
               (long)msg.val_1 % 100,
               (long)msg.val_2);

        simple_udp_sendto(&udp_conn, &msg, sizeof(msg), &sink_ip);
      } else {
        printf("No %u: sem DAG RPL. Pacote nao enviado.\n", node_id);
      }

    } else {
      printf("No %u: movimento detetado | Vib=%ld | leitura descartada\n",
             node_id,
             (long)vibracao);
    }

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
