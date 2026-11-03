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

#include "board.h"

//*****************************************************************************
//
// Board Configurations
// Initializes the rest of the modules.
// Call this function in your application if you wish to do all module
// initialization.
// If you wish to not use some of the initializations, instead of the
// Board_init use the individual Module_inits
//
//*****************************************************************************
void Board_init()
{
	EALLOW;

	PinMux_init();
	ASYSCTL_init();
	ADC_init();
	GPIO_init();
	SCI_init();

	EDIS;
}

//*****************************************************************************
//
// PINMUX Configurations
//
//*****************************************************************************
void PinMux_init()
{
	//
	// PinMux for modules assigned to CPU1
	//

	//
	// ANALOG -> myANALOGPinMux0 Pinmux
	//
	// Analog PinMux for A0, B15, C15, DACA_OUT
	GPIO_setPinConfig(GPIO_231_GPIO231);
	// AIO -> Analog mode selected
	GPIO_setAnalogMode(231, GPIO_ANALOG_ENABLED);
	// Analog PinMux for A4, B8
	GPIO_setPinConfig(GPIO_225_GPIO225);
	// AIO -> Analog mode selected
	GPIO_setAnalogMode(225, GPIO_ANALOG_ENABLED);
	// Analog PinMux for A5
	GPIO_setPinConfig(GPIO_249_GPIO249);
	// AIO -> Analog mode selected
	GPIO_setAnalogMode(249, GPIO_ANALOG_ENABLED);
	// Analog PinMux for A9, GPIO227
	GPIO_setPinConfig(GPIO_227_GPIO227);
	// AGPIO -> Analog mode selected
	GPIO_setAnalogMode(227, GPIO_ANALOG_ENABLED);
	// GPIO0 -> GATE_G1 Pinmux
	GPIO_setPinConfig(GPIO_0_GPIO0);
	// GPIO1 -> GATE_G2 Pinmux
	GPIO_setPinConfig(GPIO_1_GPIO1);
	// GPIO2 -> GATE_G3 Pinmux
	GPIO_setPinConfig(GPIO_2_GPIO2);
	// GPIO3 -> GATE_G4 Pinmux
	GPIO_setPinConfig(GPIO_3_GPIO3);
	// GPIO10 -> GATE_G5 Pinmux
	GPIO_setPinConfig(GPIO_10_GPIO10);
	// A24, D0, E0, GPIO11 -> GATE_G6 Pinmux
	GPIO_setPinConfig(GPIO_11_GPIO11);
	// AGPIO -> GPIO mode selected
	GPIO_setAnalogMode(11, GPIO_ANALOG_DISABLED);
	// A20, B20, C20, GPIO12 -> GATE_SA_TOP Pinmux
	GPIO_setPinConfig(GPIO_12_GPIO12);
	// AGPIO -> GPIO mode selected
	GPIO_setAnalogMode(12, GPIO_ANALOG_DISABLED);
	// A19, B19, C19, GPIO13 -> GATE_SA_BOTTOM Pinmux
	GPIO_setPinConfig(GPIO_13_GPIO13);
	// AGPIO -> GPIO mode selected
	GPIO_setAnalogMode(13, GPIO_ANALOG_DISABLED);
	// GPIO6 -> GATE_SB_TOP Pinmux
	GPIO_setPinConfig(GPIO_6_GPIO6);
	// GPIO7 -> GATE_SB_BOTTOM Pinmux
	GPIO_setPinConfig(GPIO_7_GPIO7);
	// GPIO37/TDO -> GATE_SC_TOP Pinmux
	GPIO_setPinConfig(GPIO_37_GPIO37);
	// GPIO35/TDI -> GATE_SC_BOTTOM Pinmux
	GPIO_setPinConfig(GPIO_35_GPIO35);
	//
	// SCIA -> mySCI0 Pinmux
	//
	GPIO_setPinConfig(mySCI0_SCIRX_PIN_CONFIG);
	// AGPIO -> GPIO mode selected
	GPIO_setAnalogMode(28, GPIO_ANALOG_DISABLED);
	GPIO_setPadConfig(mySCI0_SCIRX_GPIO, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
	GPIO_setQualificationMode(mySCI0_SCIRX_GPIO, GPIO_QUAL_ASYNC);

	GPIO_setPinConfig(mySCI0_SCITX_PIN_CONFIG);
	GPIO_setPadConfig(mySCI0_SCITX_GPIO, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
	GPIO_setQualificationMode(mySCI0_SCITX_GPIO, GPIO_QUAL_ASYNC);


}

//*****************************************************************************
//
// ADC Configurations
//
//*****************************************************************************
void ADC_init(){
	myADC0_init();
}

void myADC0_init(){
	//
	// ADC Initialization: Write ADC configurations and power up the ADC
	//
	// Set the analog voltage reference selection and ADC module's offset trims.
	// This function sets the analog voltage reference to internal (with the reference voltage of 1.65V or 2.5V) or external for ADC
	// which is same as ASysCtl APIs.
	//
	ADC_setVREF(myADC0_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);
	//
	// Configures the analog-to-digital converter module prescaler.
	//
	ADC_setPrescaler(myADC0_BASE, ADC_CLK_DIV_2_0);
	//
	// Sets the timing of the end-of-conversion pulse
	//
	ADC_setInterruptPulseMode(myADC0_BASE, ADC_PULSE_END_OF_CONV);
	//
	// Powers up the analog-to-digital converter core.
	//
	ADC_enableConverter(myADC0_BASE);
	//
	// Delay for 1ms to allow ADC time to power up
	//
	DEVICE_DELAY_US(5000);
	//
	// Enable alternate timings for DMA trigger
	//
	ADC_enableAltDMATiming(myADC0_BASE);
	//
	// SOC Configuration: Setup ADC EPWM channel and trigger settings
	//
	// Disables SOC burst mode.
	//
	ADC_disableBurstMode(myADC0_BASE);
	//
	// Sets the priority mode of the SOCs.
	//
	ADC_setSOCPriority(myADC0_BASE, ADC_PRI_ALL_ROUND_ROBIN);
	//
	// Start of Conversion 0 Configuration
	//
	//
	// Disables Sample Capacitor to reset after each conversion.
	//
	ADC_disableSampleCAPReset(myADC0_BASE, ADC_SOC_NUMBER0);
	//
	// Configures a start-of-conversion (SOC) in the ADC and its interrupt SOC trigger.
	// 	  	SOC number		: 0
	//	  	Trigger			: ADC_TRIGGER_SW_ONLY
	//	  	Channel			: ADC_CH_ADCIN0
	//	 	Sample Window	: 10 SYSCLK cycles
	//		Interrupt Trigger: ADC_INT_SOC_TRIGGER_NONE
	//
	ADC_setupSOC(myADC0_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN0, 10U);
	ADC_setInterruptSOCTrigger(myADC0_BASE, ADC_SOC_NUMBER0, ADC_INT_SOC_TRIGGER_NONE);
	//
	// Start of Conversion 1 Configuration
	//
	//
	// Disables Sample Capacitor to reset after each conversion.
	//
	ADC_disableSampleCAPReset(myADC0_BASE, ADC_SOC_NUMBER1);
	//
	// Configures a start-of-conversion (SOC) in the ADC and its interrupt SOC trigger.
	// 	  	SOC number		: 1
	//	  	Trigger			: ADC_TRIGGER_SW_ONLY
	//	  	Channel			: ADC_CH_ADCIN4
	//	 	Sample Window	: 10 SYSCLK cycles
	//		Interrupt Trigger: ADC_INT_SOC_TRIGGER_NONE
	//
	ADC_setupSOC(myADC0_BASE, ADC_SOC_NUMBER1, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN4, 10U);
	ADC_setInterruptSOCTrigger(myADC0_BASE, ADC_SOC_NUMBER1, ADC_INT_SOC_TRIGGER_NONE);
	//
	// Start of Conversion 2 Configuration
	//
	//
	// Disables Sample Capacitor to reset after each conversion.
	//
	ADC_disableSampleCAPReset(myADC0_BASE, ADC_SOC_NUMBER2);
	//
	// Configures a start-of-conversion (SOC) in the ADC and its interrupt SOC trigger.
	// 	  	SOC number		: 2
	//	  	Trigger			: ADC_TRIGGER_SW_ONLY
	//	  	Channel			: ADC_CH_ADCIN5
	//	 	Sample Window	: 10 SYSCLK cycles
	//		Interrupt Trigger: ADC_INT_SOC_TRIGGER_NONE
	//
	ADC_setupSOC(myADC0_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN5, 10U);
	ADC_setInterruptSOCTrigger(myADC0_BASE, ADC_SOC_NUMBER2, ADC_INT_SOC_TRIGGER_NONE);
	//
	// Start of Conversion 3 Configuration
	//
	//
	// Disables Sample Capacitor to reset after each conversion.
	//
	ADC_disableSampleCAPReset(myADC0_BASE, ADC_SOC_NUMBER3);
	//
	// Configures a start-of-conversion (SOC) in the ADC and its interrupt SOC trigger.
	// 	  	SOC number		: 3
	//	  	Trigger			: ADC_TRIGGER_SW_ONLY
	//	  	Channel			: ADC_CH_ADCIN9
	//	 	Sample Window	: 10 SYSCLK cycles
	//		Interrupt Trigger: ADC_INT_SOC_TRIGGER_NONE
	//
	ADC_setupSOC(myADC0_BASE, ADC_SOC_NUMBER3, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN9, 10U);
	ADC_setInterruptSOCTrigger(myADC0_BASE, ADC_SOC_NUMBER3, ADC_INT_SOC_TRIGGER_NONE);
	//
	// ADC Interrupt 1 Configuration
	// 		Source	: ADC_INT_TRIGGER_EOC3
	// 		Interrupt Source: enabled
	// 		Continuous Mode	: disabled
	//
	//
	ADC_setInterruptSource(myADC0_BASE, ADC_INT_NUMBER1, ADC_INT_TRIGGER_EOC3);
	ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
	ADC_disableContinuousMode(myADC0_BASE, ADC_INT_NUMBER1);
	ADC_enableInterrupt(myADC0_BASE, ADC_INT_NUMBER1);
}


//*****************************************************************************
//
// ASYSCTL Configurations
//
//*****************************************************************************
void ASYSCTL_init(){
	//
	// asysctl initialization
	//
	// Disables the temperature sensor output to the ADC.
	//
	ASysCtl_disableTemperatureSensor();
	//
	// Set the analog voltage reference selection to internal.
	//
	ASysCtl_setAnalogReferenceInternal( ASYSCTL_ANAREF_INTREF_ADCA | ASYSCTL_ANAREF_INTREF_ADCB | ASYSCTL_ANAREF_INTREF_ADCC | ASYSCTL_ANAREF_INTREF_ADCD | ASYSCTL_ANAREF_INTREF_ADCE );

	//
	// Set the internal analog voltage reference selection to 1.65V.
	//
	ASysCtl_setAnalogReference1P65( ASYSCTL_ANAREF_ADCA | ASYSCTL_ANAREF_ADCB | ASYSCTL_ANAREF_ADCC | ASYSCTL_ANAREF_ADCD | ASYSCTL_ANAREF_ADCE );
}

//*****************************************************************************
//
// GPIO Configurations
//
//*****************************************************************************
void GPIO_init(){
	GATE_G1_init();
	GATE_G2_init();
	GATE_G3_init();
	GATE_G4_init();
	GATE_G5_init();
	GATE_G6_init();
	GATE_SA_TOP_init();
	GATE_SA_BOTTOM_init();
	GATE_SB_TOP_init();
	GATE_SB_BOTTOM_init();
	GATE_SC_TOP_init();
	GATE_SC_BOTTOM_init();
}

void GATE_G1_init(){
	GPIO_writePin(GATE_G1, 0);
	GPIO_setPadConfig(GATE_G1, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_G1, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_G1, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_G1, GPIO_CORE_CPU1);
}
void GATE_G2_init(){
	GPIO_writePin(GATE_G2, 0);
	GPIO_setPadConfig(GATE_G2, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_G2, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_G2, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_G2, GPIO_CORE_CPU1);
}
void GATE_G3_init(){
	GPIO_writePin(GATE_G3, 0);
	GPIO_setPadConfig(GATE_G3, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_G3, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_G3, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_G3, GPIO_CORE_CPU1);
}
void GATE_G4_init(){
	GPIO_writePin(GATE_G4, 0);
	GPIO_setPadConfig(GATE_G4, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_G4, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_G4, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_G4, GPIO_CORE_CPU1);
}
void GATE_G5_init(){
	GPIO_writePin(GATE_G5, 0);
	GPIO_setPadConfig(GATE_G5, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_G5, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_G5, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_G5, GPIO_CORE_CPU1);
}
void GATE_G6_init(){
	GPIO_writePin(GATE_G6, 0);
	GPIO_setPadConfig(GATE_G6, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_G6, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_G6, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_G6, GPIO_CORE_CPU1);
}
void GATE_SA_TOP_init(){
	GPIO_writePin(GATE_SA_TOP, 0);
	GPIO_setPadConfig(GATE_SA_TOP, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_SA_TOP, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_SA_TOP, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_SA_TOP, GPIO_CORE_CPU1);
}
void GATE_SA_BOTTOM_init(){
	GPIO_writePin(GATE_SA_BOTTOM, 0);
	GPIO_setPadConfig(GATE_SA_BOTTOM, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_SA_BOTTOM, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_SA_BOTTOM, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_SA_BOTTOM, GPIO_CORE_CPU1);
}
void GATE_SB_TOP_init(){
	GPIO_writePin(GATE_SB_TOP, 0);
	GPIO_setPadConfig(GATE_SB_TOP, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_SB_TOP, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_SB_TOP, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_SB_TOP, GPIO_CORE_CPU1);
}
void GATE_SB_BOTTOM_init(){
	GPIO_writePin(GATE_SB_BOTTOM, 0);
	GPIO_setPadConfig(GATE_SB_BOTTOM, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_SB_BOTTOM, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_SB_BOTTOM, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_SB_BOTTOM, GPIO_CORE_CPU1);
}
void GATE_SC_TOP_init(){
	GPIO_writePin(GATE_SC_TOP, 0);
	GPIO_setPadConfig(GATE_SC_TOP, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_SC_TOP, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_SC_TOP, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_SC_TOP, GPIO_CORE_CPU1);
}
void GATE_SC_BOTTOM_init(){
	GPIO_writePin(GATE_SC_BOTTOM, 0);
	GPIO_setPadConfig(GATE_SC_BOTTOM, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(GATE_SC_BOTTOM, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(GATE_SC_BOTTOM, GPIO_DIR_MODE_OUT);
	GPIO_setControllerCore(GATE_SC_BOTTOM, GPIO_CORE_CPU1);
}

//*****************************************************************************
//
// SCI Configurations
//
//*****************************************************************************
void SCI_init(){
	mySCI0_init();
}

void mySCI0_init(){
	SCI_clearInterruptStatus(mySCI0_BASE, SCI_INT_RXFF | SCI_INT_TXFF | SCI_INT_FE | SCI_INT_OE | SCI_INT_PE | SCI_INT_RXERR | SCI_INT_RXRDY_BRKDT | SCI_INT_TXRDY);
	SCI_clearOverflowStatus(mySCI0_BASE);
	SCI_disableFIFO(mySCI0_BASE);
	SCI_resetChannels(mySCI0_BASE);
	SCI_setConfig(mySCI0_BASE, DEVICE_LSPCLK_FREQ, mySCI0_BAUDRATE, (SCI_CONFIG_WLEN_8|SCI_CONFIG_STOP_ONE|SCI_CONFIG_PAR_NONE));
	SCI_disableLoopback(mySCI0_BASE);
	SCI_performSoftwareReset(mySCI0_BASE);
	SCI_enableModule(mySCI0_BASE);
}

