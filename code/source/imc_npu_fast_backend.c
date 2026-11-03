#include <stdint.h>
#include <math.h>
#include "tvmgen_default.h"
#pragma CODE_SECTION(tvmgen_default_run, ".TI.ramfunc")
#pragma CODE_SECTION(tvmgen_default_IRQHandlerHelper, ".TI.ramfunc")
#pragma CODE_SECTION(tinie_sublayer_IRQHandler, ".TI.ramfunc")
volatile int32_t tvmgen_default_finished;
volatile int32_t tinie_curr_layer,tinie_curr_sublayer,tinie_num_sublayers;
void (*tinie_sublayer_postproc)(void);
void *tinie_sublayers;
extern void TINIE_setup(const void *layer);
extern void TINIE_start(void);
extern void TINIE_clearInterruptACK(void);
extern void NPU_setInterruptHandler(void (*handler)(void));
extern int32_t tvmgen_default_ti_npu_convolve1x1NHWC_0(void *,void *,void *);
#pragma DATA_SECTION(packedInput, ".npu_io")
#pragma DATA_SECTION(packedOutput, ".npu_io")
#pragma DATA_ALIGN(packedInput, 4)
#pragma DATA_ALIGN(packedOutput, 4)
static uint32_t packedInput[5],packedOutput[16];
static float *output;
static uint16_t interruptInstalled;
void tinie_sublayer_IRQHandler(void){TINIE_setup(tinie_sublayers);tinie_curr_sublayer=1;TINIE_start();}
void tvmgen_default_IRQHandlerHelper(void){
    TINIE_clearInterruptACK();
    tvmgen_default_finished=1;
}
const uint32_t *IMC_NPU_output_words(void){return packedOutput;}
int32_t tvmgen_default_run(struct tvmgen_default_inputs *in,struct tvmgen_default_outputs *out){
    uint16_t i,j;uint32_t word;int16_t q;float x;
    const float *src=(const float *)in->input_state;
    if(!interruptInstalled){NPU_setInterruptHandler(0);interruptInstalled=1;}
    output=(float *)out->output;tvmgen_default_finished=0;tinie_curr_layer=0;
    #pragma UNROLL(5)
    for(i=0;i<5;i++){
        word=0;
        #pragma UNROLL(4)
        for(j=0;j<4;j++){
            /* Exact floor(x+.5), using hardware float-to-int conversion.
               Clamping before conversion keeps every cast representable. */
            x=src[4*i+j]+.5f;if(x>127)x=127;if(x< -128)x=-128;
            q=(int16_t)x;if((float)q>x)q--;
            word|=((uint32_t)((uint16_t)q&255U))<<(8*j);
        }
        packedInput[i]=word;
    }
    return tvmgen_default_ti_npu_convolve1x1NHWC_0((void *)packedInput,(void *)packedOutput,0);
}

