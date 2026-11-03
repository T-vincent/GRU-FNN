/* CPU侧CLA任务管理。消息RAM有写方向：CPU只写Input，CLA只写Output。
 * Input[0..2]为本帧外推电压，Input[3]为请求序号。
 * Output[0..1]为选中驻留量，Output[2]为合法组号，Output[3]为幅值平方，
 * Output[6]最后写入响应序号，作为本帧结果发布标志。
 * 不使用上一帧替代值；CPU必须先看到匹配序号，再读取结果。
 */
#include "driverlib.h"
#include "device.h"
#include <string.h>
#include "imc_cla_fnn.h"
#pragma DATA_SECTION(g_claInput,"CpuToCla1MsgRAM")
volatile float g_claInput[4];
#pragma DATA_SECTION(g_claOutput,"Cla1ToCpuMsgRAM")
volatile float g_claOutput[7];
volatile unsigned long g_claCalls,g_claTimeouts,g_claMaxCounts;
static float requestSeq;
static unsigned long requestStart;
extern unsigned int Cla1ProgLoadStart,Cla1ProgLoadSize,Cla1ProgRunStart;
extern __interrupt void ClaFnnTask(void);
/* LS7存放CLA程序，LS6用于CLA临时数据；GRU/NPU常量仍使用原LS8/LS9。
 * 先拷贝任务程序、初始化消息RAM，再授权CLA内存和Task1软件触发。
 */
void IMC_CLA_FNN_init(void){
 SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLA1);
 memcpy(&Cla1ProgRunStart,&Cla1ProgLoadStart,(size_t)&Cla1ProgLoadSize);
 MemCfg_initSections(MEMCFG_SECT_MSGCLA1TOCPU|MEMCFG_SECT_MSGCPUTOCLA1);
 while(!MemCfg_getInitStatus(MEMCFG_SECT_MSGCLA1TOCPU|MEMCFG_SECT_MSGCPUTOCLA1)){}
 MemCfg_setLSRAMControllerSel(MEMCFG_SECT_LS7,MEMCFG_LSRAMCONTROLLER_CPU_CLA1);
 MemCfg_setCLAMemType(MEMCFG_SECT_LS7,MEMCFG_CLA_MEM_PROGRAM);
 MemCfg_setLSRAMControllerSel(MEMCFG_SECT_LS6,MEMCFG_LSRAMCONTROLLER_CPU_CLA1);
 MemCfg_setCLAMemType(MEMCFG_SECT_LS6,MEMCFG_CLA_MEM_DATA);
 CLA_mapTaskVector(CLA1_BASE,CLA_MVECT_1,(uint16_t)&ClaFnnTask);
 CLA_setTriggerSource(CLA_TASK_1,CLA_TRIGGER_SOFTWARE);
 CLA_enableIACK(CLA1_BASE);CLA_enableTasks(CLA1_BASE,CLA_TASKFLAG_1);
}
#pragma CODE_SECTION(IMC_CLA_FNN_run,".TI.ramfunc")
/* 保留的同步调用辅助入口，当前主控制路径使用下面的begin/finish异步接口。 */
unsigned int IMC_CLA_FNN_run(float a,float b,float c){
 unsigned long start=CPUTimer_getTimerCount(CPUTIMER2_BASE),elapsed;unsigned int i;
 float seq=g_claInput[3]+1;if(seq>1000000)seq=1;
 g_claInput[0]=a;g_claInput[1]=b;g_claInput[2]=c;g_claInput[3]=seq;
 CLA_forceTasks(CLA1_BASE,CLA_TASKFLAG_1);
 while(g_claOutput[6]!=seq){if(start-CPUTimer_getTimerCount(CPUTIMER2_BASE)>750){g_claTimeouts++;return 0;}}
 elapsed=start-CPUTimer_getTimerCount(CPUTIMER2_BASE);if(elapsed>g_claMaxCounts)g_claMaxCounts=elapsed;g_claCalls++;return 1;
}
#pragma CODE_SECTION(IMC_CLA_FNN_begin,".TI.ramfunc")
/* 校验输入后提交任务，立即返回，让CPU前馈与CLA计算重叠。 */
unsigned int IMC_CLA_FNN_begin(const float v[3]){
 if(!(v[0]>=-700&&v[0]<=700&&v[1]>=-700&&v[1]<=700&&v[2]>=-700&&v[2]<=700))return 0;
 requestSeq+=1;if(requestSeq>1000000)requestSeq=1;
 requestStart=CPUTimer_getTimerCount(CPUTIMER2_BASE);
 g_claInput[0]=v[0];g_claInput[1]=v[1];g_claInput[2]=v[2];g_claInput[3]=requestSeq;
 CLA_forceTasks(CLA1_BASE,CLA_TASKFLAG_1);return 1;
}
#pragma CODE_SECTION(IMC_CLA_FNN_finish,".TI.ramfunc")
/* 消费同帧响应；未完成时有10 us等待观察上限，整帧仍受49.5 us保护。
 * g_claMaxCounts含异步重叠区间及中断时间，不能当作CLA纯推理耗时。
 */
unsigned int IMC_CLA_FNN_finish(void){
 unsigned long elapsed;
 while(g_claOutput[6]!=requestSeq){if(requestStart-CPUTimer_getTimerCount(CPUTIMER2_BASE)>1500){g_claTimeouts++;return 0;}}
 elapsed=requestStart-CPUTimer_getTimerCount(CPUTIMER2_BASE);if(elapsed>g_claMaxCounts)g_claMaxCounts=elapsed;g_claCalls++;return 1;
}
