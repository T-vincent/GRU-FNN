#ifndef IMC_CLA_FNN_H
#define IMC_CLA_FNN_H
void IMC_CLA_FNN_init(void);
extern volatile float g_claOutput[7];
unsigned int IMC_CLA_FNN_run(float a,float b,float c);
unsigned int IMC_CLA_FNN_begin(const float v[3]);
unsigned int IMC_CLA_FNN_finish(void);
#endif
