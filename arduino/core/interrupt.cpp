/*
    Copyright (c) 2014 RedBearLab, All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**************************************************************************
 * Include Files
 **************************************************************************/
#include "interrupt.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "assert.h"
extern "C"{
#include "app_error.h"
}
#include "app_util_platform.h"
#include "nrf_gpiote.h"
#include "inter_periph_com.h"
#include "nrf51822_arduino_conversion.h"

/**************************************************************************
 * Manifest Constants
 **************************************************************************/
#define NB_GPIOTE_CHANNELS (4U)
#define MAX_NB_EXT_IT NB_GPIOTE_CHANNELS

/**************************************************************************
 * Type Definitions
 **************************************************************************/
typedef struct{
	uint32_t u32_nrfPin;
	EPinTrigger e_trigger;
	ext_it_handler_t cb;
	void* p_payload;
}SExtInterrupt;

/**************************************************************************
 * Variables
 **************************************************************************/
static uint8_t softdevice_enabled;

/**
 * External IT, index = GPIOTE channel
 */
SExtInterrupt extIT[MAX_NB_EXT_IT] = {{INVALID_PIN, OUT_OF_ENUM_PIN_TRIGGER, NULL},
		{INVALID_PIN, OUT_OF_ENUM_PIN_TRIGGER, NULL},
		{INVALID_PIN, OUT_OF_ENUM_PIN_TRIGGER, NULL},
		{INVALID_PIN, OUT_OF_ENUM_PIN_TRIGGER, NULL}};
/**************************************************************************
 * Macros
 **************************************************************************/

/**************************************************************************
 * Global Functions
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/
/***
 * Enable GPIOTE interrupt on given channel (refer reference manual 14.2)
 * @param arg_u8_gpioteChannel
 */
static void enableGPIOTEInterrupt( uint8_t arg_u8_gpioteChannel );

/**
 * Get GPIOTE channel mask corresponding to given channel
 * @param arg_u8_gpioteChannel
 * @return
 */
static uint32_t extItToGPIOTEChannelMask( uint8_t arg_u8_gpioteChannel );

static void GPIOTE_handler( void );

/**************************************************************************
 * Global Functions Definitions
 **************************************************************************/

void delay_ex_interrupter(uint32_t us) 
{
	while (us--)
    {
        __ASM(" NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t"
        " NOP\n\t");
    };
}


void attachInterrupt(uint32_t arg_u32_pin, ext_it_handler_t arg_pf_itHandler, EPinTrigger arg_e_pinTrigger, void* arg_p_handlerPayload)
{
	uint32_t nrf_pin, err_code = NRF_SUCCESS;
	nrf_gpiote_polarity_t loc_e_gpiotePol = NRF_GPIOTE_POLARITY_TOGGLE;
	uint8_t channel;
	
	channel = gpioteChannelFind();
	if(channel == UNAVAILABLE_GPIOTE_CHANNEL)
	{
		assert(false);
		return;
	}
	
	nrf_pin = arduinoToVariantPin(arg_u32_pin);
	assert(INVALID_PIN != nrf_pin);

	loc_e_gpiotePol = pinTriggerToNRF51GPIOTEPol(arg_e_pinTrigger);
	gpioteChannelSet(channel);
	extIT[channel].u32_nrfPin = nrf_pin;
	extIT[channel].e_trigger = arg_e_pinTrigger;
	extIT[channel].cb = arg_pf_itHandler;
	extIT[channel].p_payload = arg_p_handlerPayload;
	NRF_GPIOTE->CONFIG[channel] =  (loc_e_gpiotePol << GPIOTE_CONFIG_POLARITY_Pos)
							| (nrf_pin << GPIOTE_CONFIG_PSEL_Pos)
							| (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos);
	enableGPIOTEInterrupt(channel);
	IntController_linkInterrupt(GPIOTE_IRQn, GPIOTE_handler);

	err_code = sd_softdevice_is_enabled(&softdevice_enabled);
	APP_ERROR_CHECK(err_code);
	if(softdevice_enabled == 0)
	{	
		NVIC_SetPriority(GPIOTE_IRQn, APP_IRQ_PRIORITY_LOW);
		NVIC_EnableIRQ(GPIOTE_IRQn);
	}
	else
	{
		err_code = sd_nvic_SetPriority(GPIOTE_IRQn, APP_IRQ_PRIORITY_LOW);
		APP_ERROR_CHECK(err_code);
		err_code = sd_nvic_EnableIRQ(GPIOTE_IRQn);
		APP_ERROR_CHECK(err_code);
	}
}


