/******************************************************************************
 * @file    flash_memory.cpp 
 * @author  Rémi Pincent - INRIA
 * @date    17 sept. 2015   
 *
 * @brief TODO_one_line_description_of_file
 * 
 * Project : nrf51_template_application
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 * 
 * Revision History:
 * TODO_revision history
 *****************************************************************************/

#include <flash_memory.h>
#include "application_config.h"
extern "C" {
#include "app_error.h"
#include "nrf_soc.h"
#include "app_scheduler.h"
}

FlashMemory FlashMem;

/** refer linker script */
/** end rom */
extern uint32_t __etext;


// divide the address by 1024
#define  PAGE_FROM_ADDRESS(address)  ((uint8_t)((uint32_t)address >> 10))

// multiple the page by 1024
#define  ADDRESS_OF_PAGE(page)  ((uint32_t*)(page << 10))

FlashMemory::FlashMemory(void) : _b_pendingOperation(false), _s8_writeStatus(NO_ERROR)
{
}

int8_t FlashMemory::readLong(int address, int32_t& value)
{
	value = 0;
	if((uint32_t*) address <= &__etext)
	{
		/** writing in application code */
		return INVALID_ADDRESS;
	}
	value = (*(uint32_t*)address);
	return sizeof(value);
}

int8_t FlashMemory::read(int address, int8_t& value)
{
	if((uint32_t*) address <= &__etext)
	{
		/** writing in application code */
		return INVALID_ADDRESS;
	}
	value = (*(uint32_t*)address) & 0xFF;
	return sizeof(value);
}

/**
 * @param address
 * @param value
 * @return
 */
int8_t FlashMemory::write(int address, uint8_t value)
{
	uint32_t err_code = NRF_SUCCESS;

	if((uint32_t*) address <= &__etext)
	{
		/** writing in application code */
		return INVALID_ADDRESS;
	}

	if(_b_pendingOperation)
	{
		return BUSY;
	}

	_b_pendingOperation = true;
	err_code = sd_flash_write((uint32_t*)address, (uint32_t*)&value, 1);
	APP_ERROR_CHECK(err_code);
	while(_b_pendingOperation)
	{
	    uint32_t err_code = sd_app_evt_wait();
	    APP_ERROR_CHECK(err_code);
	    if(USE_EVENT_SCHEDULER)
	    {
	    	/** Must be called - otherwise sysEvtDispatch() won't be called */
	    	app_sched_execute();
	    }
	}
	return (_s8_writeStatus == NO_ERROR) ? sizeof(value) : WRITE_ERROR;
}

FlashMemory::EError FlashMemory::erasePage(int pageNumber)
{
	uint32_t err_code = NRF_SUCCESS;

	if(_b_pendingOperation)
	{
		return BUSY;
	}

	_b_pendingOperation = true;
	err_code = sd_flash_page_erase(pageNumber);
	APP_ERROR_CHECK(err_code);
	while(_b_pendingOperation)
	{
	    uint32_t err_code = sd_app_evt_wait();
	    APP_ERROR_CHECK(err_code);
	    if(USE_EVENT_SCHEDULER)
	    {
	    	/** Must be called - otherwise sysEvtDispatch() won't be called */
	    	app_sched_execute();
	    }
	    APP_ERROR_CHECK(err_code);
	}
	return (_s8_writeStatus == NO_ERROR) ? NO_ERROR : WRITE_ERROR;
}
int8_t FlashMemory::writeLong(int address, int32_t value)
{
	uint32_t err_code = NRF_SUCCESS;

	if((uint32_t*) address <= &__etext)
	{
		/** writing in application code */
		return INVALID_ADDRESS;
	}

	if(_b_pendingOperation)
	{
		return BUSY;
	}

	_b_pendingOperation = true;
	err_code = sd_flash_write((uint32_t*)address, (uint32_t*)&value, 1);
	APP_ERROR_CHECK(err_code);
	while(_b_pendingOperation)
	{
	    uint32_t err_code = sd_app_evt_wait();
	    APP_ERROR_CHECK(err_code);
	    if(USE_EVENT_SCHEDULER)
	    {
	    	/** Must be called - otherwise sysEvtDispatch() won't be called */
	    	app_sched_execute();
	    }
	}
	return (_s8_writeStatus == NO_ERROR) ? sizeof(value) : WRITE_ERROR;
}

/**
 * Flash handler
 */
void FlashMemory::flash_handler(uint32_t sys_evt)
{
    switch (sys_evt)
     {
         case NRF_EVT_FLASH_OPERATION_SUCCESS:
        	 FlashMem._s8_writeStatus = NO_ERROR;
		 FlashMem._b_pendingOperation = false;
        	 break;

         case NRF_EVT_FLASH_OPERATION_ERROR:
        	 FlashMem._s8_writeStatus = WRITE_ERROR;
		 FlashMem._b_pendingOperation = false;
             break;

         default:
             // No implementation needed.
             break;

     }
}

/**
 * Mapping between Soft device flash handler and FlashMemory handler
 * @param sys_evt
 */
void sd_flash_handler(uint32_t sys_evt)
{
	FlashMem.flash_handler(sys_evt);
}


