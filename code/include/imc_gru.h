#ifndef IMC_GRU_H
#define IMC_GRU_H
#include <stdint.h>
#define IMC_GRU_HIDDEN 16
extern const float gru_fc[48],gru_fb[3],gru_tanh_lut[2049];
typedef struct {float h[IMC_GRU_HIDDEN];uint32_t samples;} IMC_GRU;
extern volatile uint16_t g_npuBusy,g_npuError;
extern volatile uint32_t g_npuLastCounts,g_npuMaximumCounts,g_npuTimeoutCount;
void IMC_GRU_init(void);
void IMC_GRU_reset(IMC_GRU *s);
uint16_t IMC_GRU_step(IMC_GRU *s,const float volts[3],float output[3]);
#endif