void detachInterrupt(uint32_t arg_u32_pin )
{	
	uint32_t loc_u32_pin, err_code = NRF_SUCCESS;
	uint8_t loc_u8_channel = UNAVAILABLE_GPIOTE_CHANNEL;
	uint8_t loc_u8_gpioteChannel;

	//Get the GPIOTE Channel
	loc_u32_pin = arduinoToVariantPin(arg_u32_pin);
	assert(INVALID_PIN != loc_u32_pin);
	
	for(loc_u8_gpioteChannel = 0; loc_u8_gpioteChannel < MAX_NB_EXT_IT; loc_u8_gpioteChannel++)
	{
		if(loc_u32_pin == extIT[loc_u8_gpioteChannel].u32_nrfPin)
		{
			loc_u8_channel = loc_u8_gpioteChannel;
			break;
		}
	}
	if(loc_u8_channel == UNAVAILABLE_GPIOTE_CHANNEL){
		/** interrupt already detached ? */
		assert(false);
		return;
	}
	
	gpioteChannelClean(loc_u8_channel);
	extIT[loc_u8_channel].e_trigger = OUT_OF_ENUM_PIN_TRIGGER;
	extIT[loc_u8_channel].u32_nrfPin = INVALID_PIN;
	extIT[loc_u8_channel].cb = NULL;
	extIT[loc_u8_channel].p_payload = NULL;

	//if all interrupt detach, disable GPIOTE_IRQn
	loc_u8_gpioteChannel = 0;
	while((loc_u8_gpioteChannel < MAX_NB_EXT_IT)
			&& (extIT[loc_u8_gpioteChannel].u32_nrfPin == INVALID_PIN))
	{
		loc_u8_gpioteChannel++;
	}

	if(loc_u8_gpioteChannel == MAX_NB_EXT_IT)
	{
		err_code = sd_softdevice_is_enabled(&softdevice_enabled);
		APP_ERROR_CHECK(err_code);
		if(softdevice_enabled == 0)
		{
			NVIC_DisableIRQ(GPIOTE_IRQn);
		}
		else
		{
			err_code = sd_nvic_DisableIRQ(GPIOTE_IRQn);
			APP_ERROR_CHECK(err_code);
		}
		IntController_unlinkInterrupt(GPIOTE_IRQn);
	}
}

/**************************************************************************
 * Local Functions Definitions
 **************************************************************************/
void enableGPIOTEInterrupt( uint8_t arg_u8_gpioteChannel )
{
	if(arg_u8_gpioteChannel == 0)
	{
		NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos;
	}
	else if(arg_u8_gpioteChannel == 1)
	{
		NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN1_Set << GPIOTE_INTENSET_IN1_Pos;
	}
	else if(arg_u8_gpioteChannel == 2)
	{
		NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN2_Set << GPIOTE_INTENSET_IN2_Pos;
	}
	else if(arg_u8_gpioteChannel == 3)
	{
		NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN3_Set << GPIOTE_INTENSET_IN3_Pos;
	}
	else
	{
		/** invalid channel given */
		assert(false);
	}
}

uint32_t extItToGPIOTEChannelMask( uint8_t arg_u8_gpioteChannel )
{
	uint32_t loc_u32_mask = 0;
	if(arg_u8_gpioteChannel == 0)
	{
		loc_u32_mask = GPIOTE_INTENSET_IN0_Msk;
	}
	else if(arg_u8_gpioteChannel == 1)
	{
		loc_u32_mask = GPIOTE_INTENSET_IN1_Msk;
	}
	else if(arg_u8_gpioteChannel == 2)
	{
		loc_u32_mask = GPIOTE_INTENSET_IN2_Msk;
	}
	else if(arg_u8_gpioteChannel == 3)
	{
		loc_u32_mask = GPIOTE_INTENSET_IN3_Msk;
	}
	else
	{
		assert(false);
	}
	return loc_u32_mask;

}

//void GPIOTE_IRQHandler(void)
static void GPIOTE_handler( void )
{	
	uint8_t loc_u8_gpioteChannel = 0;
	

	//TODO RP 26/01/2015- check it?
	//delay_ex_interrupter(30000);

	for(loc_u8_gpioteChannel = 0; loc_u8_gpioteChannel < MAX_NB_EXT_IT; loc_u8_gpioteChannel++)
	{
		if((extIT[loc_u8_gpioteChannel].u32_nrfPin != INVALID_PIN)
				&& (extItToGPIOTEChannelMask(loc_u8_gpioteChannel) & NRF_GPIOTE->INTENSET)
				&& (NRF_GPIOTE->EVENTS_IN[loc_u8_gpioteChannel] == 1))
		{	
			if(extIT[loc_u8_gpioteChannel].e_trigger == RISING)
			{
				if(((NRF_GPIO->IN >> extIT[loc_u8_gpioteChannel].u32_nrfPin) & 1UL) == 1  )
				{
					NRF_GPIOTE->EVENTS_IN[loc_u8_gpioteChannel] = 0;
					extIT[loc_u8_gpioteChannel].cb(extIT[loc_u8_gpioteChannel].p_payload);
				}
				else
				{
					/** invalid interrupt triggered */
					NRF_GPIOTE->EVENTS_IN[loc_u8_gpioteChannel] = 0;
				}
			}
			else if( extIT[loc_u8_gpioteChannel].e_trigger == FALLING )
			{
				if(((NRF_GPIO->IN >> extIT[loc_u8_gpioteChannel].u32_nrfPin) & 1UL) == 0  )
				{
					NRF_GPIOTE->EVENTS_IN[loc_u8_gpioteChannel] = 0;
					extIT[loc_u8_gpioteChannel].cb(extIT[loc_u8_gpioteChannel].p_payload);
				}
				else
				{
					/** invalid interrupt triggered */
					NRF_GPIOTE->EVENTS_IN[loc_u8_gpioteChannel] = 0;
				}		
			}
			else /** CHANGE */
			{
				NRF_GPIOTE->EVENTS_IN[loc_u8_gpioteChannel] = 0;
				extIT[loc_u8_gpioteChannel].cb(extIT[loc_u8_gpioteChannel].p_payload);
			}
		}
	}
}







