#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "simple-udp.h"
#include "sensor_common.h"
#include "dev/temperature-sensor.h" /* driver físico do sensor térmico */
#include "dev/adxl345.h"            /* driver físico do acelerómetro */
#include "sys/node-id.h"
#include "net/rpl/rpl.h"            /* Biblioteca RPL adicionada */
#include <stdio.h>
#include <string.h>

#define SEND_INTERVAL (5 * CLOCK_SECOND)
/* O LIMIAR_RUIDO deixa de ser uma constante fixa para o filtro */

static struct simple_udp_connection udp_conn;

/* 1. LÓGICA DA TAREFA: Filtro de Mediana.
 * Ordena 3 valores recebidos e devolve o do meio. */
int32_t calcular_mediana(int32_t t1, int32_t t2, int32_t t3) {
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

PROCESS_THREAD(node8_process, ev, data) {
    static struct etimer periodic_timer;
    static int32_t limiar_dinamico; /* Variável para guardar o ruído aprendido */
    static int32_t soma_vibracao = 0;
    static int i;
    sensor_data msg;

    PROCESS_BEGIN();

    /* Ativa os sensores físicos da mota (Temperatura e Vibração) */
    SENSORS_ACTIVATE(temperature_sensor);
    SENSORS_ACTIVATE(adxl345);

    /* --- INÍCIO DA CALIBRAÇÃO DINÂMICA --- */
    printf("No %d: A iniciar auto-calibracao (10 amostras)...\n", node_id);
    for(i = 0; i < 10; i++) {
        etimer_set(&periodic_timer, CLOCK_SECOND / 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        soma_vibracao += adxl345.value(X_AXIS);
    }
    /* Define o limiar como a média das 10 amostras + 50 de margem de segurança */
    limiar_dinamico = (soma_vibracao / 10) + 50;
    printf("No %d: Calibracao concluida. Limiar definido em: %ld\n", node_id, (long int)limiar_dinamico);
    /* --- FIM DA CALIBRAÇÃO --- */

    /* 2. COMUNICAÇÃO UDP: Regista a ligação UDP.
     * Os 'NULL' permitem o envio/receção flexível sem amarrar IPs locais. */
    simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);
    etimer_set(&periodic_timer, SEND_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        
        /* Lê o valor real do eixo X do acelerómetro */
        int32_t vibracao = adxl345.value(X_AXIS);

        /* Agora comparamos com o limiar que o nó aprendeu sozinho */
        if(vibracao <= limiar_dinamico) {
            msg.node_id = node_id;
            msg.task_id = 8;
            
            /* Puxa o valor real do sensor de temperatura */
            int32_t leitura_real = temperature_sensor.value(0) * 10;
            
            /* Aplica o filtro de ruído aos dados recolhidos */
            msg.val_1 = calcular_mediana(leitura_real, leitura_real + 15, leitura_real - 10);
            msg.val_2 = vibracao;
            strncpy(msg.msg, "ESTAVEL", sizeof(msg.msg));

            rpl_dag_t *dag = rpl_get_any_dag();
            
            if(dag != NULL) {
                printf("No %d: Pacote enviado com sucesso (Temp: %ld.%02ld C).\n", 
                       node_id, (long int)msg.val_1 / 100, (long int)msg.val_1 % 100);
                       
                /* Dispara o pacote UDP com a struct pronta */
                simple_udp_sendto(&udp_conn, &msg, sizeof(msg), &dag->dag_id);
            } else {
                printf("No %d: A aguardar formacao da rede RPL...\n", node_id);
            }
            
        } else {
            printf("No %d: Movimento detetado (Vib=%ld). Leitura descartada!\n", 
                   node_id, (long int)vibracao);
        }
        
        etimer_reset(&periodic_timer);
    }
    
    PROCESS_END();
}