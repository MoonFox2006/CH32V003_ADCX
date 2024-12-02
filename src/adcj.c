#include <ch32v00x.h>
#include <ch32v00x_rcc.h>
#include <ch32v00x_gpio.h>
#include <ch32v00x_adc.h>
#include "adcj.h"

#if (ADCJ_MEASURES < 2) || (ADCJ_MEASURES > 4)
    #error "Wrong ADCJ measures!"
#endif

void ADCJ_Init(uint8_t channel) {
    {
        uint32_t periph;

        periph = RCC_APB2Periph_ADC1;
        if (channel <= 1) // Channel_0 | Channel_1 (PA2, PA1)
            periph |= RCC_APB2Periph_GPIOA;
        else if (channel == 2) // Channel_2 (PC4)
            periph |= RCC_APB2Periph_GPIOC;
        else // Channel_3..Channel_7 (PD2, PD3, PD5, PD6, PD4)
            periph |= RCC_APB2Periph_GPIOD;

        RCC_APB2PeriphClockCmd(periph, ENABLE);
    }

    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    {
        GPIO_InitTypeDef GPIO_InitStructure = { 0 };

        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
//        GPIO_InitStructure.GPIO_Pin = 0;
        if (channel <= 1) { // Channel_0 | Channel_1 (PA2, PA1)
            if (channel == 0) // Channel_0 (PA2)
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
            else // Channel_1 (PA1)
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
        } else if (channel == 2) { // Channel_2 (PC4)
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
        } else { // Channel_3..Channel_7 (PD2, PD3, PD5, PD6, PD4)
            if (channel == 3) // Channel_3 (PD2)
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
            else if (channel == 4) // Channel_4 (PD3)
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
            else if (channel == 5) // Channel_5 (PD5)
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
            else if (channel == 6) // Channel_6 (PD6)
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
            else // Channel_7 (PD4)
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
            GPIO_Init(GPIOD, &GPIO_InitStructure);
        }
    }

    {
        ADC_InitTypeDef ADC_InitStructure = { 0 };

        ADC_DeInit(ADC1);
        ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
        ADC_InitStructure.ADC_ScanConvMode = ENABLE;
        ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
        ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigInjecConv_None;
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

    ADC_InjectedSequencerLengthConfig(ADC1, ADCJ_MEASURES);
    for (uint8_t i = 1; i <= ADCJ_MEASURES; ++i) {
        ADC_InjectedChannelConfig(ADC1, channel, i, ADC_SampleTime_241Cycles);
    }
}

void ADCJ_Done(void) {
    ADC_Cmd(ADC1, DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
}

uint16_t ADCJ_Read(void) {
    uint16_t sum;

    ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);
    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC)) {}
    sum = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
    sum += ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2);
#if ADCJ_MEASURES > 2
    sum += ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_3);
#endif
#if ADCJ_MEASURES > 3
    sum += ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_4);
#endif
    return sum / ADCJ_MEASURES;
}
