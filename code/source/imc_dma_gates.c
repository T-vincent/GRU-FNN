#include "driverlib.h"
#include "imc_dma_gates.h"
#pragma CODE_SECTION(IMC_DMA_stage, ".TI.ramfunc")
#pragma CODE_SECTION(IMC_DMA_bind, ".TI.ramfunc")
#pragma CODE_SECTION(IMC_DMA_stop, ".TI.ramfunc")
/* Experimental path is disabled by IMC_USE_CPU_GATES. F28P55x TRM
   SPRUJ53C table 10-1 explicitly prohibits DMA access to GPIO_DATA.
   Internal ePWM7..10 compare events drive DMA1..4. GPIO mux is unchanged.
   Each burst writes SET then CLEAR, touching only GPIO0/1/2/3/10/11.
   Schedules contain only zero->vector or vector->zero transitions. */
static const uint32_t pwm[4]={EPWM7_BASE,EPWM8_BASE,EPWM9_BASE,EPWM10_BASE};
static const uint32_t dma[4]={DMA_CH1_BASE,DMA_CH2_BASE,DMA_CH3_BASE,DMA_CH4_BASE};
static const DMA_Trigger trig[4]={DMA_TRIGGER_EPWM7SOCA,DMA_TRIGGER_EPWM8SOCA,DMA_TRIGGER_EPWM9SOCA,DMA_TRIGGER_EPWM10SOCA};
#pragma DATA_SECTION(payload, ".dma_payload")
#pragma DATA_ALIGN(payload, 4)
static volatile uint32_t payload[2][4][2];
static volatile uint16_t edges[2],first[2];
static uint16_t shadowEmpty=1;
static uint16_t shadowEdges;
volatile uint32_t g_dmaBindCount,g_dmaBindMax,g_dmaStageMax,g_dmaErrors;
volatile uint32_t g_dmaTwoEdgeBinds,g_dmaFourEdgeBinds;
volatile uint16_t g_dmaActive,g_dmaEdgeCount,g_dmaStatus[4];
void IMC_DMA_init(void){
    uint16_t j;
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DMA);
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    DMA_initController();
    #pragma UNROLL(4)
    for(j=0;j<4;j++){
        EPWM_setClockPrescaler(pwm[j],EPWM_CLOCK_DIVIDER_2,EPWM_HSCLOCK_DIVIDER_1);
        EPWM_setTimeBaseCounter(pwm[j],0);
        EPWM_setPeriodLoadMode(pwm[j],EPWM_PERIOD_DIRECT_LOAD);
        EPWM_setTimeBasePeriod(pwm[j],(uint16_t)(IMC_PERIOD/2-1));
        EPWM_disablePhaseShiftLoad(pwm[j]);
        EPWM_setTimeBaseCounterMode(pwm[j],EPWM_COUNTER_MODE_UP);
        EPWM_setCounterCompareShadowLoadMode(pwm[j],EPWM_COUNTER_COMPARE_A,EPWM_COMP_LOAD_ON_CNTR_ZERO);
        EPWM_setCounterCompareValue(pwm[j],EPWM_COUNTER_COMPARE_A,(uint16_t)(IMC_PERIOD/2+1));
        EPWM_setADCTriggerSource(pwm[j],EPWM_SOC_A,EPWM_SOC_TBCTR_U_CMPA);
        EPWM_setADCTriggerEventPrescale(pwm[j],EPWM_SOC_A,1);
        EPWM_clearADCTriggerFlag(pwm[j],EPWM_SOC_A);
        EPWM_enableADCTrigger(pwm[j],EPWM_SOC_A);
        DMA_configAddresses(dma[j],(void *)(GPIODATA_BASE+GPIO_O_GPASET),(void *)&payload[0][j][0]);
        DMA_configBurst(dma[j],4,1,1);
        DMA_configTransfer(dma[j],1,0,0);
        DMA_configWrap(dma[j],65536,0,65536,0);
        DMA_configMode(dma[j],trig[j],DMA_CFG_ONESHOT_DISABLE|DMA_CFG_CONTINUOUS_ENABLE|DMA_CFG_SIZE_16BIT);
        DMA_disableInterrupt(dma[j]);DMA_disableTrigger(dma[j]);DMA_clearErrorFlag(dma[j]);
    }
}
void IMC_DMA_startClock(void){SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);}
void IMC_DMA_stop(void){
    uint16_t j;if(!g_dmaActive)return;
    EALLOW;
    #pragma UNROLL(4)
    for(j=0;j<4;j++){HWREGH(dma[j]+DMA_O_MODE)&=~DMA_MODE_PERINTE;HWREGH(dma[j]+DMA_O_CONTROL)=DMA_CONTROL_HALT|DMA_CONTROL_PERINTCLR;}
    EDIS;
    g_dmaActive=0;
}
void IMC_DMA_stage(uint16_t bank,volatile IMC_GateSchedule *s){
    uint16_t j,n,mask;uint32_t bits,start=CPUTimer_getTimerCount(CPUTIMER2_BASE),elapsed;
    n=s->count>1?s->count-1:0;if(n>4)n=4;edges[bank]=n;
    if(!n&&shadowEmpty){first[bank]=(uint16_t)(IMC_PERIOD/2+1);return;}
    shadowEmpty=(n==0);
    first[bank]=n?(uint16_t)((s->event[1].time_counts+1)/2):(uint16_t)(IMC_PERIOD/2+1);
    #pragma UNROLL(4)
    for(j=0;j<4;j++){
        if(j<n){
        mask=s->event[j+1].gate_mask;
#ifdef IMC_BENCHMARK_NO_PWM
        mask=0;
#endif
        bits=(mask&15U)|((uint32_t)(mask&48U)<<6);
        payload[bank][j][0]=bits;payload[bank][j][1]=0xC0FUL & ~bits;
        HWREGH(pwm[j]+EPWM_O_CMPA+1U)=(uint16_t)((s->event[j+1].time_counts+1)/2);
        }else if(j<shadowEdges){
            HWREGH(pwm[j]+EPWM_O_CMPA+1U)=(uint16_t)(IMC_PERIOD/2+1);
        }
    }
    shadowEdges=n;
    elapsed=start-CPUTimer_getTimerCount(CPUTIMER2_BASE);if(elapsed>g_dmaStageMax)g_dmaStageMax=elapsed;
}
uint16_t IMC_DMA_bind(uint16_t bank){
    uint16_t j,n=edges[bank];uint32_t start=CPUTimer_getTimerCount(CPUTIMER2_BASE),elapsed;
    if(!n){IMC_DMA_stop();g_dmaEdgeCount=0;g_dmaBindCount++;return 1;}
    /* Bank pointers must be installed well before the first >=6us edge. */
    if(n && (uint32_t)EPWM_getTimeBaseCounterValue(EPWM7_BASE)+150UL>=first[bank]){g_dmaErrors++;IMC_DMA_stop();return 0;}
    g_dmaActive=1;
    EALLOW;
    #pragma UNROLL(4)
    for(j=0;j<4;j++){
        g_dmaStatus[j]=HWREGH(dma[j]+DMA_O_CONTROL);
        if(g_dmaStatus[j]&DMA_CONTROL_OVRFLG){EDIS;g_dmaErrors++;IMC_DMA_stop();return 0;}
        if(j<n){
            HWREG(dma[j]+DMA_O_SRC_BEG_ADDR_SHADOW)=(uint32_t)&payload[bank][j][0];
            HWREG(dma[j]+DMA_O_SRC_ADDR_SHADOW)=(uint32_t)&payload[bank][j][0];
            HWREGH(dma[j]+DMA_O_MODE)|=DMA_MODE_PERINTE;HWREGH(dma[j]+DMA_O_CONTROL)=DMA_CONTROL_PERINTCLR|DMA_CONTROL_RUN;
        }
        else{HWREGH(dma[j]+DMA_O_MODE)&=~DMA_MODE_PERINTE;HWREGH(dma[j]+DMA_O_CONTROL)=DMA_CONTROL_PERINTCLR|DMA_CONTROL_HALT;}
    }
    EDIS;
    g_dmaActive=1;g_dmaEdgeCount=n;g_dmaBindCount++;
    if(n==2)g_dmaTwoEdgeBinds++;else if(n==4)g_dmaFourEdgeBinds++;
    elapsed=start-CPUTimer_getTimerCount(CPUTIMER2_BASE);if(elapsed>g_dmaBindMax)g_dmaBindMax=elapsed;
    return 1;
}
