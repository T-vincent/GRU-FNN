/* F28P550 / LAUNCHXL-F28P55X, 20kHz current-source rectifier, HIL ONLY.
   AO2/3/4 -> ADCINA0/4/5; AO5 -> ADCINA9. See imc_user_config.h.
   g_pwmEnable is the independent PWM enable; g_estimatorMode selects 0/1/2.
   The selected estimator warms for 50ms after startup.
   GRU matrix multiplication uses hardware NPU; the selected estimator runs at 20kHz.
   DC freewheel diode in the supplied HIL model is REQUIRED for all-off faults. */
/* 控制器入口：F28P550，HIL实板验证版。
 * 接线：AO2/3/4 -> ADCINA0/4/5；AO5 -> ADCINA9。
 * 门极：G1..G6 -> GPIO0/1/2/3/10/11 -> HIL DI1..DI6。
 * 电压估计选择：g_estimatorMode=0 原始电压，1 DSOGI，2 GRU16。
 * 调制选择：g_modulatorMode=0 CPU SVPWM，1 异步CLA稀疏FNN。
 * 已验证对比组合：估计/调制=(1,0)与(2,1)；两项选择相互独立。
 * g_pwmEnable=1 才允许输出，但仍须满足控制使能、预热完成、无故障。
 * Timer0每50 us采样；Timer1执行门极事件；Timer2为150 MHz递减计时器。
 * 一个计数=1/150 us；整帧保护7425计数=49.5 us，含ADC和中断负载。
 * 本次仅增加注释，未修改此前通过实板测试的算法、参数或可执行文件。
 */
#include <stdint.h>
#include <string.h>
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"
#include "imc_adc_nn_config.h"
#include "imc_gate_scheduler.h"
#include "imc_cla_fnn.h"
#include "imc_gru.h"
#include "imc_dma_gates.h"
#include "imc_estimator.h"
#include "imc_user_config.h"
#include "tvmgen_default.h"
#ifdef __TI_COMPILER_VERSION__
/* Keep the short gate ISR leaf-only: out-of-line calls otherwise force
   extra interrupt context saves on C28x. */
#pragma FUNC_ALWAYS_INLINE(now)
#pragma FUNC_ALWAYS_INLINE(gates)
#pragma FUNC_ALWAYS_INLINE(stopGates)
#pragma FUNC_ALWAYS_INLINE(fault)
#pragma FUNC_ALWAYS_INLINE(armEvent)
#pragma FUNC_ALWAYS_INLINE(applyGateEvent)
#pragma CODE_SECTION(process, ".TI.ramfunc")
#pragma CODE_SECTION(gates, ".TI.ramfunc")
#pragma CODE_SECTION(armEvent, ".TI.ramfunc")
#pragma CODE_SECTION(stopGates, ".TI.ramfunc")
#pragma CODE_SECTION(fault, ".TI.ramfunc")
#pragma CODE_SECTION(gateEventISR, ".TI.ramfunc")
#pragma CODE_SECTION(controlPeriodISR, ".TI.ramfunc")
#endif

