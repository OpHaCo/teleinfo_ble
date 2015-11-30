# Description
Bit-banging UART with Tx ring buffer.

# Supported platforms
* Nordic nrf51 

## Prerequisities
* Arduino Support => here is a template nrf51 with arduino support https://github.com/Lahorde/nrf51_template_application

## Limitations
* only tx Supported

# Code sample

    AltSoftSerial::SoftSerial.begin(9600, TX_PIN);
    AltSoftSerial::SoftSerial.println("Hello from AltSoftSerial);
    
# References
https://github.com/PaulStoffregen/AltSoftSerial
https://github.com/Lahorde/nrf51_template_application
https://devzone.nordicsemi.com/tutorials/16/