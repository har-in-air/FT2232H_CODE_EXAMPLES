#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<windows.h>
#include "ftd2xx.h"
#include "libMPSSE_spi.h"


#define APP_CHECK_STATUS(exp) {if(exp!=FT_OK){printf("%s:%d:%s(): status(0x%x) \
!= FT_OK\n",__FILE__, __LINE__, __FUNCTION__,exp);exit(1);}else{;}};
#define CHECK_NULL(exp){if(exp==NULL){printf("%s:%d:%s():  NULL expression \
encountered \n",__FILE__, __LINE__, __FUNCTION__);exit(1);}else{;}};

static FT_HANDLE ftHandle;


static FT_STATUS write_bytes( uint8* pData, int numBytes){
	uint32 sizeTransfered=0;
	FT_STATUS status;

	status = SPI_Write(ftHandle, pData, numBytes, &sizeTransfered,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	APP_CHECK_STATUS(status);
	return status;
	}


static FT_STATUS read_bytes(uint8 *pData, int numBytes){
	uint32 sizeTransferred;
	FT_STATUS status;

	sizeTransferred = 0;
	status = SPI_Read(ftHandle, pData, numBytes, &sizeTransferred,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|
		SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
	APP_CHECK_STATUS(status);

	return status;
	}


static FT_STATUS write_read(uint8 outBuffer, uint8* pRead, int numBytes ){
	uint32 sizeToTransfer = 0;
	uint32 sizeTransferred = 0;
	FT_STATUS status;

	sizeToTransfer = 1;
	sizeTransferred = 0;
	
	// for some reason, enabling and disabling CS outside the SPI_Write and SPI_Read commands reduces the 
	// the CS active time from ~16ms to ~3mS
	
	SPI_ToggleCS(ftHandle, 1); // CSn enabled
	status = SPI_Write(ftHandle, &outBuffer, sizeToTransfer, &sizeTransferred, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);
	APP_CHECK_STATUS(status);
	
	sizeToTransfer = numBytes;
	sizeTransferred = 0;
	status = SPI_Read(ftHandle, pRead, sizeToTransfer, &sizeTransferred, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);
	SPI_ToggleCS(ftHandle, 0); // CSn disabled
		
	APP_CHECK_STATUS(status);

	return status;
	}


int main(){
	FT_STATUS status = FT_OK;
	FT_DEVICE_LIST_INFO_NODE devList = {0};
	ChannelConfig channelConf = {0};
	uint32 channels = 0;
	uint8 latency = 25;	
	
	channelConf.ClockRate = 500000;
	channelConf.LatencyTimer = latency;
	channelConf.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS3 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
	channelConf.Pin = 0x080B080B;//FinalVal-FinalDir-InitVal-InitDir (dir 0=in, 1=out)

	Init_libMPSSE();
	
//	status = SPI_GetNumChannels(&channels);
//	APP_CHECK_STATUS(status);
//	printf("Number of available SPI channels = %d\n",(int)channels);

	// Open  channel 0 (interface A)
	status = SPI_OpenChannel(0,&ftHandle);
	APP_CHECK_STATUS(status);
//	printf("\nhandle=0x%x status=0x%x\n",(unsigned int)ftHandle,status);
	status = SPI_InitChannel(ftHandle,&channelConf);
	APP_CHECK_STATUS(status);

		
	uint8 outBuffer[8] = {0};
	uint8 inBuffer[8] = {0};
	int nRead, nWrite;
	
#if 0	
	// read JEDEC ID
	// write 0x9F, read 0x40EF15
	outBuffer[0] = 0x9F;
	nRead = 3;
	write_read(outBuffer[0], inBuffer, nRead);
#endif

#if 0	
	// read unique 64bit chip id
	// write 0x4B,0x00,0x00,0x00,0x00
	// read unique 8bytes for each chip
	outBuffer[0] = 0x4B;
	nWrite = 5;
	write_bytes(outBuffer, nWrite);	
	nRead = 8;
	read_bytes(inBuffer, nRead);
#endif

#if 1
	// read manufacturer id
	// write 0x90,0x00, 0x00, 0x00
	// read 0xEF14
	 outBuffer[0] = 0x90;
	 nWrite = 4;
	 write_bytes(outBuffer, nWrite);
	 nRead = 2;
	 read_bytes(inBuffer, nRead);
#endif
	
	for(int inx = 0; inx < nRead; inx++){
		printf("0x%02X\n", inBuffer[inx]);
		}
	printf("\n");
	
	
	status = SPI_CloseChannel(ftHandle);
	
	Cleanup_libMPSSE();
	return 0;
	}

