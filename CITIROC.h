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

// UsbID is equivalent to handle
int CITIROC_usbID;

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
bool CITIROC_connectBoard(char* CITIROC_serialNumber, int* usbId);
bool CITIROC_initializeBoard(int* CITIROC_usbId);
bool CITIROC_sendWord(const int CITIROC_usbID, const char subAddress, const byte* binary);
bool CITIROC_readWord(const int CITIROC_usbID, const char subAddress, byte* word, const int wordCount);
bool CITIROC_enableDAQ(const int CITIROC_usbID);
bool CITIROC_disableDAQ(const int CITIROC_usbID);
bool CITIROC_disconnetBoard(const int* CITIROC_usbID);

bool CITIROC_testParameters(){

     // Variables
    bool usbStatus;
    midas::odb daq_parameters(odbdir_DAQ);
    midas::odb temp_parameters(odbdir_temp);

    // Testing odb variables
    printf("txsize, rxsize: %i, %i\n", (int)daq_parameters["FIFO write size"], (int)daq_parameters["FIFO read size"]);

    return true;
}

bool CITIROC_connectBoard(char* CITIROC_serialNumber, int* CITIROC_usbID) {
    /**
    * Tries to open the board and, 
    * if succesful, pass the usb id by reference.
    * @param CITIROC_serialNumber 
    * @param CITIROC_usbID
    * @return true if CITIROC_usbID > 0
    */
    printf("Generating FTD2XX device list...");
    int FT_numberOfDevices;
    FT_STATUS status = FT_CreateDeviceInfoList(&FT_numberOfDevices);
    if (status != FT_OK){return false;}
    int numberOfUsbDevices = USB_GetNumberOfDevs();
    int usbID = OpenUsbDevice(CITIROC_serialNumber);
    if (usbID > 0) {CITIROC_usbID = usbID; return true;} 
    else {return false;}
}

bool CITIROC_disconnetBoard(const int* CITIROC_usbID) {
    CloseUsbDevice(CITIROC_usbID);
}

#endif 