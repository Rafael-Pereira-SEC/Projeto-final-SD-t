#ifndef SENSOR_COMMON_H
#define SENSOR_COMMON_H

#include "contiki.h"
#include <stdint.h>

typedef struct {
  uint16_t node_id;
  uint16_t task_id;   
  int32_t val_1;     // Valor da temperatura
  int32_t val_2;    // Magnitude da vibração
  char msg[20];     
} sensor_data;

#define UDP_PORT 1234

#endif