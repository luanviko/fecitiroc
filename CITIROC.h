#ifndef CITIROC_H
#define CITIROC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string>
#include "ftd2xx.h"
#include "LALUsb.h"
#include "odbxx.h"

// Byte -> 8 bits -> unsigned char.
typedef unsigned char byte;

// To be used both here and at fecitiroc.cxx

const char odbdir_DAQ[1024]  = "/Equipment/Citiroc1A_DAQ";
const char odbdir_HV[1024]   = "/Equipment/Citiroc1A_HV";
const char odbdir_temp[1024] = "/Equipment/Citiroc1A_Slow/Temperature";
const char odbdir_asic_addresses[1024] = "/Equipment/Citiroc1A_Slow/ASIC_addresses";
const char odbdir_asic_values[1024] = "/Equipment/Citiroc1A_Slow/ASIC_values";
const char odbdir_asic_sizes[1024] = "/Equipment/Citiroc1A_Slow/ASIC_sizes";

// Parameter names at ODB directories
const char odb_temp_enable  = "Enable temperature sensor";
const char odb_temp_configa = "Temp.-sensor configuration a";
const char odb_temp_configb = "Temp.-sensor configuration b";
const char odb_txsize = "FIFO write size";
const char odb_rxsize = "FIFO read size";

// Public methods/ functions
bool CITIROC_connect(char* CITIROC_serialNumber, int* CITIROC_usbID);
bool CITIROC_initialize(const int CITIROC_usbID);
bool CITIROC_reset(const int CITIROC_usbID);
bool CITIROC_disconnet(const int CITIROC_usbID);
bool CITIROC_sendWord(const int CITIROC_usbID, const char subAddress, const byte* binary);
bool CITIROC_readWord(const int CITIROC_usbID, const char subAddress, byte* word, const int wordCount);
bool CITIROC_enableDAQ(const int CITIROC_usbID);
bool CITIROC_disableDAQ(const int CITIROC_usbID);
bool CITIROC_testParameters(const int CITIROC_usbID);
bool CITIROC_readFIFO(const int CITIROC_usbID, byte* fifo20, byte*fifo21, byte* fifo23, byte* fifo24, int* wordCount);
bool CITIROC_raiseException();
bool CITIROC_sendASIC();
bool CITIROC_convertToBits(int n, const int numberOfBits, int* binary);

#endif 