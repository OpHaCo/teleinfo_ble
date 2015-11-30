

#ifndef _WIRING_ANALOG_
#define _WIRING_ANALOG_

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_RESOLUTION        16

#define WRITE_CURRENT_RESOLUTION    8
#define READ_CURRENT_RESOLUTION		10

typedef void (*on_adc_conversion_handler_t)(uint32_t*);
typedef struct
{
	on_adc_conversion_handler_t adc_handler;
	uint32_t u32_adcValue;
} app_adc_conversion_event_t;

extern void analogWrite( uint32_t ulPin, uint32_t ulValue );

/**
 * Synchronous read
 * @param ulPin
 * @return
 */
extern uint32_t analogRead( uint32_t ulPin );

/**
 * Asynchronous read
 * @param ulPin
 * @param arg_adcCb
 * @return true if conversion successfully started, false otherwise
 */
extern bool analogAsyncRead( uint32_t ulPin,  on_adc_conversion_handler_t arg_adcCb);

extern void analogReference( uint32_t type );
extern void analogInpselType( uint32_t type);
//extern void analogExtReference( uint32_t type );
extern void analogReadResolution( uint8_t resolution);
extern void analogWriteResolution( uint8_t resolution );

#ifdef __cplusplus
}
#endif

#endif 
