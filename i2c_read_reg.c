/*
  I2C send using libftdi and FT2232 chip connected to USB.
  Note that this utility will open the first FT2232 chip found.
 
  Written by Ori Idan, Helicon technologies LTD. (ori@helicontech.co.il)
 
  i2csend is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  i2csend is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program.  If not, see <http://www.gnu.org/licenses/>.
 
  ***************  Modified by HN *********************
 */



// FT2232H
#define VENDOR_ID		0x403
#define PRODUCT_ID		0x6010

// sudo apt_get install libftdi-dev 
// gcc -o i2c_read_reg i2c_read_reg.c -lftdi
// sudo ./i2c_read_reg

// uses interface A on FT2232H 
// SCL = AD0, SDA = AD1,AD2 
// connect AD1, AD2 pins together for SDA bidirectional function
// AD2 is always input (high-Z)
// AD1 is either driving output, or input (high-Z)

#include <stdio.h>
#include <ctype.h>
#include <ftdi.h>


const uint8_t MSB_RISING_EDGE_CLOCK_BYTE_IN = MPSSE_DO_READ;
const uint8_t MSB_FALLING_EDGE_CLOCK_BYTE_OUT = MPSSE_DO_WRITE | MPSSE_WRITE_NEG;
const uint8_t MSB_RISING_EDGE_CLOCK_BIT_IN = 0x22;


uint8_t OBuf[256]; // write buffer for MPSSE commands and data written to slave device
uint8_t IBuf[256];  // read buffer for data
unsigned int clkDiv = 0x004A; // 0x4A = 74, SCL Frequency = 60/((1+74)*2) MHz = 400khz
int nbWr = 0; // output buffer index
int nbSent = 0; // number bytes sent
int nbRd = 0; // number bytes read

int debug = 1;	// Debug mode

struct ftdi_context* ftdi;

int ftdi_config(void);
void i2c_config(void);
void i2c_start(void);
void i2c_stop(void);
int i2c_wrByte(uint8_t wrByte);
uint8_t i2c_rdByte(void);



int main(int argc, char *argv[]) {

	if (ftdi_config() != 0) return -1;
	
	//////////////////// read device id register from BMP280 /////////////
	
	i2c_config();

	i2c_start();
	
	uint8_t bmp280SlaveAddr = 0xEE; // 8bit slave address
	uint8_t devIDReg = 0xD0;
	
	// write register address to i2c slave device
	int res = i2c_wrByte(bmp280SlaveAddr);
	res = i2c_wrByte(devIDReg);

	// repeated start
	i2c_start();
	
	// read register
	res = i2c_wrByte(bmp280SlaveAddr | 0x01); 
	uint8_t devID = i2c_rdByte();
	
	printf("BMP280 ID = 0x%02X\r\n", devID);
	
	i2c_stop();
	
	//////////////////////////////////////////////////////////////////////


	ftdi_usb_close(ftdi);
    ftdi_deinit(ftdi);
	return 0;
	}


 // Open FT2232 device and get valid handle for subsequent access.
 // Note that this function initialize the ftdi struct used by other functions.
 
