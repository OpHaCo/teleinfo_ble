#include "Arduino.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "nrf_gpiote.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "inter_periph_com.h"
#include "interrupt_controller.h"

/**************************************************************************
 * Global Functions Declarations
 **************************************************************************/
void ADC_IRQHandler(void);

/**************************************************************************
 * Static Functions Declarations
 **************************************************************************/

/**
 * Called in application context when an ADC conversion has completed
 * @param p_event_data
 * @param event_size
 */
static void app_adc_evt_get(void * p_event_data, uint16_t event_size);
/**************************************************************************
 * Variables
 **************************************************************************/

static uint32_t PWM_Channels_Value[3] = {((2^PWM_RESOLUTION) - 1), ((2^PWM_RESOLUTION) - 1), ((2^PWM_RESOLUTION) - 1)};		//the PWM value to update
static uint8_t  PWM_Channels_Start[3] = {0, 0, 0};																			//1:start, 0:stop

//initialize default
static uint32_t analogReference_ext_type 	= EXT_REFSEL_NONE;
static uint32_t analogReference_type 	 	= REFSEL_VDD_1_3_PS;
static uint32_t analogReference_inpsel_type = INPSEL_AIN_1_3_PS;
//
static uint8_t analogReadResolution_bit 	= READ_CURRENT_RESOLUTION;
static uint8_t analogWriteResolution_bit	= WRITE_CURRENT_RESOLUTION;

//current converson on going - only handle 1 conversion at a time
static on_adc_conversion_handler_t onADCReadCb = NULL;

/**********************************************************************
name :
function : 
**********************************************************************/
void analogInpselType( uint32_t type)
{
	if(type == INPSEL_AIN_NO_PS || type == INPSEL_AIN_2_3_PS || type == INPSEL_AIN_1_3_PS || type == INPSEL_VDD_2_3_PS || type == INPSEL_VDD_1_3_PS)
	{
		analogReference_inpsel_type = type;
	}
}
/**********************************************************************
name :
function : Using an external reference voltage cannot be more than: 1.3 V
**********************************************************************/
void analogReference( uint32_t type )
{	
	if( type == REFSEL_VBG || type == REFSEL_VDD_1_2_PS || type == REFSEL_VDD_1_3_PS)
	{
		analogReference_ext_type = EXT_REFSEL_NONE;
		analogReference_type = type;
	}
	else if( type == EXT_REFSEL_AREF0 || type == EXT_REFSEL_AREF1 )
	{
		analogReference_ext_type = type;
		analogReference_type = REFSEL_EXT;		
	}
}
/**********************************************************************
name :
function : 
**********************************************************************/
void analogReadResolution( uint8_t resolution)
{	
	if(resolution <= 10 )
	{
		analogReadResolution_bit = resolution;
	}
}
/**********************************************************************
name :
function : 
**********************************************************************/
void analogWriteResolution( uint8_t resolution )
{
	if(resolution <= 16 )
	{
		analogWriteResolution_bit = resolution;
	}
}

/**********************************************************************
name :
function : 
**********************************************************************/
static inline uint32_t conversion_Resolution(uint32_t value, uint32_t from, uint32_t to) {
	
	if (from == to)
		return value;
	if (from > to)
		return value >> (from-to);
	else
		return value << (to-from);
}
/**********************************************************************
name :
function : 
**********************************************************************/
uint32_t analogRead(uint32_t pin)
{	
    uint32_t value = 0;
	uint32_t pValue = 0;
	uint32_t nrf_pin = 0;

	//PIN transform to nRF51822
	nrf_pin = arduinoToVariantPin(pin);
	APP_ERROR_CHECK_BOOL(INVALID_PIN != nrf_pin);
	
	/** Only 1 conversion at a time */
	APP_ERROR_CHECK_BOOL(onADCReadCb == NULL);

	pValue = (1 << (nrf_pin + 1));
	NRF_ADC->CONFIG = ( ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) |
						 ( analogReference_inpsel_type << ADC_CONFIG_INPSEL_Pos) |
						 ( analogReference_type << ADC_CONFIG_REFSEL_Pos) |
						 ( pValue << ADC_CONFIG_PSEL_Pos) |
						 ( analogReference_ext_type << ADC_CONFIG_EXTREFSEL_Pos);
		
	NRF_ADC->INTENCLR = ADC_INTENCLR_END_Msk;
	NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled << ADC_ENABLE_ENABLE_Pos;
	NRF_ADC->TASKS_START = 1;
		
	while( (NRF_ADC->BUSY & ADC_BUSY_BUSY_Msk) == (ADC_BUSY_BUSY_Busy << ADC_BUSY_BUSY_Pos) );
		
	value = NRF_ADC->RESULT;
		
	value = conversion_Resolution(value, ADC_RESOLUTION, analogReadResolution_bit);
	NRF_ADC->ENABLE = (ADC_ENABLE_ENABLE_Disabled 	<< ADC_ENABLE_ENABLE_Pos);
	NRF_ADC->CONFIG = 	(ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos) |
						(ADC_CONFIG_INPSEL_SupplyTwoThirdsPrescaling << ADC_CONFIG_INPSEL_Pos) |
						(ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) |
						(ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos) |
						(ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos);
	return value;
}

