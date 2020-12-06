#include "user.h"
#include "fatfs.h"
#include "stdio.h"

void writeSdCardDemo()
{
	FATFS fileSystem;
	FIL logFile;
	uint8_t testBuffer[16] = "SD write success";
	UINT testBytes;
	FRESULT res;

	if(f_mount(&fileSystem, USERPath, 1) == FR_OK)
	{
	  uint8_t path[13] = "testfile.txt";
	  path[12] = '\0';
	  res = f_open(&logFile, (char*)path, FA_WRITE | FA_CREATE_ALWAYS); printf("res: %i\n", res);
	  res = f_write(&logFile, testBuffer, 16, &testBytes); printf("res: %i\n", res);
	  res = f_close(&logFile); printf("res: %i\n", res);
	}
}

uint32_t blinkCounter = 0;
uint8_t ledState = 0;

void blinkUserLed(const uint32_t target)
{
	if (target == ++blinkCounter)
	{
	  if(0 == ledState)
		  tickLedOn();
	  else
		  tickLedOn();
	  ledState = (1 == ledState) ? 0 : 1;
	  blinkCounter = 0;
	}
}

void tickLedOn()
{

}

void tickLedOff()
{

}

