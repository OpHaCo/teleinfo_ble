# Description
Bit-banging UART

# Supported platforms
* Nordic nrf51 

## Limitations
* only tx Supported
* bad bit banging when Radio active, e.g when advertising. Timer used for bit banging has lower priority than BLE.

# Code sample

    AltSoftSerial::SoftSerial.begin(9600, TX_PIN);
    AltSoftSerial::SoftSerial.println("Hello from AltSoftSerial);
    
# References
https://github.com/PaulStoffregen/AltSoftSerial