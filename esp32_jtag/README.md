# FT2232HL adapter used as esp-prog JTAG adapter 

* Software Platform : Ubuntu 20.04, Visual Studio Code with Platformio extension
* Hardware Platform : Any ESP32 dev board with GPIO12 .. GPIO15 free for JTAG interface

## ESP32 Dev board to FT2232HL USB adapter connections

```
ESP32        FT2232HL USB adapter
                   
GND                           GND  
Vin                           VCC(5V)
             JTAG (channel A)       Serial (channel B)
GPIO13       AD0 (TCK) purple
GPIO12       AD1 (TDI) yellow
GPIO15       AD2 (TDO) orange
GPIO14       AD3 (TMS) brown
TXD                                 BD1 (RXD)
RXD                                 BD0 (TXD)		        
```	

<img src="esp32_ft2232hl_jtag.jpg">

## Integration into Visual Studio Code with Platformio on Ubuntu 20.04		    

* Download and install [99-platformio-udev.rules](https://docs.platformio.org/en/latest/plus/debug-tools/esp-prog.html)
```
sudo cp 99-platformio-udev.rules /etc/udev/rules.d/99-platformio-udev.rules
sudo service udev restart
```

Now you should only see one ttyUSB port when you plug in the FT2232HL adapter (for channel B serial port).

### Add esp-prog debug and upload to platformio.ini in your project directory

Example

```
[env:esp32_dev_module]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_port = /dev/ttyUSB*
monitor_port = /dev/ttyUSB*
debug_tool = esp-prog
upload_protocol = esp-prog
```

### Upload 

* Click on the Platformio icon (bug-eyed alien)
* In the Platformio sidebar go to Project Tasks -> Upload
* Alternatively, using cli in terminal window
```
cd <project dir>
pio home
pio run --target upload
```

### Debug

* Click on the Platformio icon
* In the Platformio sidebar go to Quick Access -> Debug -> Start Debugging

<img src="esp32_ft2232hl_debug.jpg">



