#include <string.h>
#include <ch32v00x.h>
#include <ch32v00x_rcc.h>
#include <ch32v00x_gpio.h>
#include <ch32v00x_adc.h>
#include "adcx.h"
#include "bitarray.h"

#if ADCX_CHANNELS > 4
    #error "Too many ADCX channels!"
#endif

typedef struct __attribute__((__packed__)) _adcx_t {
    uint8_t samples[BITARRAY_SIZE(ADCX_AVGCOUNT)];
    uint16_t sum;
    uint8_t len, pos;
} adcx_t;

static volatile adcx_t _adcx[ADCX_CHANNELS];
static volatile uint16_t _adcx_vref = 0;
static uint8_t _adcx_vref_mask = 0;

void ADCX_Init(uint8_t channels, uint8_t vref_mask) {
    {
        uint32_t periph;

        periph = RCC_APB2Periph_ADC1;
        if (channels & 0x03) // Channel_0 | Channel_1 (PA2, PA1)
            periph |= RCC_APB2Periph_GPIOA;
        if (channels & 0x04) // Channel_2 (PC4)
            periph |= RCC_APB2Periph_GPIOC;
        if (channels & 0xF8) // Channel_3..Channel_7 (PD2, PD3, PD5, PD6, PD4)
            periph |= RCC_APB2Periph_GPIOD;

        RCC_APB2PeriphClockCmd(periph, ENABLE);
    }

    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    {
        GPIO_InitTypeDef GPIO_InitStructure = { 0 };

        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
        if (channels & 0x03) { // Channel_0 | Channel_1 (PA2, PA1)
            GPIO_InitStructure.GPIO_Pin = 0;
            if (channels & 0x01) // Channel_0 (PA2)
                GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_2;
            if (channels & 0x02) // Channel_1 (PA1)
                GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_1;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
        }
        if (channels & 0x04) { // Channel_2 (PC4)
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
        }
        if (channels & 0xF8) { // Channel_3..Channel_7 (PD2, PD3, PD5, PD6, PD4)
            GPIO_InitStructure.GPIO_Pin = 0;
            if (channels & 0x08) // Channel_3 (PD2)
                GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_2;
            if (channels & 0x10) // Channel_4 (PD3)
                GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_3;
            if (channels & 0x20) // Channel_5 (PD5)
                GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_5;
            if (channels & 0x40) // Channel_6 (PD6)
                GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_6;
            if (channels & 0x80) // Channel_7 (PD4)
                GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_4;
            GPIO_Init(GPIOD, &GPIO_InitStructure);
        }
    }

    {
        ADC_InitTypeDef ADC_InitStructure = { 0 };

        ADC_DeInit(ADC1);
        ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
        ADC_InitStructure.ADC_ScanConvMode = ENABLE;
        ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
        ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
        ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
        ADC_InitStructure.ADC_NbrOfChannel = 1;
        ADC_Init(ADC1, &ADC_InitStructure);
    }

    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) {}
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) {}

    {
        NVIC_InitTypeDef NVIC_InitStructure = { 0 };

        NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }

    _adcx_vref_mask = vref_mask;

    ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 1, ADC_SampleTime_241Cycles);

    if (channels) {
        uint8_t l = 0;

        for (uint8_t i = 0; i < 8; ++i) {
            if (channels & (1 << i))
                ++l;
            if (l > ADCX_CHANNELS - 1)
                break;
        }
        ADC_InjectedSequencerLengthConfig(ADC1, l);

        l = 1;
        for (uint8_t i = 0; i < 8; ++i) {
            if (channels & (1 << i)) {
                ADC_InjectedChannelConfig(ADC1, i, l++, ADC_SampleTime_241Cycles);
                if (l > ADCX_CHANNELS)
                    break;
            }
        }

        ADC_AutoInjectedConvCmd(ADC1, ENABLE);
    }
}

void ADCX_Done(void) {
    ADC_Cmd(ADC1, DISABLE);
    ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
    NVIC_DisableIRQ(ADC_IRQn);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
}

void ADCX_Start(void) {
    memset((void*)&_adcx, 0, sizeof(_adcx));

    ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
    ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void ADCX_Stop(void) {
    ADC_SoftwareStartConvCmd(ADC1, DISABLE);
    ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
}

void ADCX_Reset(uint8_t index) {
    uint8_t irq;

    irq = NVIC_GetStatusIRQ(ADC_IRQn);
    if (irq)
        NVIC_DisableIRQ(ADC_IRQn);
    memset((void*)&_adcx[index], 0, sizeof(adcx_t));
    if (irq)
        NVIC_EnableIRQ(ADC_IRQn);
}

inline __attribute__((always_inline)) uint16_t ADCX_Vref(void) {
    return _adcx_vref;
}

uint16_t ADCX_Read(uint8_t index) {
    uint16_t val;
    uint8_t irq;

    irq = NVIC_GetStatusIRQ(ADC_IRQn);
    if (irq)
        NVIC_DisableIRQ(ADC_IRQn);
    val = _adcx[index].len ? _adcx[index].sum / _adcx[index].len : 0;
    if (irq)
        NVIC_EnableIRQ(ADC_IRQn);
    return val;
}

void __attribute__((interrupt("WCH-Interrupt-fast"))) ADC1_IRQHandler(void) {
    if (ADC_GetITStatus(ADC1, ADC_IT_JEOC)) {
        uint8_t l;

        _adcx_vref = ADC_GetConversionValue(ADC1);
        l = (ADC1->ISQR >> 20) & 0x03;
        for (uint8_t i = 0; i <= l; ++i) {
            uint16_t v;

            v = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1 + i * 4);
            if (_adcx_vref_mask & (1 << i))
                v = v * 120 / _adcx_vref;
            if (_adcx[i].len < ADCX_AVGCOUNT)
                ++_adcx[i].len;
            else
                _adcx[i].sum -= bitarray_get((const uint8_t*)_adcx[i].samples, _adcx[i].pos);

            bitarray_set((uint8_t*)_adcx[i].samples, _adcx[i].pos, v);
            if (++_adcx[i].pos >= ADCX_AVGCOUNT)
                _adcx[i].pos = 0;
            _adcx[i].sum += v;
        }

        ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
    }
}
