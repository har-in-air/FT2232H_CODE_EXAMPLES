# FT2232H_CODE_EXAMPLES

<img src="FT2232HL_module.png">

Ubuntu 20.04 terminal applications using FT2232HL to communicate with spi and i2c peripherals.

Install libftdi with `sudo apt-get install libftdi-dev`.

Uses interface A (AD<n> bus) for SPI / I2C interface. Interface B is free to be used as serial UART.

## SPI Master interface

Example code `spiflash_read_id.c` reads the JEDEC ID information from an W25Q16 SPI serial flash by sending the 
command 0x9F and then reading 3 consecutive bytes. SPI clock configured for 1MHz.

`
AD0 : CLK, AD1 : MOSI, AD2 : MISO, AD3 : CSn
`

<img src="ft2232_spi_transaction_screenshot.png">


## I2C Master interface


Example code `i2c_read_reg.c` reads the device ID register at address 0xD0 from a BMP280 pressure sensor
configured in I2C mode with slave address 0XEE (8bit address). I2C clock configured for 400kHz.

<img src="i2c_read_register_transaction_snapshot.png">

Note : for I2C interface pins AD1 and AD2 need to be connected together for SDA bi-directional function.

`
AD0 : SCLK, AD1+AD2 : SDA
`

<img src="ft2232_mpsse_i2c_pins.png">


