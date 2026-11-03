/* 用户配置集中入口：HIL模拟量比例、保护阈值、调制比上升步长及门极预算。
 * IMC_ANALOG_PROFILE=1为本次测试的HIL比例：电压0.002 V/V、电流0.05 V/A，
 * AO偏置1.65 V由HIL设置，ADC默认零点2048码由主程序扣除。
 * 当前使用CPU Timer1门极调度；保留的DMA路径不作为已验证输出方式。
 */
#ifndef IMC_USER_CONFIG_H
#define IMC_USER_CONFIG_H
/* 0: supplied ADC example; 1: latest HIL NPU delivery AO scaling.
   HIL AO gain/offset MUST match this selection. Pins are identical. */
#ifndef IMC_ANALOG_PROFILE
#define IMC_ANALOG_PROFILE 1
#endif
#if IMC_ANALOG_PROFILE == 0
#define IMC_VOLTAGE_GAIN .0045f
#define IMC_CURRENT_GAIN 1.4f
#define IMC_CURRENT_MIN (-.08f)
#define IMC_CURRENT_MAX .35f
#elif IMC_ANALOG_PROFILE == 1
#define IMC_VOLTAGE_GAIN .002f
#define IMC_CURRENT_GAIN .05f
#define IMC_CURRENT_MIN (-1.0f)
#define IMC_CURRENT_MAX 30.0f
#else
#error Invalid_IMC_ANALOG_PROFILE
#endif
#define IMC_WARMUP_SAMPLES 1000U
/* Per 50 us sample: keep startup gentle, allow faster running feedforward. */
#define IMC_MODULATION_STARTUP_STEP 0.0005f
#define IMC_MODULATION_FF_STEP 5.0f
/* Physical validation candidate: dual vectors include a separate 1 us gap. */
#define IMC_GATE_DUAL_BUDGET_US 43U
#define IMC_GATE_SINGLE_BUDGET_US 39U
/* Current acquisition window; matches the validated timing configuration. */
#define IMC_CURRENT_ACQUISITION_COUNTS 30U
/* DMA-to-GPIO output has not passed physical validation; use CPU Timer1. */
#define IMC_USE_CPU_GATES 1
/* 150 MHz timer ticks; entire sample-to-schedule path, including ADC wait. */
#ifndef IMC_FRAME_DEADLINE_COUNTS
#define IMC_FRAME_DEADLINE_COUNTS 7425UL
#endif
/* Pipeline phase extrapolation inherited from the deployment controller. */
#define IMC_PREDICTION_LEAD 1.5f
#endif
