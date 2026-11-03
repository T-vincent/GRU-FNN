#ifndef IMC_ADC_NN_CONFIG_H_
#define IMC_ADC_NN_CONFIG_H_

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "imc_user_config.h"

/* Keep the official four-channel example's ADC instance and SOC layout. */
#define IMC_ADC_BASE                 myADC0_BASE
#define IMC_ADC_RESULT_BASE          myADC0_RESULT_BASE
#define IMC_ADC_SOC_VA               ADC_SOC_NUMBER0   /* ADCINA0 */
#define IMC_ADC_SOC_VB               ADC_SOC_NUMBER1   /* ADCINA4 */
#define IMC_ADC_SOC_VC               ADC_SOC_NUMBER2   /* ADCINA5 */
#define IMC_ADC_SOC_SPARE            ADC_SOC_NUMBER3   /* ADCINA9 */
#define IMC_ADC_INT_NUMBER           ADC_INT_NUMBER1
#define IMC_ADC_FORCE_MASK           (ADC_FORCE_SOC0 | ADC_FORCE_SOC1 | \
                                      ADC_FORCE_SOC2 | ADC_FORCE_SOC3)

#define IMC_CONTROL_FREQUENCY_HZ      (20000UL)
#define IMC_CONTROL_PERIOD_COUNTS     (DEVICE_SYSCLK_FREQ / \
                                       IMC_CONTROL_FREQUENCY_HZ)

/* Preserve the official example's conversion convention initially. */
#define IMC_ADC_REFERENCE_VOLTAGE     (3.3f)
#define IMC_ADC_CODE_DIVISOR          (4096.0f)

/* Typhoon AO scale: V_AO = offset + IMC_VOLTAGE_GAIN * V_phase. */
#define IMC_SENSOR_GAIN_VA            (IMC_VOLTAGE_GAIN)
#define IMC_SENSOR_GAIN_VB            (IMC_VOLTAGE_GAIN)
#define IMC_SENSOR_GAIN_VC            (IMC_VOLTAGE_GAIN)

#define IMC_ADC_WAIT_TIMEOUT_LOOPS    (50000UL)
#define IMC_TWO_PI_F                  (6.2831853071795864769f)

#endif /* IMC_ADC_NN_CONFIG_H_ */