#if defined(IMC_BENCHMARK_NO_PWM) || defined(IMC_HIL_PHYSICAL_TEST)
#define IMC_INITIAL_ESTIMATOR_MODE IMC_MODE_GRU
#else
#define IMC_INITIAL_ESTIMATOR_MODE IMC_MODE_RAW
#endif
#if defined(IMC_BENCHMARK_PWM_LOAD) || defined(IMC_HIL_PHYSICAL_TEST)
#define IMC_INITIAL_PWM_ENABLE 1
#ifdef IMC_BENCHMARK_PWM_LOAD
#undef IMC_INITIAL_PWM_ENABLE
#define IMC_INITIAL_PWM_ENABLE 1
#endif
#else
#define IMC_INITIAL_PWM_ENABLE 0
#endif
/* 调试入口：控制总使能、独立门极使能、期望估计模式、故障清除请求及锁存故障。 */
volatile uint16_t g_controlEnable=1,g_pwmEnable=IMC_INITIAL_PWM_ENABLE,g_estimatorMode=IMC_INITIAL_ESTIMATOR_MODE,g_clearFaultRequest=0,g_faultLatched=0;
volatile uint16_t g_activeEstimatorMode=IMC_INITIAL_ESTIMATOR_MODE,g_dutyLimited=0,g_feedforwardLimited=0;
volatile float g_estimateAmplitude=0,g_modulationRaw=0;
volatile uint16_t g_adcRawVa,g_adcRawVb,g_adcRawVc,g_adcRawSpare,g_activeGateMask;
volatile float g_adcZeroRawVa=2048,g_adcZeroRawVb=2048,g_adcZeroRawVc=2048,g_adcZeroRawIdc=2048;
volatile float g_gridVa,g_gridVb,g_gridVc,g_gruVa,g_gruVb,g_gruVc,g_idc;
/* 基准调制比0.5；实际调制比另经电压幅值前馈、上升步长与脉宽预算限制。 */
volatile float g_modulationCommand=0.50f,g_modulationApplied=0;
volatile float g_modulationRiseStep=IMC_MODULATION_STARTUP_STEP;
volatile uint32_t g_controlTickCount,g_adcSampleCount,g_gruRunCount,g_gruLastCounts,g_gruMaxCounts;
volatile uint32_t g_computeLastCounts,g_computeMaximumCounts,g_controlOverrunCount,g_scheduleMissCount,g_gateEventLateCount,g_adcFaultCount;
volatile uint32_t g_eventMaxLateCounts;
volatile uint32_t g_gatePairCount;
/* Frozen on a gate timing fault: 1=arm deadline, 2=ISR late, 3=bad index. */
volatile uint16_t g_gateFaultReason,g_gateFaultIndex,g_gateFaultMask;
volatile uint32_t g_gateFaultTargetCounts,g_gateFaultElapsedCounts;
volatile uint16_t g_gruWarm,g_scheduleSector,g_scheduleEventCount;
volatile uint32_t g_modulatorLastCounts,g_modulatorMaxCounts;
static IMC_GRU gru;
/* 双缓冲：中断执行activeIndex，主循环只写另一份；下一周期边界再交换。 */
static volatile IMC_GateSchedule schedules[2];
static volatile uint16_t activeIndex,ready,pending,busy,eventIndex,wasRunning;
static volatile uint32_t epochCounts,sampleTick;
static float previousPred[3];
static uint16_t previousPredValid;
volatile uint16_t g_estimatorReady;
volatile uint32_t g_benchmarkCurrentAnomalies;
volatile uint32_t g_currentLowIgnored;
volatile uint32_t g_actualPeriodCounts=IMC_PERIOD;
volatile uint16_t g_analogProfile=IMC_ANALOG_PROFILE;
volatile float g_estimatedVa,g_estimatedVb,g_estimatedVc;
volatile uint32_t g_frameLastCounts,g_frameMaxCounts;
volatile uint32_t g_sampleEpoch,g_failureTotalCounts,g_failureTick,g_failureAtIRQCounts,g_failureGruCounts;
volatile uint16_t g_stage,g_failureStage,g_failurePending,g_failureBusy;
volatile uint32_t g_gateISRCounts,g_gateISRMax,g_gateISRCalls,g_frameGateISRCounts,g_failureGateISRCounts;

volatile uint32_t g_npuColdLoadCounts;
volatile uint32_t g_dsogiRunCount,g_dsogiLastCounts,g_dsogiMaxCounts;
static IMC_DSOGI dsogi;
static uint16_t modeSamples;
/* 复位GRU隐藏状态、DSOGI状态和预测历史；预热需重新累计1000点（50 ms）。 */
static void resetEstimator(void)
{ IMC_GRU_reset(&gru); IMC_DSOGI_reset(&dsogi);g_gruWarm=0;g_estimatorReady=0;modeSamples=0;previousPredValid=0;memset(previousPred,0,sizeof(previousPred)); }

extern uint16_t SigRLoadStart,SigRLoadSize,SigRRunStart,SigZLoadStart,SigZLoadSize,SigZRunStart;
extern uint16_t GruConstLoadStart,GruConstLoadSize,GruConstRunStart;
extern uint16_t NpuConstLoadStart,NpuConstLoadSize,NpuConstRunStart;
static uint32_t now(void){return CPUTimer_getTimerCount(CPUTIMER2_BASE);}
/* 六位逻辑门极掩码映射到物理GPIO；先关旧支路再开新支路，避免写入瞬间重叠。
 * benchmark编译分支强制mask=0，但仍保留门极定时ISR用于测负载。
 */
