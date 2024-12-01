#define ADC_MODE_ADCX   1
#define ADC_MODE_ADCJ   2

#define ADC_MODE    ADC_MODE_ADCX

#include "debug.h"
#if ADC_MODE == ADC_MODE_ADCX
#include "adcx.h"
#elif ADC_MODE == ADC_MODE_ADCJ
#include "adcj.h"
#endif

int main(void) {
    SystemCoreClockUpdate();

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    Delay_Init();
    USART_Printf_Init(115200);

#if ADC_MODE == ADC_MODE_ADCX
    ADCX_Init((1 << ADC_Channel_3) | (1 << ADC_Channel_4), (1 << 1));
    ADCX_Start();
#elif ADC_MODE == ADC_MODE_ADCJ
    ADCJ_Init(ADC_Channel_3);
#endif

    while (1) {
        Delay_Ms(10);
#if ADC_MODE == ADC_MODE_ADCX
        printf("\r[3] %04u\t[4] %04u", ADCX_Read(0), ADCX_Read(1));
#elif ADC_MODE == ADC_MODE_ADCJ
        printf("\r[3] %04u", ADCJ_Read());
#endif
    }
}
