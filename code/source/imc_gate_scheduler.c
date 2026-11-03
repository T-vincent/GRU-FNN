/* 门极调度：将调制量转换为同一50 us周期内的绝对事件时间。
 * 分离SVPWM与FNN函数是此前修复时序回归的关键实现改动；不要随意合并或内联。
 * 两路径共用约束：最小有效脉宽6 us，双/单矢量预算43/39 us，全关断换流间隔1 us。
 * 六位掩码0..2对应A/B/C上支路，3..5对应A/B/C下支路。
 */
#include "imc_gate_scheduler.h"
#include "imc_user_config.h"
#include <math.h>
#include "imc_cla_fnn.h"
#ifdef __TI_COMPILER_VERSION__
#pragma FUNC_ALWAYS_INLINE(push)
#pragma FUNC_ALWAYS_INLINE(IMC_GateSchedule_makeAllOff)
#pragma CODE_SECTION(IMC_GateSchedule_makeAllOff, ".TI.ramfunc")
#pragma CODE_SECTION(IMC_GateSchedule_build, ".TI.ramfunc")
#pragma CODE_SECTION(IMC_GateSchedule_svpwm, ".TI.ramfunc")
#pragma CODE_SECTION(IMC_GateSchedule_fnn, ".TI.ramfunc")
#pragma FUNC_CANNOT_INLINE(IMC_GateSchedule_svpwm)
#pragma FUNC_CANNOT_INLINE(IMC_GateSchedule_fnn)
#endif
/* 调制模式：0=原CPU SVPWM；1=CLA稀疏FNN，默认1。 */
volatile unsigned int g_modulatorMode=1;
/* Non-overlapping reverse-blocking switches. All-off commutation uses N->P
 * DC_freewheel. CPU/GPIO latency imposes a visible additional duty limit. */
static void push(volatile IMC_GateSchedule *s,uint32_t t,uint16_t mask)
{s->event[s->count].time_counts=t;s->event[s->count].gate_mask=mask;s->count++;}
void IMC_GateSchedule_makeAllOff(volatile IMC_GateSchedule *s)
{s->count=0;s->end_mask=0;s->sector=0;s->d0=1;s->d1=s->d2=0;s->duty_limited=0;push(s,0,0);}
/* CLA已在CPU前馈之前启动。这里核对同帧响应，取幅值平方、组号及两个驻留量。
 * 组号0..5对应六种合法矢量对；不是任意同时开启两条支路。
 */
uint16_t IMC_GateSchedule_fnn(const float vin[3],float m,uint16_t previous,volatile IMC_GateSchedule *s)
{
 float v[3],av[3],mean,pk,scale;uint16_t positive,main=0,i,a,b,mask1,mask2,n1,n2,n,sum,budget;uint32_t start;
 (void)previous;IMC_GateSchedule_makeAllOff(s);
 if(!(m>=0&&m<=.90f))return 0;
 {
unsigned int g;
if(!IMC_CLA_FNN_finish())return 0;
pk=sqrtf(g_claOutput[3]);if(!(pk>=50&&pk<=700))return 0;
scale=50.0f*m/pk;
g=(unsigned int)g_claOutput[2];
main=g/2;a=main==0?1:0;b=main==2?1:2;
n1=(unsigned int)(scale*g_claOutput[0]+.5f);n2=(unsigned int)(scale*g_claOutput[1]+.5f);
positive=(g%2)==0;
}
 if(n1&&n1<6){n1=0;s->duty_limited=1;}if(n2&&n2<6){n2=0;s->duty_limited=1;}budget=(n1&&n2)?IMC_GATE_DUAL_BUDGET_US:IMC_GATE_SINGLE_BUDGET_US;sum=n1+n2;
 if(sum>budget){/* Round one allocation, assign the remainder: preserve the total budget.
    Keep the existing TMU float division on this timing-critical path. */
    scale=(float)budget/sum;n1=(uint16_t)(n1*scale+.5f);n2=budget-n1;s->duty_limited=1;}
 if(n1&&n1<6){n1=0;s->duty_limited=1;}if(n2&&n2<6){n2=0;s->duty_limited=1;}
 if(positive){mask1=(1U<<main)|(1U<<(a+3));mask2=(1U<<main)|(1U<<(b+3));}
 else{mask1=(1U<<a)|(1U<<(main+3));mask2=(1U<<b)|(1U<<(main+3));}
 s->sector=main+1+(!positive?3:0);s->d1=n1/50.0f;s->d2=n2/50.0f;s->d0=1-s->d1-s->d2;
 n=(n1?1:0)+(n2?1:0);if(!n)return 1;start=(IMC_PERIOD-(n1+n2+n)*IMC_SLOT)/2;
 if(n1){push(s,start,mask1);start+=n1*IMC_SLOT;push(s,start,0);start+=IMC_SLOT;}
 if(n2){push(s,start,mask2);start+=n2*IMC_SLOT;push(s,start,0);}return 1;
}

