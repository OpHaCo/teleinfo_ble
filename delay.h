

#ifndef _WIRING_
#define _WIRING_

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t millis( void );
extern uint32_t micros( void );
extern void delay( uint32_t ms ) ;
extern void delayMicroseconds( uint32_t us );

#ifdef __cplusplus
}
#endif

#endif 