int ftdi_config(void) {
	int cnt;
	int bCommandEchoed;
	
	ftdi = ftdi_new();
	if(ftdi == NULL) {
		printf("ftdi_new() failed\r\n");
		return 1;
		}
	ftdi_set_interface(ftdi, INTERFACE_A);
	
	int ftStatus = ftdi_usb_open(ftdi, VENDOR_ID, PRODUCT_ID);
	if(ftStatus < 0) {
		printf("Error opening usb device: %s\r\n", ftdi_get_error_string(ftdi));
		return 1;
		}
	
	// Port opened successfully
	if(debug)
		printf("Port opened, resetting device...\r\n");
	
	ftStatus |= ftdi_usb_reset(ftdi); 			// Reset USB device
	ftStatus |= ftdi_usb_purge_rx_buffer(ftdi);	// purge rx buffer
	ftStatus |= ftdi_usb_purge_tx_buffer(ftdi);	// purge tx buffer
	
	// Set MPSSE mode
	ftdi_set_bitmode(ftdi, 0xFF, BITMODE_RESET);
	ftdi_set_bitmode(ftdi, 0xFF, BITMODE_MPSSE);
	
	
	 // Below code will synchronize the MPSSE interface by sending bad command 0xAA
	 // response should be echo command followed by bad command 0xAA.
	 // This will make sure the MPSSE interface enabled and synchronized successfully
	 
	nbWr = 0;
	OBuf[nbWr++] = 0xAA; 	// Add BAD command 0xAA
	nbSent = ftdi_write_data(ftdi, OBuf, nbWr);

	int inx = 0;
	do {
		nbRd = ftdi_read_data(ftdi, IBuf, 2);
		if(nbRd < 0) {
			if(debug)
				fprintf(stderr,"Error: %s\r\n", ftdi_get_error_string(ftdi));
			break;
			}
		if(debug)
			printf("Got %d bytes %02X %02X\r\n", nbRd, IBuf[0], IBuf[1]);
		if(++inx > 5)	// up to 5 times read 
			break;
		} while (nbRd == 0);
	
	// Check if echo command and bad received
	for (cnt = 0; cnt < nbRd; cnt++) {
		if ((IBuf[cnt] == 0xFA) && (IBuf[cnt+1] == 0xAA)) {
			if(debug)
				printf("FTDI synchronized\r\n");
			bCommandEchoed = 1;
			break;
			}
		}
	if (bCommandEchoed == 0) {
		fprintf(stderr, "Error, cant receive echo command , fail to synchronize MPSSE interface\r\n");
		return 1;
		}
   return 0;
   }
   
   
   
void i2c_config(void) {
	nbWr = 0;
	OBuf[nbWr++] = DIS_DIV_5; // Disable clock divide by 5 =>  60Mhz master clock
	OBuf[nbWr++] = DIS_ADAPTIVE;// Disable adaptive clocking
	OBuf[nbWr++] = EN_3_PHASE;// Enable 3 phase data clock,allow data on both clock edges
	nbSent = ftdi_write_data(ftdi, OBuf, nbWr);	
	
	nbWr = 0;
	OBuf[nbWr++] = SET_BITS_LOW; 
	OBuf[nbWr++] = 0x03 ; // Set SDA, SCL high
	OBuf[nbWr++] = 0x03; // Set SCL, SDA as outputs
	// set SCL clock frequency 
	OBuf[nbWr++] = TCK_DIVISOR; // Command to set clock divisor
	OBuf[nbWr++] = clkDiv & 0xFF; //Set clock divisor low byte
	OBuf[nbWr++] = (clkDiv >> 8) & 0xFF;	// Set clock divisor high byte
	nbSent = ftdi_write_data(ftdi, OBuf, nbWr);
	
	nbWr = 0; 
	OBuf[nbWr++] = LOOPBACK_END; // Turn off loop back of TDI/TDO
	nbSent = ftdi_write_data(ftdi, OBuf, nbWr);
	nbWr = 0; 
	}
	

// Generate I2C start : SDA hi->lo transition while SCL = hi
// Set SDA and SCL high.
// Set SDA low (while SCL remains high)
// Set SCL low
 
void i2c_start(void) {
	int cnt;

    // first ensure both SCL and SDA are hi
	for(cnt = 0; cnt < 10; cnt++)  {		
		OBuf[nbWr++] = SET_BITS_LOW; 
		//Set SDA, SCL high
		OBuf[nbWr++] = 0x03; 
		//Set SCL, SDA pins as output 
		OBuf[nbWr++] = 0x03;
		}
	
	// transition SDA to lo while SDA is high
	for(cnt = 0; cnt < 10; cnt++) {
		OBuf[nbWr++] = SET_BITS_LOW; 
		//Set SDA low, SCL high
		OBuf[nbWr++] = 0x01; 
		//Set SCL, SDA pins as output 
		OBuf[nbWr++] = 0x03; 
		}
		
	// set SCL also low 	
	OBuf[nbWr++] = SET_BITS_LOW;
	//Set SDA, SCL low
	OBuf[nbWr++] = 0x00; 
	//Set SCL, SDA pins as output
	OBuf[nbWr++] = 0x03; 
	}


// Generate I2C stop : SDA lo->hi transition while SCL = hi
// Set SDA low, SCL high.
// Set both pins to input high-Z so they are pulled up to hi

