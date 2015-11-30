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

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include "Arduino.h"
#include "interrupt_controller.h"

typedef void (*ext_it_handler_t)(void*);

/**
 * Attach interrupt on given pin for given trigger. Interrupt handler will be called with arg_p_handlerPayload
 * @param arg_u32_pin
 * @param arg_pf_itHandler
 * @param arg_e_pinTrigger
 * @param arg_p_handlerPayload
 */
void attachInterrupt(uint32_t arg_u32_pin, ext_it_handler_t arg_pf_itHandler, EPinTrigger arg_e_pinTrigger, void* arg_p_handlerPayload = NULL);

/**
 * Detach all interrupts on given pin
 * @param arg_u32_pin
 */
void detachInterrupt(uint32_t arg_u32_pin );

#endif