/* 原SVPWM独立路径：去零序 -> 幅值归一化 -> 主相/极性选择 -> 两矢量驻留时间。
 * 不等待CLA，保持此前独立SVPWM固件的运算和脉宽分配规则。
 */
uint16_t IMC_GateSchedule_svpwm(const float vin[3],float m,uint16_t previous,volatile IMC_GateSchedule *s)
{
 float v[3],av[3],mean,pk,scale;uint16_t main=0,i,a,b,mask1,mask2,n1,n2,n,sum,budget;uint32_t start;
 (void)previous;IMC_GateSchedule_makeAllOff(s);
 if(!(m>=0&&m<=.90f))return 0;
 mean=(vin[0]+vin[1]+vin[2])/3;
#ifdef __TI_COMPILER_VERSION__
#pragma UNROLL(3)
#endif
 for(i=0;i<3;i++){if(!(vin[i]>=-700&&vin[i]<=700))return 0;v[i]=vin[i]-mean;av[i]=fabsf(v[i]);}
 pk=sqrtf((2.0f/3.0f)*(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]));if(!(pk>=50&&pk<=700))return 0;
 if(av[1]>av[0])main=1;
 if(av[2]>av[main])main=2;
 /* Preserve ascending order of the two remaining phases, including ties. */
 a=main==0?1:0;b=main==2?1:2;
 scale=50.0f*m/pk;n1=(uint16_t)(scale*av[a]+.5f);n2=(uint16_t)(scale*av[b]+.5f);
 if(n1&&n1<6){n1=0;s->duty_limited=1;}if(n2&&n2<6){n2=0;s->duty_limited=1;}budget=(n1&&n2)?IMC_GATE_DUAL_BUDGET_US:IMC_GATE_SINGLE_BUDGET_US;sum=n1+n2;
 if(sum>budget){/* Round one allocation, assign the remainder: preserve the total budget.
    Keep the existing TMU float division on this timing-critical path. */
    scale=(float)budget/sum;n1=(uint16_t)(n1*scale+.5f);n2=budget-n1;s->duty_limited=1;}
 if(n1&&n1<6){n1=0;s->duty_limited=1;}if(n2&&n2<6){n2=0;s->duty_limited=1;}
 if(v[main]>=0){mask1=(1U<<main)|(1U<<(a+3));mask2=(1U<<main)|(1U<<(b+3));}
 else{mask1=(1U<<a)|(1U<<(main+3));mask2=(1U<<b)|(1U<<(main+3));}
 s->sector=main+1+(v[main]<0?3:0);s->d1=n1/50.0f;s->d2=n2/50.0f;s->d0=1-s->d1-s->d2;
 n=(n1?1:0)+(n2?1:0);if(!n)return 1;start=(IMC_PERIOD-(n1+n2+n)*IMC_SLOT)/2;
 if(n1){push(s,start,mask1);start+=n1*IMC_SLOT;push(s,start,0);start+=IMC_SLOT;}
 if(n2){push(s,start,mask2);start+=n2*IMC_SLOT;push(s,start,0);}return 1;
}

/* 唯一入口：只做模式分派，让两条计算路径各自优化。 */
uint16_t IMC_GateSchedule_build(const float v[3],float m,uint16_t previous,volatile IMC_GateSchedule *s){return g_modulatorMode?IMC_GateSchedule_fnn(v,m,previous,s):IMC_GateSchedule_svpwm(v,m,previous,s);}
