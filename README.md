# nrf51_node

## general

nrf51 node - connects over bluetooth using UART bluetooth service implemented in https://github.com/Lahorde/nrf51_template_application


## Prerequisities
Linux machine

### Bluetooth setup
   - needs a Bluetooth 4.0 dongle
   - bluez installed with bt 4.0 support - www.bluez.org - On debian based distributions :
   
        sudo apt-get install bluetooth bluez bluez-utils blueman libbluetooth-dev
	    sudo apt-get install libusb-dev libdbus-1-dev libglib2.0-dev
	    
	    # check bluetooth installed
        bluetoothd -v
        
   - to check your setup - with BT4.0 dongle plugged :

        # get local devices and identify your usb dongle
        hcitool dev 
        # Domi cube advertisements messages should be received - its name is SensorTag
        sudo hcitool lescan -i your_bt4.0_dongle 

### Nodejs
Get a nodejs recent version >= 0.10.2 On debian based distributions : 

    sudo apt-get install nodejs
    sudo apt-get install npm
    npm config set registry http://registry.npmjs.org/

For rpi, nodejs version too old, for newer version install refer : https://learn.adafruit.com/raspberry-pi-hosting-node-red/setting-up-node-dot-js

## Test 
    
## nrf51_node  methods/events description

### usage

__constructor__

    NRF51Node(peripheral);

__discover__

If an uuid table is given, only nodes with theses uuids will be discovered

    NRF51Node.Discover(callback, uuids);

__stop discover__

    NRF51Node.stopDiscover(callback);

__reconnect__

    NRF51Node.reconnect(callback);

__restore already discovered chars and reenable notifications__

    NRF51Node.restoreCharsAndNotifs(callback);

__connect__

    NRF51Node.connect(callback);
    
__disconnect__

    NRF51Node.disconnect(callback);

__discover services__

    NRF51Node.discoverServicesAndCharacteristics(callback);

__read devices infos__

    NRF51Node.readDeviceName(callback);
    NRF51Node.readAppearance(callback);
    NRF51Node.readPreferredConnParams(callback);
    NRF51Node.readDeviceName(callback);

__receive data__
	NRF51Node.notifyDataReceive(callback);
	NRF51Node.unnotifyDataReceive(callback);

__write data__

    NRF51Node.writeData(data, callback);

__emitted events__
	- 'disconnect'
	- 'connect'
    - 'connectionDrop'
    - 'reconnect'
    - 'dataReceived'

## references 

 https://epx.com.br/artigos/bluetooth_gatt.php