bool analogAsyncRead( uint32_t ulPin,  on_adc_conversion_handler_t arg_adcCb)
{
	uint32_t nrf_pin = 0;
	uint32_t pValue = 0;

    if(onADCReadCb != NULL)
    {
    	/** A conversion already on going */
    	return false;
    }
    APP_ERROR_CHECK_BOOL(arg_adcCb != NULL);
    onADCReadCb = arg_adcCb;

	//PIN transform to nRF51822
	nrf_pin = arduinoToVariantPin(ulPin);
	APP_ERROR_CHECK_BOOL(INVALID_PIN != nrf_pin);

	pValue = (1 << (nrf_pin + 1));

    // Configure ADC
    NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk;
    NRF_ADC->CONFIG     = (ADC_CONFIG_RES_10bit                        << ADC_CONFIG_RES_Pos)     |
                          (analogReference_inpsel_type                 << ADC_CONFIG_INPSEL_Pos)  |
                          (analogReference_type                        << ADC_CONFIG_REFSEL_Pos)  |
                          (pValue                                      << ADC_CONFIG_PSEL_Pos)    |
                          (analogReference_ext_type                    << ADC_CONFIG_EXTREFSEL_Pos);
    NRF_ADC->EVENTS_END = 0;
    NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled;

    // Enable ADC interrupt
    if(!IntController_enableIRQ(ADC_IRQn, NRF_APP_PRIORITY_LOW))
    {
    	return false;
    }

    NRF_ADC->EVENTS_END  = 0;    // Stop any running conversions.
    NRF_ADC->TASKS_START = 1;
}

/**********************************************************************
name :
function : PWM update
**********************************************************************/
static void update_PWM_value(uint32_t ulPin, uint32_t ulValue, uint32_t PWM_channel)
{
	uint32_t channel;

	channel = GPIOTE_Channel_for_Analog[PWM_channel];
	PWM_Channels_Value[PWM_channel] = ((2^PWM_RESOLUTION) - 1) - conversion_Resolution(ulValue, analogWriteResolution_bit, PWM_RESOLUTION);

	if ((NRF_GPIOTE->CONFIG[channel] & GPIOTE_CONFIG_MODE_Msk) == (GPIOTE_CONFIG_MODE_Disabled << GPIOTE_CONFIG_MODE_Pos))
	{	
		PWM_Channels_Start[PWM_channel] = 1;
	}
	
}

