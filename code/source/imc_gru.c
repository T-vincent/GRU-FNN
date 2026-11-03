/* 16单元量化GRU：NPU执行编译后的INT8矩阵模型；CPU完成输入归一化、
 * 查表激活、隐藏状态更新和三相输出投影。权重与量化尺度保持实板验证版本。
 * Timer2递减计数器用于测时；等待NPU期间保持门极中断可响应。
 */
#include "imc_gru.h"
#include "driverlib.h"
#include "tvmgen_default.h"
/* This implementation uses the compiled hardware NPU matrix model.
   CPU only handles activation, persistent state, and the three-output FC. */
volatile uint16_t g_npuBusy,g_npuError;
volatile uint32_t g_gruPreMax,g_gruPostMax;
volatile uint32_t g_npuLastCounts,g_npuMaximumCounts,g_npuTimeoutCount;
static uint16_t npuColdStart;
#ifdef IMC_CAPTURE_TRACE
volatile uint16_t g_traceCount;
float g_trace[64][6];
#endif
/* NPU输入布局：3个电压量、16个上一帧隐藏状态、1个固定占位量。 */
#define INPUT_WIDTH 20
#ifndef IMC_NPU_WARM_TIMEOUT_COUNTS
#define IMC_NPU_WARM_TIMEOUT_COUNTS 5250UL
#endif
extern const float gru_input_center[20],gru_input_inverse_scale[20],gru_gate_center[64],gru_gate_scale[64];
#pragma DATA_SECTION(npuInput, ".npu_io")
#pragma DATA_SECTION(npuOutput, ".npu_io")
static float npuInput[INPUT_WIDTH],npuOutput[4*IMC_GRU_HIDDEN];
#pragma CODE_SECTION(IMC_GRU_step, ".TI.ramfunc")
#pragma FUNC_ALWAYS_INLINE(fastTanh)
extern const float gru_fc_projected[32],gru_fb_projected[2];
extern const float gru_sig_r[16][256],gru_sig_z[16][256];
static float fastTanh(float x)
{float a=(x+4.0f)*256.0f,f;uint16_t i=(uint16_t)a;f=a-(float)i;return gru_tanh_lut[i]+f*(gru_tanh_lut[i+1]-gru_tanh_lut[i]);}

#ifdef __TI_COMPILER_VERSION__
extern const uint32_t *IMC_NPU_output_words(void);
#pragma FUNC_ALWAYS_INLINE(qbyte)
static int16_t qbyte(const uint32_t *w,uint16_t i){uint16_t v=(uint16_t)((w[i/4]>>(8*(i%4)))&255UL);return (int16_t)(v^128U)-128;}
#pragma FUNC_ALWAYS_INLINE(qindex)
static uint16_t qindex(const uint32_t *w,uint16_t i){return (uint16_t)((w[i/4]>>(8*(i%4)))&255UL)^128U;}
#endif
#pragma FUNC_ALWAYS_INLINE(dot8)
static float dot8(const float *restrict a,const float *restrict b){float sum=0;uint16_t j;
#ifdef __TI_COMPILER_VERSION__
#pragma MUST_ITERATE(8,8,8)
#pragma UNROLL(1)
#endif
for(j=0;j<8;j++)sum+=a[j]*b[j];return sum;}

void IMC_GRU_init(void){TI_NPU_init();g_npuBusy=0;g_npuError=0;npuColdStart=1;}
/* 仅在NPU完成后复位隐藏状态，禁止覆盖仍在计算中的输入/状态。 */
void IMC_GRU_reset(IMC_GRU *s)
{uint16_t i;
 if(g_npuBusy){if(!tvmgen_default_finished){g_npuError=1;return;}g_npuBusy=0;}
 for(i=0;i<IMC_GRU_HIDDEN;i++)s->h[i]=0;s->samples=0;g_npuError=0;
}
/* 每帧推理入口。第一次冷加载允许300 us且在周期调度开始前执行；
 * 正常周期的NPU等待上限35 us。最终实时性仍以主控制器49.5 us整帧判据为准。
 */
