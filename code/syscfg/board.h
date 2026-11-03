/*
 * Copyright (c) 2020 Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef BOARD_H
#define BOARD_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//
// Included Files
//

#include "driverlib.h"
#include "device.h"

//*****************************************************************************
//
// PinMux Configurations
//
//*****************************************************************************

//
// ANALOG -> myANALOGPinMux0 Pinmux
//
//
// GPIO0 - GPIO Settings
//
#define GATE_G1_GPIO_PIN_CONFIG GPIO_0_GPIO0
//
// GPIO1 - GPIO Settings
//
#define GATE_G2_GPIO_PIN_CONFIG GPIO_1_GPIO1
//
// GPIO2 - GPIO Settings
//
#define GATE_G3_GPIO_PIN_CONFIG GPIO_2_GPIO2
//
// GPIO3 - GPIO Settings
//
#define GATE_G4_GPIO_PIN_CONFIG GPIO_3_GPIO3
//
// GPIO10 - GPIO Settings
//
#define GATE_G5_GPIO_PIN_CONFIG GPIO_10_GPIO10
//
// GPIO11 - GPIO Settings
//
#define GATE_G6_GPIO_PIN_CONFIG GPIO_11_GPIO11
//
// GPIO12 - GPIO Settings
//
#define GATE_SA_TOP_GPIO_PIN_CONFIG GPIO_12_GPIO12
//
// GPIO13 - GPIO Settings
//
#define GATE_SA_BOTTOM_GPIO_PIN_CONFIG GPIO_13_GPIO13
//
// GPIO6 - GPIO Settings
//
#define GATE_SB_TOP_GPIO_PIN_CONFIG GPIO_6_GPIO6
//
// GPIO7 - GPIO Settings
//
#define GATE_SB_BOTTOM_GPIO_PIN_CONFIG GPIO_7_GPIO7
//
// GPIO37 - GPIO Settings
//
#define GATE_SC_TOP_GPIO_PIN_CONFIG GPIO_37_GPIO37
//
// GPIO35 - GPIO Settings
//
#define GATE_SC_BOTTOM_GPIO_PIN_CONFIG GPIO_35_GPIO35

//
// SCIA -> mySCI0 Pinmux
//
//
// SCIA_RX - GPIO Settings
//
#define GPIO_PIN_SCIA_RX 28
#define mySCI0_SCIRX_GPIO 28
#define mySCI0_SCIRX_PIN_CONFIG GPIO_28_SCIA_RX
//
// SCIA_TX - GPIO Settings
//
#define GPIO_PIN_SCIA_TX 29
#define mySCI0_SCITX_GPIO 29
#define mySCI0_SCITX_PIN_CONFIG GPIO_29_SCIA_TX

//*****************************************************************************
//
// ADC Configurations
//
//*****************************************************************************
#define myADC0_BASE ADCA_BASE
#define myADC0_RESULT_BASE ADCARESULT_BASE
#define myADC0_SOC0 ADC_SOC_NUMBER0
#define myADC0_FORCE_SOC0 ADC_FORCE_SOC0
#define myADC0_SAMPLE_WINDOW_SOC0 66.66666666666667
#define myADC0_TRIGGER_SOURCE_SOC0 ADC_TRIGGER_SW_ONLY
#define myADC0_CHANNEL_SOC0 ADC_CH_ADCIN0
#define myADC0_SOC1 ADC_SOC_NUMBER1
#define myADC0_FORCE_SOC1 ADC_FORCE_SOC1
#define myADC0_SAMPLE_WINDOW_SOC1 66.66666666666667
#define myADC0_TRIGGER_SOURCE_SOC1 ADC_TRIGGER_SW_ONLY
#define myADC0_CHANNEL_SOC1 ADC_CH_ADCIN4
#define myADC0_SOC2 ADC_SOC_NUMBER2
#define myADC0_FORCE_SOC2 ADC_FORCE_SOC2
#define myADC0_SAMPLE_WINDOW_SOC2 66.66666666666667
#define myADC0_TRIGGER_SOURCE_SOC2 ADC_TRIGGER_SW_ONLY
#define myADC0_CHANNEL_SOC2 ADC_CH_ADCIN5
#define myADC0_SOC3 ADC_SOC_NUMBER3
#define myADC0_FORCE_SOC3 ADC_FORCE_SOC3
#define myADC0_SAMPLE_WINDOW_SOC3 66.66666666666667
#define myADC0_TRIGGER_SOURCE_SOC3 ADC_TRIGGER_SW_ONLY
#define myADC0_CHANNEL_SOC3 ADC_CH_ADCIN9
void myADC0_init();


//*****************************************************************************
//
// ASYSCTL Configurations
//
//*****************************************************************************

//*****************************************************************************
//
// GPIO Configurations
//
//*****************************************************************************
#define GATE_G1 0
void GATE_G1_init();
#define GATE_G2 1
void GATE_G2_init();
#define GATE_G3 2
void GATE_G3_init();
#define GATE_G4 3
void GATE_G4_init();
#define GATE_G5 10
void GATE_G5_init();
#define GATE_G6 11
void GATE_G6_init();
#define GATE_SA_TOP 12
void GATE_SA_TOP_init();
#define GATE_SA_BOTTOM 13
void GATE_SA_BOTTOM_init();
#define GATE_SB_TOP 6
void GATE_SB_TOP_init();
#define GATE_SB_BOTTOM 7
void GATE_SB_BOTTOM_init();
#define GATE_SC_TOP 37
void GATE_SC_TOP_init();
#define GATE_SC_BOTTOM 35
void GATE_SC_BOTTOM_init();

//*****************************************************************************
//
// SCI Configurations
//
//*****************************************************************************
#define mySCI0_BASE SCIA_BASE
#define mySCI0_BAUDRATE 115200
#define mySCI0_CONFIG_WLEN SCI_CONFIG_WLEN_8
#define mySCI0_CONFIG_STOP SCI_CONFIG_STOP_ONE
#define mySCI0_CONFIG_PAR SCI_CONFIG_PAR_NONE
void mySCI0_init();

//*****************************************************************************
//
// Board Configurations
//
//*****************************************************************************
void	Board_init();
void	ADC_init();
void	ASYSCTL_init();
void	GPIO_init();
void	SCI_init();
void	PinMux_init();

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // end of BOARD_H definition