static void gates(uint16_t mask)
{
    uint32_t desired;
#ifdef IMC_BENCHMARK_NO_PWM
    mask=0; /* Bench-only build physically suppresses every gate write. */
#endif
    mask&=63U; desired=(mask&15U)|((uint32_t)(mask&48U)<<6);
    /* Clear outgoing branches before setting incoming ones. */
    GPIO_clearPortPins(GPIO_PORT_A,0xC0FUL & ~desired);
    if(desired)GPIO_setPortPins(GPIO_PORT_A,desired);
    g_activeGateMask=mask;
}
static void stopGates(void)
{
#ifndef IMC_USE_CPU_GATES
 IMC_DMA_stop();
#endif
 CPUTimer_stopTimer(CPUTIMER1_BASE);gates(0);wasRunning=0;
}
static void fault(uint16_t code)
{if(!g_faultLatched)g_faultLatched=code;stopGates();}
/* 按当前周期绝对时间预约下一门极事件；来不及预约即故障，不能靠平移事件掩盖延迟。 */
static void armEvent(void)
{
    uint32_t elapsed,target,delta;
    uint16_t idx=eventIndex;
    volatile IMC_GateSchedule *schedule=&schedules[activeIndex];
    /* Both callers have already stopped Timer1 and cleared its flag.
       Interrupt nesting is disabled here, so the active schedule is stable. */
    if(idx>=schedule->count)return;
    target=schedule->event[idx].time_counts;
    elapsed=epochCounts-now();
    /* Do not shift future boundaries to hide ISR latency. */
    if(target<=elapsed+80UL+60UL){
        g_gateFaultReason=1;g_gateFaultIndex=eventIndex;g_gateFaultMask=g_activeGateMask;
        g_gateFaultTargetCounts=target;g_gateFaultElapsedCounts=elapsed;
        g_gateEventLateCount++;fault(4);return;
    }
    /* Compensate timer programming and ISR entry latency. The ISR still
       waits until target before switching, so boundaries cannot be advanced. */
    delta=target-elapsed-60UL;
    CPUTimer_setPeriod(CPUTIMER1_BASE,delta-1UL);
    CPUTimer_reloadTimerCounter(CPUTIMER1_BASE);
    /* startTimer reloads again; resumeTimer only clears STOP. */
    CPUTimer_resumeTimer(CPUTIMER1_BASE);
}
/* Execute the all-off/new-vector pair without a second timer interrupt. Timer2
   bounds the dead interval from AFTER the GPIO clear, never below IMC_OVERLAP.
   No interrupt nesting is enabled; the active schedule remains immutable. */
static void applyGateEvent(volatile IMC_GateSchedule *schedule,
                           uint16_t idx,uint32_t target,uint16_t mask)
{
    uint16_t nextMask;
    uint32_t nextTarget,applied,elapsed,late,observed;
    gates(mask);applied=now();eventIndex=++idx;
    if(idx<schedule->count){
        nextTarget=schedule->event[idx].time_counts;
        nextMask=schedule->event[idx].gate_mask;
        if(nextTarget-target==IMC_OVERLAP && mask==0 && nextMask!=0){
            /* Reuse the final wait timestamp, rather than performing a
               second timer read after the minimum overlap has elapsed. */
            do{observed=now();}while((uint32_t)(applied-observed)<IMC_OVERLAP);
            elapsed=epochCounts-observed;
            late=elapsed>nextTarget?elapsed-nextTarget:0;
            if(late>g_eventMaxLateCounts)g_eventMaxLateCounts=late;
            if(late>225UL){
                g_gateFaultReason=2;g_gateFaultIndex=idx;g_gateFaultMask=mask;
                g_gateFaultTargetCounts=nextTarget;g_gateFaultElapsedCounts=elapsed;
                g_gateEventLateCount++;fault(4);return;
            }
            gates(nextMask);eventIndex=idx+1U;g_gatePairCount++;
        }
    }
    armEvent();
}
/* 高频短中断：检查互锁，等待目标时刻、切换门极、预约下一事件并累计执行耗时。
 * 不允许嵌套；换流的最小全关断间隔仍由Timer2测量，不能随意删除等待。
 */
