#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<windows.h>

#include "ftd2xx.h"
#include "libMPSSE_i2c.h"


#define APP_CHECK_STATUS(exp) {if(exp!=FT_OK){printf("%s:%d:%s(): status(0x%x) \
	!= FT_OK\n",__FILE__, __LINE__, __FUNCTION__,exp);exit(1);}else{;}};
#define APP_CHECK_STATUS_NOEXIT(exp) {if(exp!=FT_OK){printf("%s:%d:%s(): status(0x%x) \
	!= FT_OK\n",__FILE__, __LINE__, __FUNCTION__,exp);}else{;}};
#define CHECK_NULL(exp){if(exp==NULL){printf("%s:%d:%s():  NULL expression \
	encountered \n",__FILE__, __LINE__, __FUNCTION__);exit(1);}else{;}};

#define I2C_DEVICE_ADDRESS				0x77
#define I2C_DEVICE_BUFFER_SIZE 			16
#define I2C_COMPLETION_RETRIES			10

#define FAST_TRANSFER					1

static FT_HANDLE ftHandle;
static uint8 buffer[I2C_DEVICE_BUFFER_SIZE] = {0};
static uint32 timeWrite = 0;
static uint32 timeRead = 0;
static LARGE_INTEGER llTimeStart = {0};
static LARGE_INTEGER llTimeEnd = {0};
static LARGE_INTEGER llFrequency = {0};


static void init_time(){
	QueryPerformanceFrequency(&llFrequency);
	}

static void start_time(){
    QueryPerformanceCounter(&llTimeStart);
	}

static uint32 stop_time(){
	uint32 dwTemp = 0;
    QueryPerformanceCounter(&llTimeEnd);
    dwTemp = (uint32)((llTimeEnd.QuadPart - llTimeStart.QuadPart) * 1000 / (float)llFrequency.QuadPart);
    return dwTemp;
	}

static FT_STATUS write_bytes(uint8 slaveAddress, uint8 registerAddress, const uint8 *data, uint32 numBytes){
	FT_STATUS status;
	uint32 bytesToTransfer = 0;
	uint32 bytesTransfered = 0;
	uint32 options = 0;
	uint32 trials = 0;

#if FAST_TRANSFER
	options = I2C_TRANSFER_OPTIONS_START_BIT|I2C_TRANSFER_OPTIONS_STOP_BIT|I2C_TRANSFER_OPTIONS_FAST_TRANSFER_BYTES;
#else // FAST_TRANSFER
	options = I2C_TRANSFER_OPTIONS_START_BIT|I2C_TRANSFER_OPTIONS_STOP_BIT;
#endif // FAST_TRANSFER

	buffer[bytesToTransfer++] = registerAddress;
	memcpy(buffer + bytesToTransfer, data, numBytes);
	bytesToTransfer += numBytes;

	start_time();
	status = I2C_DeviceWrite(ftHandle, slaveAddress, bytesToTransfer, buffer, &bytesTransfered, options);
	timeWrite = stop_time();
	while (status != FT_OK && trials < I2C_COMPLETION_RETRIES)	{
		APP_CHECK_STATUS_NOEXIT(status);
		start_time();
		status = I2C_DeviceWrite(ftHandle, slaveAddress, bytesToTransfer, buffer, &bytesTransfered, options);
		timeWrite = stop_time();
		trials++;
		}
	printf("write_bytes write : trials %d\r\n", trials);

	return status;
	}

static FT_STATUS read_bytes(uint8 slaveAddress, uint8 registerAddress, uint8 *data, uint32 numBytes){
	FT_STATUS status = FT_OK;
	uint32 bytesToTransfer = 0;
	uint32 bytesTransfered = 0;
	uint32 options = 0;
	uint32 trials = 0;

#if FAST_TRANSFER
		options = I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_FAST_TRANSFER_BYTES;
#else // FAST_TRANSFER
		options = I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT;
#endif // FAST_TRANSFER
		buffer[bytesToTransfer++] = registerAddress;
		status = I2C_DeviceWrite(ftHandle, slaveAddress, bytesToTransfer, buffer, &bytesTransfered, options);
		trials = 0;
		while (status != FT_OK && trials < I2C_COMPLETION_RETRIES)		{
			APP_CHECK_STATUS_NOEXIT(status);
			status = I2C_DeviceWrite(ftHandle, slaveAddress, bytesToTransfer, buffer, &bytesTransfered, options);
			trials++;
			}
		printf("read_bytes write : trials %d\r\n", trials);
		if (status != FT_OK)		{
			return status;
			}
		//APP_CHECK_STATUS(status);
	

	bytesTransfered = 0;
	bytesToTransfer = numBytes;
#if FAST_TRANSFER
	options = I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE | I2C_TRANSFER_OPTIONS_FAST_TRANSFER_BYTES;
#else // FAST_TRANSFER
	options = I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE;
#endif // FAST_TRANSFER

	start_time();
	status |= I2C_DeviceRead(ftHandle, slaveAddress, bytesToTransfer, buffer, &bytesTransfered, options);
	timeRead = stop_time();

	trials = 0;
	while (status != FT_OK && trials < I2C_COMPLETION_RETRIES){
		APP_CHECK_STATUS_NOEXIT(status);
		start_time();
		status = I2C_DeviceRead(ftHandle, slaveAddress, bytesToTransfer, buffer, &bytesTransfered, options);
		timeRead = stop_time();
		trials++;
		}
	printf("read_bytes write : trials %d\r\n", trials);
	if (status == FT_OK){
		memcpy(data, buffer, bytesToTransfer);
		}
	return status;
	}





int main(){
	FT_STATUS status = FT_OK;
	ChannelConfig channelConf;
	uint32 channels;

	Init_libMPSSE();

	memset(&channelConf, 0, sizeof(channelConf));
	channelConf.ClockRate = I2C_CLOCK_FAST_MODE;
#if FAST_TRANSFER
	channelConf.LatencyTimer = 1;
#else 
	channelConf.LatencyTimer = 255;
#endif

	//status = I2C_GetNumChannels(&channels);
	//APP_CHECK_STATUS(status);
	//printf("Number of available I2C channels = %d\n",(int)channels);

	status = I2C_OpenChannel(0,&ftHandle);
	APP_CHECK_STATUS(status);
	//printf("\nhandle=0x%x status=%d\n",(unsigned int)ftHandle,(unsigned int)status);
	
	status = I2C_InitChannel(ftHandle,&channelConf);
	APP_CHECK_STATUS(status);

	init_time();

    uint8 id = 0;
	read_bytes(I2C_DEVICE_ADDRESS, 0xD0, &id, 1);

	printf("bmp280 device id should be 0x58, read = 0x%02X\r\n", id);

	status = I2C_CloseChannel(ftHandle);

	Cleanup_libMPSSE();
	return 0;
	}