void i2c_stop(void) {
	int cnt;

	// first ensure both SCL and SDA are low
	for(cnt = 0; cnt < 10; cnt++) {
		OBuf[nbWr++] = SET_BITS_LOW;
		//Set SDA low, SCL low
		OBuf[nbWr++] = 0x00; 
		//Set SCL, SDA pins as output
		OBuf[nbWr++] = 0x03; 
		}		
		
	// set SCL high 	
	for(cnt = 0; cnt < 10; cnt++) {
		OBuf[nbWr++] = SET_BITS_LOW;
		//Set SDA low, SCL high
		OBuf[nbWr++] = 0x01; 
		//Set SCL, SDA pins as output
		OBuf[nbWr++] = 0x03; 
		}		

	// set the SCL, SDA pins as input (high Z)
	// this allows SDA to be pulled high, generating the stop condition
	OBuf[nbWr++] = SET_BITS_LOW; 
	OBuf[nbWr++] = 0x03; 
	OBuf[nbWr++] = 0x00; 
	
	ftdi_write_data(ftdi, OBuf, nbWr);
	nbWr = 0;	
	}



int i2c_wrByte(uint8_t wrByte) {
	OBuf[nbWr++] = SET_BITS_LOW; 
	// Set SDA high, SCL low
	OBuf[nbWr++] = 0x02; 
	//Set SCL, SDA pins as output
	OBuf[nbWr++] = 0x03; 
	
	// Clock data byte out on –ve Clock Edge MSB first
	OBuf[nbWr++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; 
	OBuf[nbWr++] = 0x00;
	OBuf[nbWr++] = 0x00; //Data length of 0x0000 means 1 byte data to clock out
	OBuf[nbWr++] = wrByte; //Add data to be send
	
	// Get Acknowledge bit
	OBuf[nbWr++] = SET_BITS_LOW; 
	OBuf[nbWr++] = 0x02; // Set SCL low
	//Set SCL pin as output, SDA as input floating
	OBuf[nbWr++] = 0x01;
	//Command to read ACK bit , +ve clock Edge MSB first
	OBuf[nbWr++] = MSB_RISING_EDGE_CLOCK_BIT_IN;
	OBuf[nbWr++] = 0x00;  //Length of 0x0 means to scan in 1 bit
	OBuf[nbWr++] = SEND_IMMEDIATE; //Send answer back immediate command
	nbSent = ftdi_write_data(ftdi, OBuf, nbWr);
	int res;
	// Read one byte from device receive buffer
	nbRd = ftdi_read_data(ftdi, IBuf, 1);
	if (nbRd == 0) {
		fprintf(stderr, "error reading bit\r\n");
		return 0; 
		}
	else if(!(IBuf[0] & 0x01)) {
		// Check ACK bit  on data byte read out
		res = 1;
		}
	else 	res = 0;
	
	if(debug)
		printf("Received: %d, %02X\r\n", nbRd, IBuf[0]);

	nbWr = 0;
	OBuf[nbWr++] = SET_BITS_LOW; 
	// Set SDA high, SCL low
	OBuf[nbWr++] = 0x02; 
	//Set SCL as output, SDA floating
	OBuf[nbWr++] = 0x01; 
	return res;
	}




// read address must be already written to slave device

uint8_t i2c_rdByte(void) {
	OBuf[nbWr++] = SET_BITS_LOW; 
	// Set SCL low, SDA high
	OBuf[nbWr++] = 0x02; 
	// Set SCL pin as output, SDA pin as input (high Z)
	OBuf[nbWr++] = 0x01; 
	// Command to clock data byte in on +ve Clock Edge MSB first
	OBuf[nbWr++] = MSB_RISING_EDGE_CLOCK_BYTE_IN; 
	OBuf[nbWr++] = 0x00; // read one byte
	OBuf[nbWr++] = 0x00;
	// command to read acknowledge bit
	OBuf[nbWr++] = MSB_RISING_EDGE_CLOCK_BIT_IN; 
	OBuf[nbWr++] = 0x0;  // Length of 0 means to scan in 1 bit
	OBuf[nbWr++] = SEND_IMMEDIATE; // Send answer back immediate command
	nbSent = ftdi_write_data(ftdi, OBuf, nbWr); // Send commands
	nbWr = 0;	
	// Read two bytes from device receive buffer, first byte is data read, second byte is ACK bit
	nbRd = ftdi_read_data(ftdi, IBuf, 2);
	if(nbRd < 2) {
		printf("Error reading i2c\r\n");
		return 0xFF;
		}
	if(debug) {
		printf("Data read: %02X\r\n", IBuf[0]);	
		}
	return IBuf[0];
	}


#if 0
// Read I2C bytes.
// Note that read address must be sent beforehand
 
void i2c_rdBytes(uint8_t * readBuffer, unsigned int readLength) {
    unsigned int clock = 60 * 1000/(1+clkDiv)/2; // K Hz
    const int loopCount = (int)(10 * ((float)200/clock));
    unsigned int readCount = 0;
    int i = 0;  // Used only for loop
    if (!readBuffer || !readLength) {
        return;
    	}
    
    while(readCount != (readLength -1))    {
        // Command of read one byte
        OBuf[nbWr++] = SET_BITS_LOW; 
        OBuf[nbWr++] = 0x02; //Set SCL low
        OBuf[nbWr++] = 0x01; //Set SCL pin as output, SDA pin as input (high Z)
        OBuf[nbWr++] = MSB_RISING_EDGE_CLOCK_BYTE_IN; //Command to clock data byte in on –ve Clock Edge MSB first
        OBuf[nbWr++] = 0x00; // read one byte
        OBuf[nbWr++] = 0x00;
        
        // Set ACK
        for (i=0; i != loopCount; ++i) {
            OBuf[nbWr++] = SET_BITS_LOW;
            OBuf[nbWr++] = 0x00;  // SDA and SCL Low
            OBuf[nbWr++] = 0x03;
        	}

        for (i=0; i != loopCount; ++i)  {
            OBuf[nbWr++] = SET_BITS_LOW;
            OBuf[nbWr++] = 0x01;  // SDA Low, SCL High
            OBuf[nbWr++] = 0x03;
        	}

        for (i=0; i != loopCount; ++i)  {
            OBuf[nbWr++] = SET_BITS_LOW;
            OBuf[nbWr++] = 0x02;  // SDA High, SCL Low
            OBuf[nbWr++] = 0x03;
        	}
        ftdi_write_data(ftdi, OBuf, nbWr);
        nbWr = 0;
        ++readCount;
    	}
    
    // The last byte is read with NO ACK.
    // Command of read one byte
    OBuf[nbWr++] = SET_BITS_LOW; //Command to set directions of lower 8 pins and force value on bits set as output
    OBuf[nbWr++] = 0x00; //Set SCL low
    OBuf[nbWr++] = 0x01; //Set SCL pin as output
    OBuf[nbWr++] = MSB_RISING_EDGE_CLOCK_BYTE_IN; //Command to clock data byte in on –ve Clock Edge MSB first
    OBuf[nbWr++] = 0x00;
    OBuf[nbWr++] = 0x00; //Data length of 0x0000 means 1 byte data to clock in
    
    // Set NO ACK
    for (i=0; i != loopCount; ++i)    {
        OBuf[nbWr++] = SET_BITS_LOW;
        OBuf[nbWr++] = 0x02; // SDA High SCL Low
        OBuf[nbWr++] = 0x03;
    	}

    for (i=0; i != loopCount; ++i) {
        OBuf[nbWr++] = SET_BITS_LOW;
        OBuf[nbWr++] = 0x03; // SDA High, SCL High
        OBuf[nbWr++] = 0x03;
    	}

    for (i=0; i != loopCount; ++i)    {
        OBuf[nbWr++] = SET_BITS_LOW;
        OBuf[nbWr++] = 0x02; // SDA High, SCL Low
        OBuf[nbWr++] = 0x03;
    	}
    ftdi_write_data(ftdi, OBuf, nbWr);
    nbWr = 0;
        
    // Read bytes from device receive buffer, first byte is data read, second byte is ACK bit
	nbRd = ftdi_read_data(ftdi, readBuffer, readLength);
    
    if(nbRd != readLength) {
		printf("Error reading i2c\n");
		return;
		}
    
    if(debug) {
        for(i=0; i != readLength; ++i) {
            printf("Data read: %02X\n", readBuffer[i]);
        	}
    	}	
    return;
	}
#endif



