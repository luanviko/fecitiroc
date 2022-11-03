#ifndef CITIROC_H
#define CITIROC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "ftd2xx.h"
#include "LALUsb.h"
#include "odbxx.h"

// Byte -> 8 bits -> unsigned char.
typedef unsigned char byte;

// Useful bits for slow control
extern byte daqON = "10000000";
extern byte daqOFF = "00000000";

// To be used both here and at fecitiroc.cxx
extern const char odbdir_DAQ[1024]  = "/Equipment/Citiroc1A_DAQ";
extern const char odbdir_HV[1024]   = "/Equipment/Citiroc1A_HV";
extern const char odbdir_temp[1024] = "/Equipment/Citiroc1A_Slow/Temperature";

bool another_test(){return false;};

// Public methods/ functions
bool CITIROC_printInfo(char* CITIROC_serialNumber);
bool CITIROC_testParameters();
bool CITIROC_initializeBoard(int* CITIROC_usbId);
bool CITIROC_sendWord(const int CITIROC_usbID, const char subAddress, const byte* binary);
bool CITIROC_readWord(const int CITIROC_usbID, const char subAddress, byte* word, const int wordCount);
bool CITIROC_enableDAQ(const int CITIROC_usbID);
bool CITIROC_disableDAQ(const int CITIROC_usbID);

#endif 