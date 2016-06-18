/******************************************************************************
 * @file    appication_main.cpp
 * @author  Rémi Pincent - INRIA
 * @date    14 janv. 2015   
 *
 * @brief Teleinfo ble main
 * 
 * Project : teleinfo_ble
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 * 
 * Revision History:
 * TODO_revision history
 *****************************************************************************/

/**************************************************************************
 * Include Files
 **************************************************************************/
#include "Arduino.h"
#include "logger.h"
#include "nrf51_status.h"
#include "assert.h"
#include "memory_watcher.h"
#include "pinout.h"
#include "ble_teleinfo.h"

/**************************************************************************
 * Manifest Constants
 **************************************************************************/
static const char* as8_bleName = "teleinfo";
static const unsigned int LOOP_PERIOD_MS = 200;

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**************************************************************************
 * Type Definitions
 **************************************************************************/

/**************************************************************************
 * Variables
 **************************************************************************/
BLETransceiver bleTransceiver;
BleTeleinfo bleTeleinfo(bleTransceiver);
/**************************************************************************
 * Macros
 **************************************************************************/

/**************************************************************************
 * Global Functions
 **************************************************************************/

void application_setup(void){
	Serial.begin(1200);

	/** Transceiver must be initialized before other application peripherals */
	bleTransceiver.init(as8_bleName);
	LOG_INIT_STREAM(LOG_LEVEL, &Serial);
	LOG_INFO_LN("\nStarting application ...");
	LOG_INFO_LN("min remaining stack = %l", MemoryWatcher::getMinRemainingStack());
	LOG_INFO_LN("min remaining heap = %l", MemoryWatcher::getMinRemainingHeap());
	LOG_INFO_LN("remaining stack = %l", MemoryWatcher::getRemainingStack());
	MemoryWatcher::paintStackNow();

	delay(200);
	/** Use event manager - instantiate it now */
	EventManager::getInstance();

	int loc_error = bleTransceiver.advertise();
	if(loc_error != BLETransceiver::NO_ERROR){
		LOG_ERROR("Error %d when launching advertisement", loc_error);
	}

	bleTeleinfo.start();

	LOG_INFO_LN("Setup finished");
}


/**
 * Called in application context
 */
void application_loop(void){
  MemoryWatcher::checkRAMHistory();
  MemoryWatcher::paintStackNow();
  EventManager::applicationTick(LOOP_PERIOD_MS);
}
