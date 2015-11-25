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
		_b_uartBusy(false),
		timing_error(false),
		_u8_sendByte(0),
		_txBuffer(TX_BUFFER_SIZE)
{

}

void AltSoftSerial::initTimer(void)
{
	/** Use TIMER 2 - Also used by Arduino Tone object */
	/** MUST not use TIMER0 : used by soft_device */
	NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;               // Set the timer in Timer Mode.
	NRF_TIMER2->PRESCALER = 0;                              // Prescaler 0 produces F_CPU/2^prescaler timer frequency => 1 tick = 32 us.
	NRF_TIMER2->BITMODE     = TIMER_BITMODE_BITMODE_16Bit;  // 16 bit mode
	NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;   // Short to lear IT
	NRF_TIMER2->TASKS_CLEAR = 1;                            // clear the task first to be usable for later.
	NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Msk;     // Enable IT
	NRF_TIMER2->CC[0] = _u16_ticks_per_bit;

	IntController_linkInterrupt( TIMER2_IRQn, AltSoftSerial::txTimerIRQ);

    if(!IntController_enableIRQ(TIMER2_IRQn, NRF_APP_PRIORITY_LOW))
    {
    	APP_ERROR_CHECK_BOOL(false);
    }
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

	initTimer();
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
	if(_txBuffer.pushElement(b) != RingBuffer<uint8_t>::NO_ERROR)
	{
		/** May be buffer full... */
		return false;
	}

    if (!_b_uartBusy)
    {
    	_b_uartBusy = true;
    	_txBuffer.popElement(SoftSerial._u8_sendByte);

        // We set start bit active (low)
    	digitalWrite(_u8_txPin, LOW);
        NRF_TIMER2->TASKS_START = 1;
    }
    else
    {
    	/** UART busy but byte added to RING buffer */
    }
    return true;
}


void AltSoftSerial::flushOutput(void)
{
	while (_b_uartBusy) /* wait */ ;
}

/**
 * Bit-Bang ring buffer bytes. No parity, start and stop bits, 8 bits
 *
 *   ______________________________________________________________
 *  | start bit | d0 | d1 | d2 | d3 | d4 | d5 | d6 | d7 | stop bit |
 *  |___________|____|____|____|____|____|____|____|____|__________|
 *
 */
void AltSoftSerial::txTimerIRQ()
{
    if(NRF_TIMER2->EVENTS_COMPARE[0])
    {
        static uint8_t bit_pos;
        NRF_TIMER2->EVENTS_COMPARE[0] = 0;

        if (bit_pos < UART_STOP_BIT_POS)
        {
        	/** set bit at bit_pos */
            (SoftSerial._u8_sendByte & (1 << bit_pos)) ? digitalWrite(SoftSerial._u8_txPin, HIGH) : digitalWrite(SoftSerial._u8_txPin, LOW);;
            bit_pos++;
        }
        else if (bit_pos == UART_STOP_BIT_POS)
        {
            bit_pos++;
            /** Set stop bit */
            digitalWrite(SoftSerial._u8_txPin, HIGH);
        }
        // We have to wait until STOP bit is finished before releasing m_uart_busy.
        else if (bit_pos == UART_STOP_BIT_POS + 1)
        {
            bit_pos = 0;
            if(!SoftSerial._txBuffer.elementsAvailable())
            {
            	/** no more byte to write to UART */
            	NRF_TIMER2->TASKS_STOP = 1;
            	SoftSerial._b_uartBusy = false;
            	SoftSerial._u8_sendByte = 0;
            	return;
            }
            else
            {
            	/** still some bytes to write to UART */
            	SoftSerial._txBuffer.popElement(SoftSerial._u8_sendByte);
            	digitalWrite(SoftSerial._u8_txPin, LOW);
            }
        }
    }
}



