/******************************************************************************
 * @file    AltSoftSerial.h
 * @author  Rémi Pincent - INRIA
 * @date    24 nov. 2015   
 *
 * @brief Bit-Banging uart
 * 
 * Project : AltSoftSerial
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 * 
 * Originally from
 * 
 * LICENSE :
 * AltSoftSerial (c) by Rémi Pincent
 * AltSoftSerial is licensed under a
 * Creative Commons Attribution-NonCommercial 3.0 Unported License.
 *
 * You should have received a copy of the license along with this
 * work.  If not, see <http://creativecommons.org/licenses/by-nc/3.0/>.
 *
 * ORIGINALLY FROM :
 * An Alternative Software Serial Library
 * http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
 * Copyright (c) 2014 PJRC.COM, LLC, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *****************************************************************************/

#include "AltSoftSerial.h"
#include "pinout.h"
extern "C"{
#include "nrf_gpio.h"
#include "app_error.h"
}

/****************************************/
/**          Defines                   **/
/****************************************/
#define UART_STOP_BIT_POS       8
#define UART_STOP_BIT_ACTIVE    1

/****************************************/
/**          Variables                 **/
/****************************************/
AltSoftSerial AltSoftSerial::SoftSerial = AltSoftSerial();

/****************************************/
/**          Initialization            **/
/****************************************/

AltSoftSerial::AltSoftSerial() : _u8_txPin(INVALID_PIN),
		_u16_ticks_per_bit(0),
		_u32_mics_per_byte(0),
		_b_uartBusy(false),
		timing_error(false),
		_u8_sendByte(0),
		_txBuffer(TX_BUFFER_SIZE),
		_bit_pos(0)
{

}

void AltSoftSerial::startTimer0(void)
{
	/** Use TIMER0 using timeslot API as it is also used by soft_device */
	NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;               // Set the timer in Timer Mode.
	NRF_TIMER0->PRESCALER = 0;                              // Prescaler 0 produces F_CPU/2^prescaler timer frequency => 1 tick = 32 us.
	NRF_TIMER0->BITMODE     = TIMER_BITMODE_BITMODE_16Bit;  // 16 bit mode
	NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;   // Short to lear IT
	NRF_TIMER0->TASKS_CLEAR = 1;                            // clear the task first to be usable for later.
	NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;     // Enable IT
	NRF_TIMER0->CC[0] = _u16_ticks_per_bit;
	NRF_TIMER0->EVENTS_COMPARE[0] = 0;
	NVIC_EnableIRQ(TIMER0_IRQn);
	NRF_TIMER0->TASKS_START = 1;
}

void AltSoftSerial::stopTimer(void)
{
	NRF_TIMER2->TASKS_STOP = 1;
	NRF_TIMER2->INTENCLR = TIMER_INTENSET_COMPARE0_Disabled << TIMER_INTENCLR_COMPARE0_Pos;
}

void AltSoftSerial::begin(uint32_t baud, uint8_t txPin)
{
	_u8_txPin = arduinoToVariantPin(txPin);
	_u16_ticks_per_bit = F_CPU / baud;
	/** add 200µs for processing */
	_u32_mics_per_byte = (1000000/baud) * (8 + 2) + 200;

	register_timer0_timeslot_IRQ(AltSoftSerial::txTimer0IRQ);
	register_timeslot_start_cb(AltSoftSerial::prepare);
	assert(timeslot_sd_init() == NRF_SUCCESS);

	pinMode(_u8_txPin, OUTPUT);
	digitalWrite(txPin, HIGH);
}

void AltSoftSerial::end(void)
{
	flushOutput();
	stopTimer();
}


/****************************************/
/**           Transmission             **/
/****************************************/

bool AltSoftSerial::writeByte(uint8_t b)
{
	uint32_t loc_u32_err = NRF_SUCCESS;
	if(_txBuffer.pushElement(b) != RingBuffer<uint8_t>::NO_ERROR)
	{
		/** May be buffer full... */
		return false;
	}

    if (!_b_uartBusy)
    {
    	/** Request a timeslot for bit banging using with  timer 0 ISR */
    	loc_u32_err = request_next_event_normal(SoftSerial._u32_mics_per_byte);
    	if(loc_u32_err == NRF_SUCCESS)
    	{
    		SoftSerial._b_uartBusy = true;
    	}
    	else
    	{
    		SoftSerial._b_uartBusy = false;
    	}
    }
    else
    {
    	/** UART busy but byte added to RING buffer */
    }
    return true;
}


void AltSoftSerial::prepare(TsNextAction* arg_p_nextAction){
	if(SoftSerial._txBuffer.elementsAvailable())
	{
	    if(SoftSerial._txBuffer.popElement(SoftSerial._u8_sendByte) != RingBuffer<uint8_t>::NO_ERROR)
	    {
	    	/** nothing to do - cancel timeslot */
	    	arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_END;
	    	SoftSerial._b_uartBusy = false;
	    	return;
	    }

	   /** send a byte in timeslot */
	    nrf_gpio_pin_clear(SoftSerial._u8_txPin);
	    SoftSerial.startTimer0();
	    arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
	}
	else
	{
    	/** nothing to do - cancel timeslot */
		arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_END;
    	SoftSerial._b_uartBusy = false;
	}
}

void AltSoftSerial::flushOutput(void)
{
	/** commented => infinite loop, ok when commented - TODO */
	//while (_b_uartBusy) /* wait */;
}

/**
 * Bit-Bang ring buffer bytes. No parity, start and stop bits, 8 bits
 *
 *   ______________________________________________________________
 *  | start bit | d0 | d1 | d2 | d3 | d4 | d5 | d6 | d7 | stop bit |
 *  |___________|____|____|____|____|____|____|____|____|__________|
 *
 */
void AltSoftSerial::txTimer0IRQ(TsNextAction* arg_p_nextAction)
{
	if(NRF_TIMER0->EVENTS_COMPARE[0])
    {
        NRF_TIMER0->EVENTS_COMPARE[0] = 0;

        if (SoftSerial._bit_pos < UART_STOP_BIT_POS)
        {
        	/** set bit at bit_pos */
            (SoftSerial._u8_sendByte & (1 << SoftSerial._bit_pos)) ? nrf_gpio_pin_set(SoftSerial._u8_txPin) : nrf_gpio_pin_clear(SoftSerial._u8_txPin);
            SoftSerial._bit_pos++;
            arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
        }
        else if (SoftSerial._bit_pos == UART_STOP_BIT_POS)
        {
        	SoftSerial._bit_pos++;
            /** Set stop bit */
            nrf_gpio_pin_set(SoftSerial._u8_txPin);
            arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
        }
        // We have to wait until STOP bit is finished before releasing m_uart_busy.
        else if (SoftSerial._bit_pos == UART_STOP_BIT_POS + 1)
        {
        	SoftSerial._bit_pos = 0;
        	NRF_TIMER0->TASKS_STOP = 1;

            if(SoftSerial._txBuffer.elementsAvailable()){
            	/** Some elements to pop : either extend current timeslot */
            	arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND;
            	arg_p_nextAction->extension.slot_length = SoftSerial._u32_mics_per_byte;

            }
            else
            {
            	arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_END;
            	SoftSerial._b_uartBusy = false;
            }
        }
        else
        {
        	assert(false);
        }
    }
	else
	{
		arg_p_nextAction->callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
	}
}

