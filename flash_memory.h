/******************************************************************************
 * @file    flash_memory.h 
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
#ifndef FLASH_MEMORY_H_
#define FLASH_MEMORY_H_

#include <stdint.h>

/**
 * @class FlashMemory
 * @brief TODO_one_line_description_of_class.
 *
 * TODO_longer_description_of_class_meant_for_users._
 * 
 */
class FlashMemory
{
public:
	typedef int8_t EError;
	static const EError NO_ERROR = 0;
	static const EError BUSY = NO_ERROR -1;
	static const EError INVALID_ADDRESS = BUSY + 1 ;
	static const EError WRITE_ERROR = INVALID_ADDRESS - 1;
	static const EError ERASE_ERROR = WRITE_ERROR - 1;
public:
	FlashMemory(void);
	int8_t readLong(int, int32_t&);
	int8_t read(int, int8_t&);
	int8_t write(int, uint8_t);
	int8_t writeLong(int, int32_t);
	EError erasePage(int);
	void flash_handler(uint32_t sys_evt);
private:
	bool _b_pendingOperation;
	int8_t _s8_writeStatus;
};

extern FlashMemory FlashMem;

#endif /* FLASH_MEMORY_H_ */
