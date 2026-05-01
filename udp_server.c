#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "simple-udp.h"
#include "net/rpl/rpl.h"
#include "sensor_common.h"
#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_server_process, "Coordenador Central (Sink)");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/

static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  if(datalen < sizeof(sensor_data)) return;

  sensor_data *msg = (sensor_data *)data;

  printf("\n[SINK] Recebido do No %d | Tarefa %d: ", msg->node_id, msg->task_id);

  /* Lógica de Interpretação por Task_ID */
  switch(msg->task_id) {

    case 0: // Média Móvel (Térmico + Bateria)
      printf("\n >> ALERTA: Sobreaquecimento em no critico!\n");
      printf(" >> Media Temp: %ld.%02ld C | V_Batt: %ld.%03ld V\n", 
              msg->val_1 / 100, msg->val_1 % 100, msg->val_2 / 1000, msg->val_2 % 1000);
      break;

    case 1: // Deteção de Ignição
      printf("\n >> PERIGO: Incendio com colapso estrutural detetado!\n");
      printf(" >> Delta Temp: +%ld.%02ld C | Magnitude Vibracao: %ld\n", 
              msg->val_1 / 100, msg->val_1 % 100, msg->val_2);
      break;

    case 2: // Peak-hold (Choque)
      printf("\n >> LOG: Monitorizacao de impacto em no saudavel.\n");
      printf(" >> Pico de Vibracao: %ld G | V_Batt: %ld.%03ld V\n", 
              msg->val_1, msg->val_2 / 1000, msg->val_2 % 1000);
      break;

    case 3: // Atrito Mecânico
      printf("\n >> MANUTENCAO: Desgaste critico de rolamento/motor.\n");
      printf(" >> Temp Atual: %ld.%02ld C | Vibracao Ativa: %ld\n", 
              msg->val_1 / 100, msg->val_1 % 100, msg->val_2);
      break;

    case 4: // Análise de Descarga
      printf("\n >> ESTUDO: Degradacao de bateria com calor.\n");
      printf(" >> Queda de Tensao: %ld mV | Temp Media: %ld.%02ld C\n", 
              msg->val_1, msg->val_2 / 100, msg->val_2 % 100);
      break;

    case 5: // Modo de Repouso Ativo
      printf("\n >> DIAGNOSTICO: Leitura de no estatico em armazem.\n");
      printf(" >> Tensao Bateria: %ld.%03ld V | Status: Estatico\n", 
              msg->val_1 / 1000, msg->val_1 % 1000);
      break;

    case 6: // Intrusão Térmica
      printf("\n >> SEGURANCA: Presenca humana ou maquinaria detetada!\n");
      printf(" >> Gatilho Vibracao: %ld | Variacao Termica: %ld\n", 
              msg->val_1, msg->val_2);
      break;

    case 7: // Registo de Queda Livre
      printf("\n >> ALERTA: Queda de carga ou roubo do mote!\n");
      printf(" >> Status: Queda Livre Detectada | V_Batt no Impacto: %ld mV\n", msg->val_2);
      break;

    case 8: // Filtro de Ruído Físico 
      printf("\n >> LAB: Garantia de precisao termica (Mediana).\n");
      printf(" >> Temperatura Filtrada: %ld.%02ld C | Ambiente: Estavel\n", 
              msg->val_1 / 100, msg->val_1 % 100);
      break;

    case 9: // Relatório de Estabilidade
      printf("\n >> CERTIFICACAO: Relatorio periodico de 5 min.\n");
      printf(" >> Media T: %ld.%02ld C | Oscilacao Accel: %ld\n", 
              msg->val_1 / 100, msg->val_1 % 100, msg->val_2);
      break;

    default:
      printf("\n >> Tarefa desconhecida ou erro de ID.\n");
  }

  /* Validação de Inteligência: Correlacionar Falha vs Evento */
  if(msg->val_1 > 100000 || msg->val_1 < -50000) {
    printf(" [!] CUIDADO: Valores fora da escala fisica. Possivel falha de sensor no No %d.\n", msg->node_id);
  } else {
    printf(" [+] Validacao: Dados correlacionados com sucesso.\n");
  }
  printf("----------------------------------------------------------\n");
}
// ----------------------------------------------------------------------

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  uip_ipaddr_t ipaddr;
  struct uip_ds6_addr *root_if;

  PROCESS_BEGIN();

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  root_if = uip_ds6_addr_lookup(&ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
    uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &ipaddr, 64);
  }

  /* Inicializa a escuta UDP */
  simple_udp_register(&udp_conn, UDP_PORT, NULL,
                      UDP_PORT, udp_rx_callback);

  printf("SINK iniciado. Aguardar Tasks 0 - 9.\n");

  PROCESS_END();
}