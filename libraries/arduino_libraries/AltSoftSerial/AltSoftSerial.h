/******************************************************************************
 * @file    AltSoftSerial.h 
 * @author  Rémi Pincent - INRIA
 * @date    24 nov. 2015   
 *
 * @brief Bit-Banging uart - Tx buffered
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

#ifndef AltSoftSerial_h
#define AltSoftSerial_h

#include <inttypes.h>
#include <ring_buffer.h>
extern "C"
{
#include <timeslot.h>
}
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "pins_arduino.h"
#endif

class AltSoftSerial : public Stream
{
private :
	static const uint8_t TX_BUFFER_SIZE = 80;

	uint8_t _u8_txPin;
	uint16_t _u16_ticks_per_bit;
	uint32_t _u32_mics_per_byte;
	bool _b_uartBusy;
	bool timing_error;
	uint8_t _u8_sendByte;
	RingBuffer<uint8_t> _txBuffer;
	int8_t _bit_pos;

public:
	/** instance */
	static AltSoftSerial SoftSerial;

public:
	AltSoftSerial();
	 virtual ~AltSoftSerial() { end(); }
	void begin(uint32_t baud, uint8_t txPin);
	void end();
	/** RX not implemented */
	int peek(){return 0;};
	/** RX not implemented */
	int read(){return 0;};
	/** RX not implemented */
	int available(){return 0;};
#if ARDUINO >= 100
	size_t write(uint8_t byte) { writeByte(byte); return 1; }
	void flush() { flushOutput(); }
#else
	void write(uint8_t byte) { writeByte(byte); }
	void flush() { flushInput(); }
#endif
	using Print::write;
	/** RX not implemented */
	void flushInput(){return;};
	void flushOutput();
	// for drop-in compatibility with NewSoftSerial, rxPin & txPin ignored
	AltSoftSerial(uint8_t rxPin, uint8_t txPin, bool inverse = false) : AltSoftSerial() { }
	bool listen() { return false; }
	bool isListening() { return true; }
	bool overflow() { bool r = timing_error; timing_error = false; return r; }
	static int library_version() { return 1; }

	static void txTimer0IRQ(TsNextAction* arg_p_nextAction);
	static void prepare(TsNextAction*);

private:
	void initTimer(void);
	void stopTimer(void);
	bool writeByte(uint8_t byte);
	void startTimer0();
};

#endif