__interrupt void gateEventISR(void)
{
    uint32_t entered=now(),elapsed,target,late;
    uint16_t idx,mask;
    volatile IMC_GateSchedule *schedule;
    CPUTimer_stopTimer(CPUTIMER1_BASE);CPUTimer_clearOverflowFlag(CPUTIMER1_BASE);
    if(g_faultLatched||g_pwmEnable!=1||!g_controlEnable||g_estimatorMode!=g_activeEstimatorMode){stopGates();return;}
    idx=eventIndex;schedule=&schedules[activeIndex];
    if(idx>=schedule->count){g_gateFaultReason=3;g_gateFaultIndex=idx;fault(4);return;}
    target=schedule->event[idx].time_counts;mask=schedule->event[idx].gate_mask;
    elapsed=epochCounts-now();
    while(elapsed<target)elapsed=epochCounts-now();
    late=elapsed>target?elapsed-target:0;
    if(late>g_eventMaxLateCounts)g_eventMaxLateCounts=late;
    if(late>225UL){
        g_gateFaultReason=2;g_gateFaultIndex=eventIndex;g_gateFaultMask=g_activeGateMask;
        g_gateFaultTargetCounts=target;g_gateFaultElapsedCounts=elapsed;
        g_gateEventLateCount++;fault(4);return;
    }
    applyGateEvent(schedule,idx,target,mask);
    elapsed=entered-now();g_gateISRCounts+=elapsed;g_gateISRCalls++;if(elapsed>g_gateISRMax)g_gateISRMax=elapsed;
}
/* 50 us周期边界：检查上一帧是否未完成，触发四路ADC，并发布上一帧已完成的门极表。 */
__interrupt void controlPeriodISR(void)
{
    uint16_t run;
    CPUTimer_clearOverflowFlag(CPUTIMER0_BASE);CPUTimer_stopTimer(CPUTIMER1_BASE);
    CPUTimer_clearOverflowFlag(CPUTIMER1_BASE);
    epochCounts=now();g_controlTickCount++;
    if(g_estimatorMode!=g_activeEstimatorMode){stopGates();ready=0;g_estimatorReady=0;g_pwmEnable=0;}
    if(!g_controlEnable){g_modulationApplied=0;}
    run=(g_pwmEnable==1&&g_controlEnable&&g_estimatorReady&&!g_faultLatched);
    if(pending||busy){
        if(!g_failureTick){g_failureTick=g_controlTickCount;g_failureStage=g_stage;g_failureBusy=busy;g_failurePending=pending;g_failureAtIRQCounts=g_sampleEpoch-now();g_failureGateISRCounts=g_gateISRCounts-g_frameGateISRCounts;}
        g_controlOverrunCount++;fault(1);run=0;
    }
    if(g_controlEnable&&!g_faultLatched){
        if(ADC_getInterruptOverflowStatus(IMC_ADC_BASE,IMC_ADC_INT_NUMBER)){
            ADC_clearInterruptOverflowStatus(IMC_ADC_BASE,IMC_ADC_INT_NUMBER);fault(3);
        }else{
            ADC_clearInterruptStatus(IMC_ADC_BASE,IMC_ADC_INT_NUMBER);
            g_stage=1;g_sampleEpoch=epochCounts;g_frameGateISRCounts=g_gateISRCounts;ADC_forceMultipleSOC(IMC_ADC_BASE,IMC_ADC_FORCE_MASK);sampleTick=g_controlTickCount;pending=1;
        }
    }
    if(run){
        if(!ready){g_scheduleMissCount++;fault(2);run=0;}
        else{
            activeIndex^=1U;ready=0;eventIndex=0;
            g_scheduleSector=schedules[activeIndex].sector;g_scheduleEventCount=schedules[activeIndex].count;
            wasRunning=1;
            gates(0);
#ifdef IMC_USE_CPU_GATES
            eventIndex=1;armEvent();
#else
            if(!IMC_DMA_bind(activeIndex))fault(4);
#endif
        }
    }else{ready=0;stopGates();}
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
/* 主循环计算路径：ADC -> 电压估计 -> 相位外推 -> 异步CLA启动 -> 幅值前馈
 * -> 调制比限幅 -> 门极表生成 -> 整帧耗时检查 -> 发布ready。
 * 计算期间允许门极中断，故最大整帧包含中断抢占，不能将各环节最大值简单相加。
 */
static void process(void)
{
    uint16_t raw[4],i,idx,ok,mode;uint32_t start,gs,elapsed,frameEpoch;float v[3],pred[3]={0,0,0},control[3],delta,m;
    if(!pending||!ADC_getInterruptStatus(IMC_ADC_BASE,IMC_ADC_INT_NUMBER))return;
    DINT;pending=0;busy=1;EINT;start=now();frameEpoch=g_sampleEpoch;mode=g_estimatorMode;g_stage=2;
    for(i=0;i<4;i++)raw[i]=ADC_readResult(IMC_ADC_RESULT_BASE,(ADC_SOCNumber)i);
    ADC_clearInterruptStatus(IMC_ADC_BASE,IMC_ADC_INT_NUMBER);
    g_adcRawVa=raw[0];g_adcRawVb=raw[1];g_adcRawVc=raw[2];g_adcRawSpare=raw[3];
    for(i=0;i<4;i++)if(raw[i]<100U||raw[i]>3995U){g_adcFaultCount++;DINT;fault(3);busy=0;EINT;return;}
    /* ADC码先减零点，再按3.3 V/4096及当前HIL增益还原物理电压、电流。 */
    v[0]=((float)raw[0]-g_adcZeroRawVa)*(3.3f/4096.0f/IMC_VOLTAGE_GAIN);
    v[1]=((float)raw[1]-g_adcZeroRawVb)*(3.3f/4096.0f/IMC_VOLTAGE_GAIN);
    v[2]=((float)raw[2]-g_adcZeroRawVc)*(3.3f/4096.0f/IMC_VOLTAGE_GAIN);
    g_idc=((float)raw[3]-g_adcZeroRawIdc)*(3.3f/4096.0f/IMC_CURRENT_GAIN);
    if(!(g_idc>IMC_CURRENT_MIN&&g_idc<IMC_CURRENT_MAX)){
#ifdef IMC_BENCHMARK_NO_PWM
        /* Record only in the build that hard-forces every gate write to zero. */
        g_benchmarkCurrentAnomalies++;
#elif defined(IMC_HIL_PHYSICAL_TEST)
        if(g_idc<=IMC_CURRENT_MIN)g_currentLowIgnored++;
        else{DINT;fault(5);busy=0;EINT;return;}
#else
        DINT;fault(5);busy=0;EINT;return;
#endif
    }
    g_gridVa=v[0];g_gridVb=v[1];g_gridVc=v[2];g_adcSampleCount++;
    if(mode>IMC_MODE_GRU){DINT;fault(7);busy=0;EINT;return;}
    /* 运行中切换估计器先撤销门极使能并复位，避免旧状态与新算法混用。 */
    if(mode!=g_activeEstimatorMode){
        DINT;stopGates();ready=0;g_pwmEnable=0;EINT;
        resetEstimator();g_activeEstimatorMode=mode;g_modulationApplied=0;
    }
    if(g_activeEstimatorMode==IMC_MODE_RAW){for(i=0;i<3;i++)pred[i]=v[i];ok=1;}
    else if(g_activeEstimatorMode==IMC_MODE_DSOGI){
        gs=now();ok=IMC_DSOGI_step(&dsogi,v,pred);g_dsogiLastCounts=gs-now();
        if(g_dsogiLastCounts>g_dsogiMaxCounts)g_dsogiMaxCounts=g_dsogiLastCounts;g_dsogiRunCount++;
    }else{
        g_stage=3;gs=now();ok=IMC_GRU_step(&gru,v,pred);g_gruLastCounts=gs-now();
        if(g_gruLastCounts>g_gruMaxCounts)g_gruMaxCounts=g_gruLastCounts;g_gruRunCount++;g_stage=4;
        g_gruVa=pred[0];g_gruVb=pred[1];g_gruVc=pred[2];g_gruWarm=(gru.samples>=1000UL);
    }
    if(modeSamples<IMC_WARMUP_SAMPLES)modeSamples++;g_estimatorReady=(modeSamples>=IMC_WARMUP_SAMPLES);
    g_estimatedVa=pred[0];g_estimatedVb=pred[1];g_estimatedVc=pred[2];
    /* GRU output remains y(t). Linear extrapolation is ONLY for the schedule
       centered 1.5Ts later (one-period pipeline + half-period center). */
    for(i=0;i<3;i++){
        delta=previousPredValid?pred[i]-previousPred[i]:0;if(delta>10)delta=10;if(delta< -10)delta=-10;
        control[i]=pred[i]+IMC_PREDICTION_LEAD*delta;previousPred[i]=pred[i];
    }
    previousPredValid=1;
    if(!(g_modulationCommand>=0&&g_modulationCommand<=.90f))ok=0;
    g_stage=5;
    /* 提前启动CLA：使用本帧外推后的三相电压；CPU随后并行计算前馈。
     * CLA返回不含调制比的驻留量，CPU最后按本帧实际调制比缩放。
     */
    {extern volatile unsigned int g_modulatorMode;if(ok&&g_estimatorReady&&g_modulatorMode)ok=IMC_CLA_FNN_begin(control);}
    {float amp,mraw;m=IMC_amplitude_feedforward(pred,g_modulationCommand,&amp,&mraw);g_estimateAmplitude=amp;g_modulationRaw=mraw;g_feedforwardLimited=(mraw>.90f);if(g_estimatorReady&&amp<50)ok=0;}
    if(g_pwmEnable!=1||!g_controlEnable||!g_estimatorReady){m=0;g_modulationRiseStep=IMC_MODULATION_STARTUP_STEP;}
    /* 上升受步长限制；下降立即跟随。启动步长0.0005，运行步长5.0。
     * 运行步长大于允许调制比范围，因此通常不会额外限制运行期间的上升。
     */
    if(g_modulationApplied<m){
        g_modulationApplied+=g_modulationRiseStep;
        if(g_modulationApplied>=m){g_modulationApplied=m;g_modulationRiseStep=IMC_MODULATION_FF_STEP;}
    }else{
        g_modulationApplied=m;
        if(m>0)g_modulationRiseStep=IMC_MODULATION_FF_STEP;
    }
    g_stage=6;
    idx=activeIndex^1U;
    if(ok&&g_estimatorReady){gs=now();ok=IMC_GateSchedule_build(control,g_modulationApplied,wasRunning?schedules[activeIndex].end_mask:0,&schedules[idx]);g_modulatorLastCounts=gs-now();if(g_modulatorLastCounts>g_modulatorMaxCounts)g_modulatorMaxCounts=g_modulatorLastCounts;}
#ifndef IMC_USE_CPU_GATES
    if(ok&&g_estimatorReady)IMC_DMA_stage(idx,&schedules[idx]);
#endif
    g_dutyLimited=schedules[idx].duty_limited;
    if(!ok){DINT;fault(6);busy=0;EINT;return;}
    g_stage=7;
    elapsed=start-now();g_computeLastCounts=elapsed;if(elapsed>g_computeMaximumCounts)g_computeMaximumCounts=elapsed;
    DINT;
    g_frameLastCounts=frameEpoch-now();if(g_failureTick&&!g_failureTotalCounts){g_failureTotalCounts=g_frameLastCounts;g_failureGruCounts=g_gruLastCounts;}if(g_frameLastCounts>g_frameMaxCounts)g_frameMaxCounts=g_frameLastCounts;
    if(mode!=g_estimatorMode||!g_controlEnable){ready=0;g_estimatorReady=0;stopGates();busy=0;EINT;return;}
    if(sampleTick!=g_controlTickCount||g_frameLastCounts>IMC_FRAME_DEADLINE_COUNTS){g_controlOverrunCount++;fault(1);}
    else if(g_estimatorReady&&!g_faultLatched)ready=1;
    busy=0;g_stage=0;EINT;
}
/* 启动顺序：设备/CLA/外设 -> ADC与RAM常量 -> NPU冷加载 -> 周期中断。
 * NPU冷加载放在20 kHz控制开始之前；主循环处理计算、停止和受控故障复位。
 */
void main(void)
{
    uint16_t i;
    Device_init();IMC_CLA_FNN_init();Device_initGPIO();Interrupt_initModule();Interrupt_initVectorTable();Board_init();C2000Ware_libraries_init();
#ifdef _FLASH
    memcpy(&SigRRunStart,&SigRLoadStart,(size_t)&SigRLoadSize);
    memcpy(&SigZRunStart,&SigZLoadStart,(size_t)&SigZLoadSize);
    memcpy(&GruConstRunStart,&GruConstLoadStart,(size_t)&GruConstLoadSize);
    memcpy(&NpuConstRunStart,&NpuConstLoadStart,(size_t)&NpuConstLoadSize);
#endif
    /* Runtime ADC override keeps SysConfig and physical channels untouched.
       30 SYSCLK acquisition clocks=200ns; clock divider=2 =>75MHz ADCCLK. */
    ADC_setPrescaler(IMC_ADC_BASE,ADC_CLK_DIV_2_0);
    ADC_setSOCPriority(IMC_ADC_BASE,ADC_PRI_ALL_HIPRI);
    ADC_setupSOC(IMC_ADC_BASE,ADC_SOC_NUMBER0,ADC_TRIGGER_SW_ONLY,ADC_CH_ADCIN0,30);
    ADC_setupSOC(IMC_ADC_BASE,ADC_SOC_NUMBER1,ADC_TRIGGER_SW_ONLY,ADC_CH_ADCIN4,30);
    ADC_setupSOC(IMC_ADC_BASE,ADC_SOC_NUMBER2,ADC_TRIGGER_SW_ONLY,ADC_CH_ADCIN5,30);
    ADC_setupSOC(IMC_ADC_BASE,ADC_SOC_NUMBER3,ADC_TRIGGER_SW_ONLY,ADC_CH_ADCIN9,IMC_CURRENT_ACQUISITION_COUNTS);
    GPIO_clearPortPins(GPIO_PORT_A,0x3CCFUL);GPIO_clearPortPins(GPIO_PORT_B,0x28UL);
    IMC_GateSchedule_makeAllOff(&schedules[0]);IMC_GateSchedule_makeAllOff(&schedules[1]);
    CPUTimer_stopTimer(CPUTIMER2_BASE);CPUTimer_setPreScaler(CPUTIMER2_BASE,0);CPUTimer_setPeriod(CPUTIMER2_BASE,0xffffffffUL);
    CPUTimer_reloadTimerCounter(CPUTIMER2_BASE);CPUTimer_disableInterrupt(CPUTIMER2_BASE);CPUTimer_startTimer(CPUTIMER2_BASE);
    /* Load immutable NPU weights before the 20 kHz schedule starts.
       All GPIO gates are off; the cold load has a separate 300 us timeout. */
    IMC_GRU_init();IMC_GRU_reset(&gru);
    {float zero[3]={0,0,0},warm[3];
     if(!IMC_GRU_step(&gru,zero,warm)){g_faultLatched=8;g_controlEnable=0;}
     g_npuColdLoadCounts=g_npuLastCounts;g_npuMaximumCounts=0;g_npuLastCounts=0;
     if(!g_npuBusy)IMC_GRU_reset(&gru);
    }
    for(i=0;i<2;i++){
        uint32_t base=i?CPUTIMER1_BASE:CPUTIMER0_BASE;
        CPUTimer_stopTimer(base);CPUTimer_setPreScaler(base,0);CPUTimer_setPeriod(base,IMC_PERIOD-1);
        CPUTimer_reloadTimerCounter(base);CPUTimer_clearOverflowFlag(base);
        CPUTimer_setEmulationMode(base,CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);CPUTimer_enableInterrupt(base);
    }
#ifndef IMC_USE_CPU_GATES
    IMC_DMA_init();
#endif
    Interrupt_register(INT_TIMER0,&controlPeriodISR);
    Interrupt_enable(INT_TIMER0);
#ifdef IMC_USE_CPU_GATES
    Interrupt_register(INT_TIMER1,&gateEventISR);Interrupt_enable(INT_TIMER1);
#else
    Interrupt_disable(INT_TIMER1);
#endif
    EINT;ERTM;CPUTimer_startTimer(CPUTIMER0_BASE);
#ifndef IMC_USE_CPU_GATES
    IMC_DMA_startClock();
#endif
    for(;;){
        if(!g_controlEnable){DINT;stopGates();pending=busy=ready=0;resetEstimator();EINT;}
        if(g_pwmEnable!=1){DINT;stopGates();EINT;}
        process();
        /* 故障只在门极关闭且NPU可安全复位时清除；清故障不等于自动重新使能。 */
        if(g_clearFaultRequest&&g_pwmEnable==0&&(!g_npuBusy||tvmgen_default_finished)){
            DINT;stopGates();pending=busy=ready=0;g_faultLatched=0;g_clearFaultRequest=0;resetEstimator();
            IMC_GRU_reset(&gru);for(i=0;i<3;i++)previousPred[i]=0;
            ADC_clearInterruptStatus(IMC_ADC_BASE,IMC_ADC_INT_NUMBER);ADC_clearInterruptOverflowStatus(IMC_ADC_BASE,IMC_ADC_INT_NUMBER);
            EINT;
        }
    }
}
