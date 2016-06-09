#include "Arduino.h"
#include "app_timer.h"

// widen rtc1 to 40-bit (1099511627775 ticks = 33554431999969us = 388 days)
// (dont overflow uint64_t when multipying by 1000000)
extern uint64_t rtc1_overflow_count;

/**********************************************************************
name :
function :
**********************************************************************/
uint64_t millis64( void )
{	
	//divide by 32768
	uint32_t time_;
	app_timer_cnt_get( &time_ );
	return (rtc1_overflow_count + time_) * 1000 >> 15;  
}
/**********************************************************************
name :
function : 
**********************************************************************/
uint64_t micros64( void )
{
	//accurate to 30.517us, divide by 32768
	uint32_t time_;
	app_timer_cnt_get( &time_ );
	return (rtc1_overflow_count + time_) * 1000000 >> 15;  
}
/**********************************************************************
name :
function : 
**********************************************************************/
uint32_t millis( void )
{
	return millis64();
}
/**********************************************************************
name :
function : 
**********************************************************************/
uint32_t micros( void )
{
	return micros64();
}
/**********************************************************************
name :
function : 
**********************************************************************/
void delay( uint32_t ms )
{
	uint32_t start = millis();
	while (millis() - start < ms)
	{
	    yield();
	}
}
/**********************************************************************
name :
function : 
**********************************************************************/
void delayMicroseconds( uint32_t us )
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

#ifdef __cplusplus
}
#endif
