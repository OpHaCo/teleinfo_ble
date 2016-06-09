
/*
  Copyright (c) 2011 Arduino.  All right reserved.

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
*/

/**
  * \file syscalls_sam3.c
  *
  * Implementation of newlib syscall.
  *
  */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/


#include "syscalls.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include "wuart.h"
#if defined (  __GNUC__  ) /* GCC CS3 */
  #include <sys/types.h>
  #include <sys/stat.h>
#endif
#include "nrf51.h"

/*----------------------------------------------------------------------------
 *        Exported variables
 *----------------------------------------------------------------------------*/

#undef errno
extern int errno ;
extern int  _end ;

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/
extern void _exit( int status ) ;
extern void _kill( int pid, int sig ) ;
extern int _getpid ( void ) ;

extern caddr_t _sbrk ( int incr )
{
	/** RP 18/11/2014 - check heap https://devzone.nordicsemi.com/question/14581/nrf51822-with-gcc-stacksize-and-heapsize/ */
    extern uint32_t __HeapBase;
    extern uint32_t __HeapLimit;
    static char* heap = 0;
    if (heap == 0) heap = (char*)&__HeapBase;
    void* ret = heap;
    if (heap + incr >= (char*)&__HeapLimit)
    {
        ret = (void*)-1;
        /** be sure to never be here... - if here : check dynamic allocations and eventually
         * increase heap size*/
        assert(false);
    }
    else
        heap += incr;
    return ret;
}

extern int link( char *cOld, char *cNew )
{
    return -1 ;
}

extern int _close( int file )
{
    return -1 ;
}

extern int _fstat( int file, struct stat *st )
{
    st->st_mode = S_IFCHR ;

    return 0 ;
}

extern int _isatty( int file )
{
    return 1 ;
}

extern int _lseek( int file, int ptr, int dir )
{
    return 0 ;
}

extern int _read(int file, char *ptr, int len)
{
    return 0 ;
}

extern int _write( int file, char *ptr, int len )
{
    int i;
    for (i = 0; i < len; i++, ptr++)
	{
        //UART0_TX(*ptr);
		if( UART0_State == UART0_AfterFirstTX )
		{
			while( !NRF_UART0->EVENTS_TXDRDY);
		}
		
		NRF_UART0->EVENTS_TXDRDY = 0;
		NRF_UART0->TXD = *ptr;
		
		if (UART0_State == UART0_BeforeFirstTX)
		{
			UART0_State = UART0_AfterFirstTX;
		}
	}
    return i;
}

extern void _exit( int status )
{
    printf( "Exiting with status %d.\n", status ) ;

    for ( ; ; ) ;
}

extern void _kill( int pid, int sig )
{
    return ;
}

extern int _getpid ( void )
{
    return -1 ;
}