uint16_t IMC_GRU_step(IMC_GRU *s,const float volts[3],float output[3])
{
 uint16_t i,j;uint32_t start,elapsed,begin=CPUTimer_getTimerCount(CPUTIMER2_BASE),post;float r,z,n,mean;
 struct tvmgen_default_inputs input;struct tvmgen_default_outputs result;
#ifdef __TI_COMPILER_VERSION__
 const uint32_t *restrict packed;
#endif
 if(g_npuBusy||g_npuError)return 0;
#ifdef __TI_COMPILER_VERSION__
#pragma UNROLL(3)
#endif
 for(j=0;j<3;j++){if(!(volts[j]>=-700&&volts[j]<=700))return 0;npuInput[j]=(volts[j]/600-gru_input_center[j])*gru_input_inverse_scale[j];}
 /* Normalize directly into the input buffer: avoid copying the hidden state
    and traversing it again. Floating-point operation order is unchanged. */
#ifdef __TI_COMPILER_VERSION__
#pragma UNROLL(16)
#endif
 for(j=0;j<IMC_GRU_HIDDEN;j++)npuInput[3+j]=(s->h[j]-gru_input_center[3+j])*gru_input_inverse_scale[3+j];
 npuInput[19]=(0.0f-gru_input_center[19])*gru_input_inverse_scale[19];
 input.input_state=(void*)npuInput;result.output=(void*)npuOutput;
 tvmgen_default_finished=0;g_npuBusy=1;start=CPUTimer_getTimerCount(CPUTIMER2_BASE);
 elapsed=begin-start;if(elapsed>g_gruPreMax)g_gruPreMax=elapsed;
 if(tvmgen_default_run(&input,&result)){g_npuError=1;return 0;}
 /* Main-loop polling leaves gate interrupts enabled. No buffer/state is
    overwritten before the NPU completion flag, including on a timeout. */
 while(!tvmgen_default_finished){
  if(start-CPUTimer_getTimerCount(CPUTIMER2_BASE)>(npuColdStart?45000UL:IMC_NPU_WARM_TIMEOUT_COUNTS)){g_npuError=1;g_npuTimeoutCount++;return 0;}
 }
 elapsed=start-CPUTimer_getTimerCount(CPUTIMER2_BASE);g_npuLastCounts=elapsed;
 if(elapsed>g_npuMaximumCounts)g_npuMaximumCounts=elapsed;g_npuBusy=0;post=CPUTimer_getTimerCount(CPUTIMER2_BASE);


#ifdef __TI_COMPILER_VERSION__
 packed=IMC_NPU_output_words();
#endif
#ifdef __TI_COMPILER_VERSION__
#pragma UNROLL(16)
#endif
 for(i=0;i<IMC_GRU_HIDDEN;i++){
#ifdef __TI_COMPILER_VERSION__
  int16_t qr,qz;float ni,nr;
  qr=qindex(packed,i);qz=qindex(packed,16+i);ni=(float)qbyte(packed,32+i);nr=(float)qbyte(packed,48+i);
#else
  int16_t qr=(int16_t)npuOutput[i],qz=(int16_t)npuOutput[16+i];float ni=npuOutput[32+i],nr=npuOutput[48+i];
#endif
#ifdef __TI_COMPILER_VERSION__
  r=gru_sig_r[i][qr];z=gru_sig_z[i][qz];
#else
  r=gru_sig_r[i][qr+128];z=gru_sig_z[i][qz+128];
#endif
  n=fastTanh((ni*gru_gate_scale[32+i]+gru_gate_center[32+i])+r*(nr*gru_gate_scale[48+i]+gru_gate_center[48+i]));
  s->h[i]=n+z*(s->h[i]-n);
 }
 output[0]=gru_fb_projected[0]+dot8(gru_fc_projected,s->h)+dot8(gru_fc_projected+8,s->h+8);
 output[1]=gru_fb_projected[1]+dot8(gru_fc_projected+16,s->h)+dot8(gru_fc_projected+24,s->h+8);
 output[2]=-output[0]-output[1];
 for(i=0;i<3;i++)if(!(output[i]>=-700&&output[i]<=700))return 0;
#ifdef IMC_CAPTURE_TRACE
 if(!npuColdStart && g_traceCount<64){for(j=0;j<3;j++){g_trace[g_traceCount][j]=volts[j];g_trace[g_traceCount][3+j]=output[j];}g_traceCount++;}
#endif
 elapsed=post-CPUTimer_getTimerCount(CPUTIMER2_BASE);if(elapsed>g_gruPostMax)g_gruPostMax=elapsed;
 npuColdStart=0;if(s->samples<0xffffffffUL)s->samples++;return 1;
}
