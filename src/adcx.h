#pragma once

#include <stdint.h>

#define ADCX_CHANNELS   4
#define ADCX_AVGCOUNT   10

void ADCX_Init(uint8_t channels, uint8_t vref_mask);
void ADCX_Done(void);
void ADCX_Start(void);
void ADCX_Stop(void);
void ADCX_Reset(uint8_t index);
uint16_t ADCX_Vref(void);
uint16_t ADCX_Read(uint8_t index);
