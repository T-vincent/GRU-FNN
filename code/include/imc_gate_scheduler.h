#ifndef IMC_GATE_SCHEDULER_H_
#define IMC_GATE_SCHEDULER_H_
#include <stdint.h>
#define IMC_GATE_EVENT_MAX 16
#define IMC_GATE_MASK_ALL 0x003fU
#ifndef IMC_PERIOD
#define IMC_PERIOD 7500UL
#endif
#define IMC_SLOT 150UL
#define IMC_OVERLAP IMC_SLOT
#define IMC_MIN_DWELL 900UL
typedef struct {uint32_t time_counts; uint16_t gate_mask;} IMC_GateEvent;
typedef struct {uint16_t count,end_mask,sector,duty_limited; float d1,d2,d0; IMC_GateEvent event[IMC_GATE_EVENT_MAX];} IMC_GateSchedule;
void IMC_GateSchedule_makeAllOff(volatile IMC_GateSchedule *s);
uint16_t IMC_GateSchedule_build(const float v[3],float modulation,uint16_t previousMask,volatile IMC_GateSchedule *s);
#endif