/**********************************************************************
name :  
function : called by wiring_digital.c
**********************************************************************/
void PPI_ON_TIMER_GPIO(uint32_t gpiote_channel, NRF_TIMER_Type* Timer, uint32_t CC_channel)
{	
	uint32_t err_code = NRF_SUCCESS, chen;
	uint8_t  softdevice_enabled;

	// Initialize Programmable Peripheral Interconnect
	int chan_0 = findFreePPIChannel(255);
	int chan_1 = findFreePPIChannel(chan_0);
	
	if ((chan_0 != 255) && (chan_1 != 255))
	{	
		err_code = sd_softdevice_is_enabled(&softdevice_enabled);
		APP_ERROR_CHECK(err_code);
		if (softdevice_enabled == 0)
		{	
			// Enable PPI using registers
			NRF_PPI->CH[chan_0].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[CC_channel]) );
			NRF_PPI->CH[chan_0].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
			NRF_PPI->CHEN |= ( 1 << chan_0);
			// Save PPI channel number
			PPI_Channels_Occupied[gpiote_channel][0] = chan_0;
			
			// Configure PPI channel "chan_1" to toggle "ulPin" pin on every Timer COMPARE[3] match
			NRF_PPI->CH[chan_1].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[3]) );
			NRF_PPI->CH[chan_1].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
			NRF_PPI->CHEN |= ( 1 << chan_1);
			// Save PPI channel number
			PPI_Channels_Occupied[gpiote_channel][1] = chan_1;
			//simple_uart_printHEX(NRF_PPI->CHEN);
						
		}
		else
		{	
			//Enable PPI using sd_ppi_x
			err_code = sd_ppi_channel_assign(chan_0, &((*Timer).EVENTS_COMPARE[CC_channel]), &NRF_GPIOTE->TASKS_OUT[gpiote_channel]);
			APP_ERROR_CHECK(err_code);
			err_code = sd_ppi_channel_enable_set(1 << chan_0);
			APP_ERROR_CHECK(err_code);
			//Save PPI channel number
			PPI_Channels_Occupied[gpiote_channel][0] = chan_0;
			
			err_code = sd_ppi_channel_assign(chan_1, &((*Timer).EVENTS_COMPARE[3]), &NRF_GPIOTE->TASKS_OUT[gpiote_channel]);
			APP_ERROR_CHECK(err_code);
			err_code = sd_ppi_channel_enable_set(1 << chan_1 );	
			APP_ERROR_CHECK(err_code);
			//Save PPI channel number
			PPI_Channels_Occupied[gpiote_channel][1] = chan_1;	
			
			err_code = sd_ppi_channel_enable_get(&chen);
			APP_ERROR_CHECK(err_code);
		}

	}

	/*
	if(0 == gpiote_channel)
	{
		NRF_PPI->CH[9].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[CC_channel]) );
		NRF_PPI->CH[9].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
		
		NRF_PPI->CHEN |= ( 1 << 9);
		// Save PPI channel number
		PPI_Channels_Occupied[gpiote_channel][0] = 9;

		NRF_PPI->CH[10].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[3]) );
		NRF_PPI->CH[10].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
		NRF_PPI->CHEN |= ( 1 << 10);
		// Save PPI channel number
		PPI_Channels_Occupied[gpiote_channel][1] = 10;		
	}
	else if(1 == gpiote_channel)
	{
		NRF_PPI->CH[11].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[CC_channel]) );
		NRF_PPI->CH[11].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
		NRF_PPI->CHEN |= ( 1 << 11);
		// Save PPI channel number
		PPI_Channels_Occupied[gpiote_channel][0] = 11;

		NRF_PPI->CH[12].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[3]) );
		NRF_PPI->CH[12].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
		NRF_PPI->CHEN |= ( 1 << 12);
		// Save PPI channel number
		PPI_Channels_Occupied[gpiote_channel][1] = 12;		
	}
	else if(2 == gpiote_channel)
	{
		NRF_PPI->CH[13].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[CC_channel]) );
		NRF_PPI->CH[13].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
		NRF_PPI->CHEN |= ( 1 << 13);
		// Save PPI channel number
		PPI_Channels_Occupied[gpiote_channel][0] = 13;

		NRF_PPI->CH[14].EEP = (uint32_t)( &((*Timer).EVENTS_COMPARE[3]) );
		NRF_PPI->CH[14].TEP = (uint32_t)( &NRF_GPIOTE->TASKS_OUT[gpiote_channel] );
		NRF_PPI->CHEN |= ( 1 << 14);
		// Save PPI channel number
		PPI_Channels_Occupied[gpiote_channel][1] = 14;		
	}
	*/
}

