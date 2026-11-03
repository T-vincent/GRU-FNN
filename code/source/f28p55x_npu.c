//###########################################################################
//
// FILE:    f28p55x_npu.c
//
// TITLE:   NPU initialization and Support Functions.
//
//###########################################################################
//
//
// 
// C2000Ware v26.01.00.00
//
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

//
// Includes
//
#include <stdbool.h>
#include <stdint.h>
#include "driverlib.h"

//
// Defines
//
#define NPU_O_CTL5      0x20U    // Control Register 5 Offset
#define NPU_O_IMASK     0x08U    // Interrupt Mask Register

#pragma FUNCTION_OPTIONS(NPU_setInstrMem, "--opt_level=3 --opt_for_speed=5")
#pragma FUNCTION_OPTIONS(NPU_setParamMem, "--opt_level=3 --opt_for_speed=5")

// Returns the address of the first ACC register of the NPU module.
inline uint32_t NPU_getAccReg(void)
{
    return (ACC_BASE);
}

// Returns address of first CTL register of NPU module.
inline uint32_t NPU_getCtlReg(void)
{
    return (IPSPECIFIC_BASE + NPU_O_CTL5);
}


// Immutable model parameters can remain resident between samples.
// This application exclusively owns the NPU; initialization invalidates caches.
static const uint32_t *imcParamPtr,*imcInstructionPtr;
static uint32_t imcParamOffset,imcParamCount,imcInstructionOffset,imcInstructionCount;
volatile uint32_t g_npuParameterCopies,g_npuParameterCacheHits,g_npuInstructionCopies,g_npuInstructionCacheHits;
void NPU_invalidateCache(void){imcParamPtr=0;imcInstructionPtr=0;}
// Set the instruction memory of the NPU at the specified offset.
void NPU_setInstrMem(uint32_t byteOffset, uint32_t num, const uint32_t *src)
{
    if(src==imcInstructionPtr && byteOffset==imcInstructionOffset && num==imcInstructionCount){g_npuInstructionCacheHits++;return;}
    imcInstructionPtr=src;imcInstructionOffset=byteOffset;imcInstructionCount=num;g_npuInstructionCopies++;
    volatile uint32_t *ins = (uint32_t *) (INSTRUCTION_BASE + byteOffset);

    int count_by_4 = num >> 2;
    int count_by_1 = num & 0x3;

    if (count_by_4 > 0)
    {
        while (count_by_4--)
        {
            * ins    = *src++;
            *(ins+2) = *src++;
            *(ins+4) = *src++;
            *(ins+6) = *src++;
            ins   += 8;       /* C28 16-bit addressable, access NPU memory accordingly */
        }
    }
    if (count_by_1 > 0)
    {
        while (count_by_1--)
        {
            *ins++ = *src++;
            ins++;             /* C28 16-bit addressable, access NPU memory accordingly */
        }
    }
}

// Set the parameter memory of the NPU at the specified offset.
void NPU_setParamMem(uint32_t byteOffset, uint32_t num, const uint32_t *src)
{
    if(src==imcParamPtr && byteOffset==imcParamOffset && num==imcParamCount){g_npuParameterCacheHits++;return;}
    imcParamPtr=src;imcParamOffset=byteOffset;imcParamCount=num;g_npuParameterCopies++;
    volatile uint32_t *params = (uint32_t *) (RFDATA_BASE + byteOffset);

    int count_by_4 = num >> 2;
    int count_by_1 = num & 0x3;

    if (count_by_4 > 0)
    {
        while (count_by_4--)
            {
            * params    = *src++;
            *(params+2) = *src++;
            *(params+4) = *src++;
            *(params+6) = *src++;
            params   += 8;           /* C28 16-bit addressable, access NPU memory accordingly */
            }
    }
    if (count_by_1 > 0)
    {
        while (count_by_1--)
        {
            *params++ = *src++;
            params++;           /* C28 16-bit addressable, access NPU memory accordingly */
        }
    }
}

// Gate Timer1 and sampling Timer0 may preempt NPU completion postprocessing.
/* Call the ordinary helper, never call an __interrupt function as a
   regular C function (C28x IRET and normal return are incompatible). */
extern void tvmgen_default_IRQHandlerHelper(void);
__interrupt void imcNpuISR(void)
{
    uint16_t savedIER=IER;
    IER=(savedIER & (INTERRUPT_CPU_INT1|INTERRUPT_CPU_INT13));
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
    __asm(" NOP");
    EINT;
    tvmgen_default_IRQHandlerHelper();
    DINT;
    IER=savedIER;
}
// Sets the interrupt handler for the NPU.
void NPU_setInterruptHandler(void (*handler)(void))
{
    Interrupt_disableGlobal();
    (void)handler;
    Interrupt_disable(INT_NPU);
    Interrupt_register(INT_NPU, &imcNpuISR);
    Interrupt_enable(INT_NPU);
    Interrupt_enableGlobal();
}

// Enables the interrupt for the NPU.
void NPU_enableInterrupt(void)
{
    HWREG(IPSTANDARD_BASE + NPU_O_IMASK) |= 0x1;
}

// Clears the interrupt ACK for the NPU.
void NPU_clearInterruptACK(void)
{
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
