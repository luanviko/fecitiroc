/* API wrapper to interface MIDAS with CITIROC1A*/
#include "CITIROC.h"
#include <string>


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

bool CITIROC_testParameters(const int CITIROC_usbID){ 

    return true;
}

bool CITIROC_readFIFO(const int CITIROC_usbID, byte* fifo20, byte*fifo21, byte* fifo23, byte* fifo24, int* wordCount) {
    
    return true;
}

bool CITIROC_raiseException() {
    USB_Perror(USB_GetLastError());
    return true;
}

bool CITIROC_convertToBits(int n, const int numberOfBits, int* binary) {
    /** 
     *  Convert integer into binary. 
     *  Returns an array with size *numberOfBits*, 
     *  with remaining values set to zero.
     *  Little-endian format.
     @param n: decimal integer to convert to binary
     @param numberOfBits: number of bits you want
     @param binary: array with binary number
     @return true
     */ 
    for (int j=numberOfBits-1; j>=0; j--) {
        if (n <= 0) {binary[j] = 0;}
        else {binary[j] = n%2;}
        n = n/2;
    }
    return true;

    // for(int j=0; n>0; j++) {
    //     asicStack[j] = n%2;    
    //     // printf("asicStack[%d]: %d\n", bitCounter, asicStack[bitCounter]);
    //     n=n/2;    
        
    // }
}

bool CITIROC_sendASIC() {
    /** 
     * Create ASIC bit-stack and send to FPGA.
     @param odbdir_asic_addresses: global odbxx object.
     @param odbdir_asic_values: global odbxx object.
     @return true if usbStatus successful.
     */
    int bitCounter = 0;
    int asicStack[1144];

    std::string bitStack;
    midas::odb asic_subaddress(odbdir_asic_addresses);
    midas::odb asic_values(odbdir_asic_values);
    midas::odb asic_sizes(odbdir_asic_sizes);

    for (midas::odb& subkey : asic_values) {
        std::vector<int> genericVector = (std::vector<int>)asic_values[subkey.get_name().c_str()];
        for (int i=0; i < genericVector.size(); i++) {
            int n = (int)genericVector.at(i);
            const int numberOfBits = (int)asic_sizes[subkey.get_name().c_str()];
            int binary[numberOfBits]; 
            CITIROC_convertToBits(n, numberOfBits, binary);
            for (int j=0; j<numberOfBits; j++) { std::cout << binary[j] << std::endl;}
            // for (int j=0; j<numberOfBits; j++) {asicStack[bitCounter] = binary[j]; bitCounter++;}
            // std::cout << subkey.get_name() << " :: " << genericVector.at(i) << " :: ";
            // for (int j=0; j<numberOfBits; j++) {std::cout << binary[j];}
            // std::cout << std::endl;
        }
    }

    std::vector<int> inputDAC     = (std::vector<int>)asic_values["inputDac"];
    std::vector<int> cmdInputDAC  = (std::vector<int>)asic_values["sc_cmdInputDac"];
    std::vector<int> highGain     = (std::vector<int>)asic_values["paHgGain"];
    std::vector<int> lowGain      = (std::vector<int>)asic_values["paLgGain"];
    std::vector<int> testHighGain = (std::vector<int>)asic_values["CtestHg"];
    std::vector<int> testLowGain  = (std::vector<int>)asic_values["CtestLg"];
    std::vector<int> enablePA     = (std::vector<int>)asic_values["enPa"];

    for (int i=0; i < highGain.size(); i++) {
        int binary6[6] = {0, 0, 0, 0, 0, 0};
        CITIROC_convertToBits(highGain.at(i), 6, binary6);
        for (int j=0; j<6; j++) {asicStack[619+i*15+j]=binary6[j];}
        asicStack[625+i*15] = lowGain.at(i);
        asicStack[631+i*15] = testHighGain.at(i);
        asicStack[632+i*15] = testLowGain.at(i);
        asicStack[633+i*15] = enablePA.at(i);
    }

    for (int i=0; i < inputDAC.size(); i++) {
        int binary8[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        CITIROC_convertToBits(inputDAC.at(i), 8, binary8);
        for (int j=0; j<8; j++) {asicStack[331+i*15+j]=binary8[j];}
        asicStack[339+i*9] = cmdInputDAC.at(i);
    }


    for (int i=0; i < 10; i++) {
        std::cout << i << "  " << asicStack[i] << std::endl;
    }

    // for (int i=0; i < 1143; i++) {
    //     printf("asicStack[%d]: %d\n", i, asicStack[i]);
    // }
    printf("Size of stack: %d\n", sizeof(asicStack)/sizeof(asicStack[0]) );

    return true;
}

bool CITIROC_readASIC() {

    return 0;
}