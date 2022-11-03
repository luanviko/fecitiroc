/* API wrapper to interface MIDAS with CITIROC1A*/

// #include <stdio.h>
// #include <stdlib.h>
// #include <stdbool.h>
// #include <unistd.h>
// #include "odbxx.h"
// #include "ftd2xx.h"
// #include "LALUsb.h"
#include "CITIROC.h"

bool CITIROC_printInfo(char* CITIROC_serialNumber) {
    return true;
}

bool CITIROC_testParameters(){

     // Variables
    bool usbStatus;
    midas::odb daq_parameters(odbdir_DAQ);
    midas::odb temp_parameters(odbdir_temp);

    // Testing odb variables
    printf("txsize, rxsize: %i, %i\n", (int)daq_parameters["FIFO write size"], (int)daq_parameters["FIFO read size"]);

    return true;
}

bool CITIROC_initializeBoard(int* CITIROC_usbId, char* CITIROC_serialNumber) {
    /* Looks for the register parameters in ODB
    then changes parameters in the board using CITIROC_sendWord 
    or other LALUsb functions.*/

    // Variables
    bool usbStatus;
    midas::odb daq_parameters(odbdir_DAQ);
    midas::odb temp_parameters(odbdir_temp);

    // Testing odb variables
    printf("txsize, rxsize: %i, %i\n", (int)daq_parameters["FIFO write size"], (int)daq_parameters["FIFO read size"]);

    // General USB configurations
    printf("Setting USB configuration...\n");

    usbStatus = USB_Init(CITIROC_usbId, true);
    if (usbStatus == false) { return false; }

    // FIFO sizes, write and read.
    usbStatus = USB_SetXferSize(CITIROC_usbId, 8192, 32768);
    if (usbStatus == false) { return false; }

    usbStatus = USB_SetTimeouts(CITIROC_usbId, 200, 200);
    if (usbStatus == false) { return false; }
    
    // Temperature configuration
    printf("Initializing temperature sensors...\n");

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 63, "00110100");
    if (usbStatus == false) { return false; }

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 62, "00000011");
    if (usbStatus == false) { return false; }

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 62, "00000010");
    if (usbStatus == false) { return false; }

    return true;
}

bool CITIROC_sendWord(const int CITIROC_usbID, const char subAddress, const byte* binary) {
    /* Converts byte :binary: to hexadecimal 
    then sends such value to :subAddress: on the FPGA. */
    long integer = strtol(binary, NULL, 2);
    byte word[1] = {NULL};
    sprintf(word, "%x", integer);
    int realCount = UsbWrt(CITIROC_usbID, subAddress, word, 1);
    if (realCount == 1) {return true;} else {return false;}
}

bool CITIROC_readWord(const int CITIROC_usbID, const char subAddress, byte* word, const int wordCount) {
    /* Use UsbRd from LALUsb to read :word: from a given :subAddress:.
    :wordCount: must be equal to :realCount: */
    int realCount = UsbRd(CITIROC_usbID, subAddress, word, wordCount);
    if (realCount <= 0) {return false;} else {return true;}
}

bool CITIROC_enableDAQ(const int CITIROC_usbID) {
    /* Send daqON to sub address 43 to enable data acquisition. */
    bool usbStatus = CITIROC_sendWord(CITIROC_usbID, "43", &daqON);
    if (usbStatus) {return true;} else {return false;}
}

bool CITIROC_disableDAQ(const int CITIROC_usbID) {
    /* Send daqOFF to sub address 43 to enable data acquisition. */
    bool usbStatus = CITIROC_sendWord(CITIROC_usbID, "43", &daqOFF);
    if (usbStatus) {return true;} else {return false;}
}

