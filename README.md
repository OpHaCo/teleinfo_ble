## logging

### Overview

Arduino logging library, supports a variable list of arguments and its format specifiers. 
Log level can be modified according to user needs. 
Using preprocessor logging macros defined in logger.h reduces code size depending on selected logger level.

#### Implementation supported platform

  * Arduino Uno - ATmega328p
  * Jeenode µ - ATtiny 84

####Log levels
 * LOG_LEVEL_NOOUTPUT 
 * LOG_LEVEL_ERRORS 1
 * LOG_LEVEL_INFOS 2
 * LOG_LEVEL_DEBUG 3
 * LOG_LEVEL_VERBOSE 4

#### Code snippet
Log using a software serial stream.

    #include <logger.h>
    #include <AltSoftSerial.h>
    
    #define LOG_LEVEL LOG_LEVEL_DEBUG
    
    static AltSoftSerial altSerial;

    void setup()
    {
        Serial.begin(115200);
        altSerial.begin(9600);
        
        LOG_INIT(LOG_LEVEL, &altSerial);
	    LOG_INFO(F("application started ! \n"));
    }