/**********************************************************************
name :
function : 
**********************************************************************/
//void TIMER1_IRQHandler(void)  
static void TIMER1_handler( void )
{	
	uint8_t index;
	// Update the CCx values
	NRF_TIMER1->CC[0] = PWM_Channels_Value[0];
	NRF_TIMER1->CC[1] = PWM_Channels_Value[1];
	NRF_TIMER1->CC[2] = PWM_Channels_Value[2];
	NRF_TIMER1->CC[3] = 0;
	
	if (PWM_Channels_Start[0] == 1)
	{
		NRF_TIMER1->EVENTS_COMPARE[0] = 0;
		nrf_gpiote_task_config(GPIOTE_Channel_for_Analog[0], Timer1_Occupied_Pin[0], NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
		PWM_Channels_Start[0] = 0;
	}
	if (PWM_Channels_Start[1] == 1)
	{
		NRF_TIMER1->EVENTS_COMPARE[1] = 0;
		nrf_gpiote_task_config(GPIOTE_Channel_for_Analog[1], Timer1_Occupied_Pin[1], NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
		PWM_Channels_Start[1] = 0;
	}
	if (PWM_Channels_Start[2] == 1)
	{
		NRF_TIMER1->EVENTS_COMPARE[2] = 0;
		nrf_gpiote_task_config(GPIOTE_Channel_for_Analog[2], Timer1_Occupied_Pin[2], NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
		PWM_Channels_Start[2] = 0;
	}
	//when IRQHandle, make sure all GPIO is LOW. If HIGH,reconfigure it.
	for(index=0; index<3; index++)
	{
		if(Timer1_Occupied_Pin[index] != 255)
		{
			if( (NRF_GPIO->IN >> Timer1_Occupied_Pin[index]) & 1UL )
			{
				nrf_gpiote_task_config(GPIOTE_Channel_for_Analog[index], Timer1_Occupied_Pin[index], NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
			}
		}
	}
	
	NRF_TIMER1->EVENTS_COMPARE[3] = 0;
}

/**********************************************************************
name :
function : 
**********************************************************************/
void analogWrite(uint32_t ulPin, uint32_t ulValue)
{	
	uint32_t nrf_pin, max_value;
	uint8_t gpiote_channel;

	nrf_pin = arduinoToVariantPin(ulPin);
	if( nrf_pin < 31)
	{	//if vaule 0 or >255, set LOW or HIGH
		if(ulValue <= 0)
		{  
			ppiOffFromGPIO(nrf_pin);
			NRF_GPIO->OUTCLR = (1 << nrf_pin);
			return;
		}
		max_value = (uint32_t)( pow(2, analogWriteResolution_bit) - 1 );
		if(ulValue >= max_value )
		{	
			ulValue = max_value - 1;
		}
		//if exist,  update the value
		if (Timer1_Occupied_Pin[0] == nrf_pin)
		{	
			update_PWM_value(nrf_pin, ulValue, 0);
		}
		else if (Timer1_Occupied_Pin[1] == nrf_pin)
		{
			update_PWM_value(nrf_pin, ulValue, 1);
		}
		else if (Timer1_Occupied_Pin[2] == nrf_pin)
		{
			update_PWM_value(nrf_pin, ulValue, 2);
		}
		else
		{  
			if ((Timer1_Occupied_Pin[0] == 255) && (Timer1_Occupied_Pin[1] == 255) && (Timer1_Occupied_Pin[2] == 255))
			{	
				// Configure ulPin as output
				NRF_GPIO->PIN_CNF[nrf_pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
						| (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
						| (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
						| (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
						| (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
				NRF_GPIO->OUTCLR = (1 << nrf_pin);	
				
				//fine a free gpiote channel
				gpiote_channel = gpioteChannelFind();
				if( gpiote_channel == UNAVAILABLE_GPIOTE_CHANNEL )
				{
					return;
				}
				//configure TIMER1
				NRF_TIMER1->TASKS_STOP = 1;
				NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
				NRF_TIMER1->PRESCALER = 0; // Source clock frequency is divided by 2^6 = 64 				
				//NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_08Bit;
				NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit;	
				// Clears the timer, sets it to 0
				NRF_TIMER1->TASKS_CLEAR = 1;
				NRF_TIMER1->CC[0] = ((2^PWM_RESOLUTION) - 1);
				NRF_TIMER1->CC[1] = ((2^PWM_RESOLUTION) - 1);
				NRF_TIMER1->CC[2] = ((2^PWM_RESOLUTION) - 1);
				NRF_TIMER1->CC[3] = 0;
				NRF_TIMER1->EVENTS_COMPARE[0] = 0;
				NRF_TIMER1->EVENTS_COMPARE[1] = 0;
				NRF_TIMER1->EVENTS_COMPARE[2] = 0;
				NRF_TIMER1->EVENTS_COMPARE[3] = 0;

				//Interrupt setup
				NRF_TIMER1->INTENSET = (TIMER_INTENSET_COMPARE3_Enabled << TIMER_INTENSET_COMPARE3_Pos);
				IntController_linkInterrupt( TIMER1_IRQn, TIMER1_handler );
				//can't set low priority, else the GPIO polarity will change 
				//NVIC_SetPriority(TIMER1_IRQn, 3);  
				//NVIC_ClearPendingIRQ(TIMER1_IRQn);
				NVIC_EnableIRQ(TIMER1_IRQn); 
				NRF_TIMER1->TASKS_START = 1;				
				// PPI for TIMER1 and IO TASK
				nrf_gpiote_task_config(gpiote_channel, nrf_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
				gpioteChannelSet(gpiote_channel);
				PPI_ON_TIMER_GPIO(gpiote_channel, NRF_TIMER1, 0);
				//Save pin , channel and value
				GPIOTE_Channel_for_Analog[0] = gpiote_channel;
				PWM_Channels_Value[0] = ((2^PWM_RESOLUTION) - 1) - conversion_Resolution(ulValue, analogWriteResolution_bit, PWM_RESOLUTION);
				Timer1_Occupied_Pin[0] = nrf_pin;
			}
			else
			{
				if (Timer1_Occupied_Pin[0] == 255)
				{
					NRF_GPIO->PIN_CNF[nrf_pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
												| (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
												| (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
												| (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
												| (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
					NRF_GPIO->OUTCLR = (1 << nrf_pin);
					//fine a free gpiote channel and configure the channel
					gpiote_channel = gpioteChannelFind();
					if( gpiote_channel == UNAVAILABLE_GPIOTE_CHANNEL )
					{
						return;
					}
					nrf_gpiote_task_config(gpiote_channel, nrf_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
					gpioteChannelSet(gpiote_channel);
					PPI_ON_TIMER_GPIO(gpiote_channel, NRF_TIMER1, 0);
					//save the pin and value
					GPIOTE_Channel_for_Analog[0] = gpiote_channel;
					PWM_Channels_Value[0] = ((2^PWM_RESOLUTION) - 1) - conversion_Resolution(ulValue, analogWriteResolution_bit, PWM_RESOLUTION);
					Timer1_Occupied_Pin[0] = nrf_pin;
					NRF_TIMER1->EVENTS_COMPARE[0] = 0;
				}
				else if (Timer1_Occupied_Pin[1] == 255)
				{
					NRF_GPIO->PIN_CNF[nrf_pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
												| (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
												| (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
												| (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
												| (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
					NRF_GPIO->OUTCLR = (1 << nrf_pin);
					//find a free gpiote channel
					gpiote_channel = gpioteChannelFind();
					if( gpiote_channel == UNAVAILABLE_GPIOTE_CHANNEL )
					{
						return;
					}
					
					nrf_gpiote_task_config(gpiote_channel, nrf_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
					gpioteChannelSet(gpiote_channel);
					PPI_ON_TIMER_GPIO(gpiote_channel, NRF_TIMER1, 1);
					//save the pin and value
					GPIOTE_Channel_for_Analog[1] = gpiote_channel;
					PWM_Channels_Value[1] = ((2^PWM_RESOLUTION) - 1) - conversion_Resolution(ulValue, analogWriteResolution_bit, PWM_RESOLUTION);
					Timer1_Occupied_Pin[1] = nrf_pin;
					NRF_TIMER1->EVENTS_COMPARE[1] = 0;
				}
				else if (Timer1_Occupied_Pin[2] == 255)
				{
					NRF_GPIO->PIN_CNF[nrf_pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
												| (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
												| (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
												| (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
												| (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
					NRF_GPIO->OUTCLR = (1 << nrf_pin);
					//find a free gpiote channel
					gpiote_channel = gpioteChannelFind();
					if( gpiote_channel == UNAVAILABLE_GPIOTE_CHANNEL )
					{
						return;
					}
					
					nrf_gpiote_task_config(gpiote_channel, nrf_pin, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
					gpioteChannelSet(gpiote_channel);
					PPI_ON_TIMER_GPIO(gpiote_channel, NRF_TIMER1, 2);
					//save the pin and value
					GPIOTE_Channel_for_Analog[2] = gpiote_channel;
					PWM_Channels_Value[2] = ((2^PWM_RESOLUTION) - 1) - conversion_Resolution(ulValue, analogWriteResolution_bit, PWM_RESOLUTION);
					Timer1_Occupied_Pin[2] = nrf_pin;
					NRF_TIMER1->EVENTS_COMPARE[2] = 0;
				}
				else
				{   
					//no more
				}
			}
		}	
	}
}

static void app_adc_evt_get(void * p_event_data, uint16_t event_size)
{
	app_adc_conversion_event_t * app_adc_event = (app_adc_conversion_event_t *)p_event_data;

    APP_ERROR_CHECK_BOOL(event_size == sizeof(app_adc_conversion_event_t));
    onADCReadCb = NULL;
    app_adc_event->adc_handler(&app_adc_event->u32_adcValue);
}

/**@brief Function for handling the ADC interrupt.
 * @details  This function will fetch the conversion result from the ADC
 */
void ADC_IRQHandler(void)
{
	app_adc_conversion_event_t app_adc_event;

    if (NRF_ADC->EVENTS_END != 0)
    {
        APP_ERROR_CHECK_BOOL(onADCReadCb != NULL);
        app_adc_event.adc_handler = onADCReadCb;
        NRF_ADC->EVENTS_END     = 0;
        app_adc_event.u32_adcValue =  conversion_Resolution(NRF_ADC->RESULT, ADC_RESOLUTION, analogReadResolution_bit);
        NRF_ADC->TASKS_STOP     = 1;
        NRF_ADC->INTENCLR = ADC_INTENCLR_END_Msk;

        app_sched_event_put(&app_adc_event, sizeof(app_adc_event), app_adc_evt_get);
    }
}
