#include "Arduino.h"
#include "nrf_gpio.h"
#include "nrf51822_arduino_conversion.h"
#include "nrf_gpiote.h"
#include "inter_periph_com.h"
#include "assert.h"

/**********************************************************************
name :
function : 
**********************************************************************/
void pinMode( uint32_t ulPin, EPinMode arg_e_pinMode )
{	
	uint32_t pin;
	
	pin = arduinoToVariantPin(ulPin);
	if(pin < 31)
	{
		if(arg_e_pinMode == INPUT
				|| arg_e_pinMode == INPUT_NOPULL
				|| arg_e_pinMode == INPUT_PULLDOWN
				|| arg_e_pinMode == INPUT_PULLUP)
		{
			nrf_gpio_cfg_input(pin, inputPinModeToNRF51Pull(arg_e_pinMode));
		}
		else if(arg_e_pinMode == OUTPUT
				|| arg_e_pinMode == OUTPUT_S0S1
				|| arg_e_pinMode == OUTPUT_H0S1
				|| arg_e_pinMode == OUTPUT_S0H1
				|| arg_e_pinMode == OUTPUT_H0H1
				|| arg_e_pinMode == OUTPUT_D0S1
				|| arg_e_pinMode == OUTPUT_D0H1
				|| arg_e_pinMode == OUTPUT_S0D1
				|| arg_e_pinMode == OUTPUT_H0D1)
		{
			NRF_GPIO->PIN_CNF[pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
									| (pinModeToNRF51Drive(arg_e_pinMode) << GPIO_PIN_CNF_DRIVE_Pos)
									| (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
									| (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
									| (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
		}
		else
		{
			assert(false);
		}
	}
}
/**********************************************************************
name :
function : 
**********************************************************************/
void digitalWrite( uint32_t ulPin, uint32_t ulVal )
{
	uint32_t pin;
	pin = arduinoToVariantPin(ulPin);
	if(pin < 31)
	{	//if pin is used for analog, release it.
		ppiOffFromGPIO(pin);
		if (ulVal)
			NRF_GPIO->OUTSET = (1 << pin);
		else
			NRF_GPIO->OUTCLR = (1 << pin);
	}
}
/**********************************************************************
name :
function : 
**********************************************************************/
int digitalRead( uint32_t ulPin )
{
	uint32_t pin;
	pin = arduinoToVariantPin(ulPin);
	if(pin < 31)
	{	
		ppiOffFromGPIO(pin);
		return ((NRF_GPIO->IN >> pin) & 1UL);
	}
	else
	{
		return 0;
	}
}
