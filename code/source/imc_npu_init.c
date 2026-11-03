#include "driverlib.h"
extern void NPU_enableInterrupt(void);
extern void NPU_invalidateCache(void);
void TI_NPU_init(void){
    /* Device_init() in the supplied ADC example does not enable NPU clock. */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_NPU);
    SysCtl_setNPUClockDivider(SYSCTL_NPUCLK_DIV_1);
    NPU_invalidateCache();NPU_enableInterrupt();
}
