#ifndef IMC_ESTIMATOR_H
#define IMC_ESTIMATOR_H
#include <stdint.h>
#include "imc_gru.h"
enum { IMC_MODE_RAW=0, IMC_MODE_DSOGI=1, IMC_MODE_GRU=2 };
typedef struct {float sogi[2][2],previous[2],theta,xi,amp,omega;uint16_t initialized;} IMC_DSOGI;
void IMC_DSOGI_reset(IMC_DSOGI *s);
uint16_t IMC_DSOGI_step(IMC_DSOGI *s,const float v[3],float y[3]);
float IMC_amplitude_feedforward(const float v[3],float base,float *amplitude,float *raw);
#endif
