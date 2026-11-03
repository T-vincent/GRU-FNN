#ifndef IMC_DMA_GATES_H
#define IMC_DMA_GATES_H
#include "imc_gate_scheduler.h"
void IMC_DMA_init(void);
void IMC_DMA_stop(void);
void IMC_DMA_stage(uint16_t bank,volatile IMC_GateSchedule *s);
uint16_t IMC_DMA_bind(uint16_t bank);
void IMC_DMA_startClock(void);
extern volatile uint32_t g_dmaBindCount,g_dmaBindMax,g_dmaStageMax,g_dmaErrors;
extern volatile uint16_t g_dmaActive,g_dmaEdgeCount,g_dmaStatus[4];
#endif
