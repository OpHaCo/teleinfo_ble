

#ifndef _WIRING_DIGITAL_
#define _WIRING_DIGITAL_

#include "wiring_constants.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void pinMode( uint32_t ulPin, EPinMode arg_e_pinMode );
 
extern void digitalWrite( uint32_t ulPin, uint32_t ulVal );

extern int digitalRead( uint32_t ulPin );

#ifdef __cplusplus
}
#endif

#endif /* _WIRING_DIGITAL_ */
