#ifndef CITIROC_H
#define CITIROC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include "ftd2xx.h"
#include "LALUsb.h"
#include "odbxx.h"

#define CITIROC_DEBUG_FLAG true

// Byte -> 8 bits -> unsigned char.
typedef unsigned char byte;

// To be used both here and at fecitiroc.cxx

const char odbdir_DAQ[1024]  = "/Equipment/Citiroc1A_DAQ";
const char odbdir_HV[1024]   = "/Equipment/Citiroc1A_HV";
const char odbdir_temp[1024] = "/Equipment/Citiroc1A_Slow/Temperature";
const char odbdir_asic_addresses[1024] = "/Equipment/Citiroc1A_Slow/ASIC_addresses";
const char odbdir_asic_values[1024] = "/Equipment/Citiroc1A_Slow/ASIC_values";
const char odbdir_asic_sizes[1024] = "/Equipment/Citiroc1A_Slow/ASIC_sizes";
const char odbdir_firmware[1024] = "/Equipment/Citiroc1A_Slow/Firmware";
const char database_firmware[1024] = "/Equipment/Citiroc1A_Slow/Slow_control";

// Parameter names at ODB directories
const char odb_temp_enable  = "Enable temperature sensor";
const char odb_temp_configa = "Temp.-sensor configuration a";
const char odb_temp_configb = "Temp.-sensor configuration b";
const char odb_txsize = "FIFO write size";
const char odb_rxsize = "FIFO read size";

// Public methods/ functions
int  CITIROC_connect(char* CITIROC_serialNumber);
bool CITIROC_initialize(const int CITIROC_usbID);
bool CITIROC_reset(const int CITIROC_usbID);
bool CITIROC_disconnet(const int CITIROC_usbID);
bool CITIROC_sendWord(const int CITIROC_usbID, const char subAddress, const char* bitArray);
bool CITIROC_sendWords(const int CITIROC_usbID, const char subAddress, char* asicString, const int wordCount);
bool CITIROC_sendASIC(const int CITIROC_usbID);
bool CITIROC_writeASIC(const int CITIROC_usbID, std::vector<int> asicVector, const int numberOfWords);
bool CITIROC_convertToBits(int n, const int numberOfBits, int* binary);
bool CITIROC_readWord(const int CITIROC_usbID, const char subAddress, char* word, const int wordCount);
bool CITIROC_readString(const int CITIROC_usbID, const char subAddress, std::string* wordString);
int  CITIROC_readFIFO(const int CITIROC_usbID, int* dataLG, int* dataHG, int* totalHits, int run_number);
bool CITIROC_readFIFO_fixedAcqNumber(const int CITIROC_usbID, char* fifoHG, char* fifoLG);
bool CITIROC_printWord(char subAddress, char word, int wordCount);
bool CITIROC_readFPGASubAddress(const int usbId, const char subAddress);
bool CITIROC_sendFirmwareSettings(const int CITIROC_usbId);
void CITIROC_raiseException();
#endif 