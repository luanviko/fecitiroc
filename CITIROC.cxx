/* API wrapper to interface MIDAS with CITIROC1A*/
#include "CITIROC.h"


bool CITIROC_connect(char* CITIROC_serialNumber, int* CITIROC_usbID) {
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

bool CITIROC_initialize(const int CITIROC_usbId) {
    /**
     * Looks for the right parameters at ODB and 
     * writes such parameters on the board registers. 
     * Run this before trying data acquisition.
     * @param  CITIROC_usbId: usb id for the board.
     * @return true if all the writings are done correctly. 
     */

    // Variables from odb.
    bool usbStatus;
    midas::odb daq_parameters(odbdir_DAQ);
    midas::odb temp_parameters(odbdir_temp);
    int txsize = (int)daq_parameters["FIFO write size"];
    int rxsize = (int)daq_parameters["FIFO read size"];
    int ttimeout = (int)daq_parameters["Write time out (1-255 ms)"];
    int rtimeout = (int)daq_parameters["Read time out (1-255 ms)"];

    printf("Initializing board...\n");
    usbStatus = USB_Init(CITIROC_usbId, true);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    printf("Setting buffer sizes to (FIFO write size, FIFO read size): %i, %i\n", txsize, rxsize);
    usbStatus = USB_SetXferSize(CITIROC_usbId, rxsize, txsize);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    printf("Setting timeout values to (write timeout, read timeout): %i, %i\n", ttimeout, rtimeout);    
    usbStatus = USB_SetTimeouts(CITIROC_usbId, ttimeout, rtimeout);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }
    
    printf("Enabling temperature sensors...\n");
    usbStatus = CITIROC_sendWord(CITIROC_usbId, 63, (byte)temp_parameters[odb_temp_enable]);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    printf("Setting temperature configurations...");
    usbStatus = CITIROC_sendWord(CITIROC_usbId, 62, (byte)temp_parameters[odb_temp_configa]);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }
    usbStatus = CITIROC_sendWord(CITIROC_usbId, 62, (byte)temp_parameters[odb_temp_configb]);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    return true;
}

bool CITIROC_reset(const int CITIROC_usbID){
    /**
     * Hardware reset of FT2232HL chip and restart of FTD2XX drivers. 
     * Buffer are cleared. 
     * Buffer sizes and timouts are kept the same.
     * @param CITIROC_usbID
     * @return true if CITIROC_usbID > 0
     */
    USB_ResetDevice(CITIROC_usbID);
    return true;
}

bool CITIROC_disconnet(const int CITIROC_usbID) {
    /** 
     * Closes devide with a given usb id.
     * @param CITIROC_usbID
     * @return true always.
     */
    CloseUsbDevice(CITIROC_usbID);
    return true;
}

bool CITIROC_enableDAQ(const int CITIROC_usbID) {
    /** 
     * Send daqON to sub address 43 to enable data acquisition.
     * @param CITIROC_usbID
     * @return true if usbStatus succesful.
     */
    bool usbStatus = CITIROC_sendWord(CITIROC_usbID, "43", "10000000");
    if (usbStatus) {return true;} else {return false;}
}

bool CITIROC_disableDAQ(const int CITIROC_usbID) {
    /** 
     * Send daqOFF to sub address 43 to enable data acquisition.
     * @param CITIROC_usbID
     * @return true if usbStatus succesful.
     */
    bool usbStatus = CITIROC_sendWord(CITIROC_usbID, "43", "00000000");
    if (usbStatus) {return true;} else {return false;}
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