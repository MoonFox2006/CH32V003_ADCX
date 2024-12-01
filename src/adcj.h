#pragma once

#include <stdint.h>

#define ADCJ_MEASURES   4

void ADCJ_Init(uint8_t channel);
void ADCJ_Done(void);
uint16_t ADCJ_Read(void);
