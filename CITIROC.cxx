/* API wrapper to interface MIDAS with CITIROC1A*/
#include "CITIROC.h"
#include <string>


int CITIROC_connect(char* CITIROC_serialNumber) {
    /**
     * Tries to open the board and, 
     * if succesful, pass the usb id by reference.
     * @param CITIROC_serialNumber 
     * @param CITIROC_usbID
     * @return true if CITIROC_usbID > 0
     */
    printf("FTD2XX: Generating FTD2XX device list...\n");
    int FT_numberOfDevices;
    FT_STATUS status = FT_CreateDeviceInfoList(&FT_numberOfDevices);
    if (status != FT_OK){return false;}
    int numberOfUsbDevices = USB_GetNumberOfDevs();
    printf("LALUSB: Trying to connect with the board...\n");
    int usbID = OpenUsbDevice(CITIROC_serialNumber);
    return usbID;
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

    printf("LALUSB: Initializing device of usb ID: %d...\n", CITIROC_usbId);
    usbStatus = USB_Init(CITIROC_usbId, true);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    printf("LALUSB: Setting buffer sizes to (FIFO write size, FIFO read size): %i, %i\n", txsize, rxsize);
    usbStatus = USB_SetXferSize(CITIROC_usbId, rxsize, txsize);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    printf("LALUSB: Setting timeout values to (write timeout, read timeout): %i, %i\n", ttimeout, rtimeout);    
    usbStatus = USB_SetTimeouts(CITIROC_usbId, ttimeout, rtimeout);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }
    
    printf("CITIROC: Enabling CITIROC1A temperature sensors...\n");
    usbStatus = CITIROC_sendWord(CITIROC_usbId, 63, "00110100");
    CITIROC_readFPGASubAddress(CITIROC_usbId, 63);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    printf("CITIROC: Setting temperature configurations...");
    usbStatus = CITIROC_sendWord(CITIROC_usbId, 62, "00000011");
    CITIROC_readFPGASubAddress(CITIROC_usbId, 62);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 62, "00000010");
    CITIROC_readFPGASubAddress(CITIROC_usbId, 62);
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    printf("CITIROC: writing firmware options...");
    usbStatus = CITIROC_sendFirmwareSettings(CITIROC_usbId);

    return true;
}

bool CITIROC_sendFirmwareSettings(const int CITIROC_usbId) {

    bool usbStatus;

    midas::odb firmware(odbdir_firmware);
    std::string disReadAdc         = (firmware["disReadAdc"] == true ) ? "1" : "0";
    std::string enSerialLink       = (firmware["enSerialLink"] == true ) ? "1" : "0";
    std::string selRazChn          = (firmware["selRazChn"] == true ) ? "1" : "0";    
    std::string valEvt             = (firmware["valEvt"] == true ) ? "1" : "0";
    std::string razChn             = (firmware["razChn"] == true ) ? "1" : "0";
    std::string selValEvt          = (firmware["selValEvt"] == true ) ? "1" : "0";
    std::string rstbPa             = (firmware["rstbPa"] == true) ? "1" : "0";
    std::string select             = (firmware["select"] == true) ? "1" : "0";
    std::string readOutSpeed       = (firmware["readOutSpeed"] == 1) ? "1" : "0";
    std::string NOR32polarity      = (firmware["OR32polarity"] == true ) ? "1" : "0";
    std::string ADC1               = (firmware["ADC1"] == true) ? "1" : "0";
    std::string ADC2               = (firmware["ADC2"] == true) ? "1" : "0";
    std::string rstbPS             = (firmware["rstbPS"] == true) ? "1" : "0";
    std::string timeOutHold        = (firmware["timeOutHold"] == true) ? "1" : "0";
    std::string selHold            = (firmware["selHold"] == true) ? "1" : "0";
    std::string selTrigToHold      = (firmware["selTrigToHold"] == true) ? "1" : "0";
    std::string triggerTorQ        = (firmware["triggerTorQ"] == true) ? "1" : "0";
    std::string pwrOn              = (firmware["pwrOn"] == true) ? "1" : "0";
    std::string selPSGlobalTrigger = (firmware["selPSGlobalTrigger"] == true) ? "1" : "0";
    std::string selPSMode          = (firmware["selPSMode"] == true) ? "1" : "0"; 
    std::string PSGlobalTrigger    = (firmware["PSGlobalTrigger"] == true) ? "1" : "0";
    std::string PSMode             = (firmware["PSMode"] == true) ? "1" : "0";
    
    usbStatus = CITIROC_sendWord(CITIROC_usbId, 0, ("00"+disReadAdc+enSerialLink+selRazChn+valEvt+razChn+selValEvt).c_str());
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 1, ("11"+select+rstbPa+readOutSpeed+NOR32polarity+"00").c_str());
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 2, ("000001"+ADC1+ADC2).c_str());
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 3, (rstbPS+"00"+timeOutHold+selHold+selTrigToHold+triggerTorQ+pwrOn).c_str());
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    usbStatus = CITIROC_sendWord(CITIROC_usbId, 5, ("0"+selPSGlobalTrigger+selPSMode+"000"+PSGlobalTrigger+PSMode).c_str());
    if (usbStatus == false) { USB_Perror(USB_GetLastError()); return false; }

    if (CITIROC_DEBUG_FLAG) {
        std::cout << std::endl;
        std::cout << "To be written on 0: " << ("00"+disReadAdc+enSerialLink+selRazChn+valEvt+razChn+selValEvt).c_str() << std::endl;
        CITIROC_readFPGASubAddress(CITIROC_usbId, 0);
        std::cout << "To be written on 1: " << ("111"+rstbPa+readOutSpeed+NOR32polarity+"00").c_str() << std::endl;    
        CITIROC_readFPGASubAddress(CITIROC_usbId, 1);
        std::cout << "To be written on 2: " << ("000001"+ADC1+ADC2).c_str() << std::endl;
        CITIROC_readFPGASubAddress(CITIROC_usbId, 2);
        std::cout << "To be written on 3: " << (rstbPS+"00"+timeOutHold+selHold+selTrigToHold+triggerTorQ+pwrOn).c_str() << std::endl; 
        CITIROC_readFPGASubAddress(CITIROC_usbId, 3);
        std::cout << "To be written on 5: " << ("0"+selPSGlobalTrigger+selPSMode+"000"+PSGlobalTrigger+PSMode).c_str() << std::endl;
        CITIROC_readFPGASubAddress(CITIROC_usbId, 5);
    }
     
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

bool CITIROC_sendWord(const int CITIROC_usbID, const char subAddress, const char* bitArray) {
    /* Converts char :word: to hexadecimal 
    then sends such value to :subAddress: on the FPGA. */
    byte* binary = (byte*)bitArray;
    long integer = strtol(binary, NULL, 2);
    byte word[1] = {(byte)integer};
    int realCount = UsbWrt(CITIROC_usbID, subAddress, word, 1);
    if (realCount == 1) {return true;} else {return false;}
}

bool CITIROC_sendWords(const int CITIROC_usbID, const char subAddress, char* asicString, const int wordCount) {
    /* Converts byte :binary: to hexadecimal 
    then sends such value to :subAddress: on the FPGA. */

    byte reverseAsicString[1144];
    byte asicWords[wordCount];
    int writtenCount = 0;

    printf("asic: %s\n", asicString);
    for (int i=0; i<1144; i++) {reverseAsicString[i] = asicString[1143-i];}
    printf("reverse: %s\n", reverseAsicString);
    
    for (int i=0; i<wordCount; i++) {

        char temporaryWord[9] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        char reversedTemporaryWord[9] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};    
        for (int j=0; j<8; j++) {
            strncat(reversedTemporaryWord, &reverseAsicString[8*i+7-j], 1);
        }
        printf("Reversed temporary string: %s\n", reversedTemporaryWord);
        long reversedIntegerWord = strtol(reversedTemporaryWord, NULL, 2);
        asicWords[i] = (byte)reversedIntegerWord;
    }
    writtenCount = UsbWrt(CITIROC_usbID, subAddress, asicWords, 143);
    printf("Byte count to ASIC: %d\n", writtenCount);
    return writtenCount;
}

bool CITIROC_readWord(const int CITIROC_usbID, const char subAddress, char* word, const int wordCount) {
    /* Use UsbRd from LALUsb to read :word: from a given :subAddress:.
    :wordCount: must be equal to :realCount: */
    int realCount = UsbRd(CITIROC_usbID, subAddress, word, wordCount);
    if (realCount <= 0) {return false;} else {return true;}
}

bool CITIROC_printWord(char subAddress, char* word, int wordCount){
    printf("Printing subaddress %3d: ", subAddress);
    for (int i=0; i<wordCount; i++) {
        char final[1024];
        int bits[8];
        sprintf(final, "0x%x", (unsigned char)word[i]);
        long ret = strtol(final, NULL, 16);
        CITIROC_convertToBits(ret, 8, bits);
        printf("%ld (", ret);
        for (int j=0; j<8; j++) {
            printf("%d", bits[j]);
        }
        printf(")\n");
    }
    return true;
}

bool CITIROC_readFPGASubAddress(const int usbId, const char subAddress) {
    char array0;
    bool readStatus   = CITIROC_readWord(usbId, subAddress, &array0, 1);
    bool printStatus  = CITIROC_printWord(subAddress, &array0, 1);
}

bool CITIROC_readFIFO(const int CITIROC_usbID, byte* fifo20, byte*fifo21, byte* fifo23, byte* fifo24, int* wordCount) {
    
    // Private variables
    bool usbStatus;

    // The FIFO in the FPGA can store up to 100 acquisitions per cycle
    int fifoLength = 100;
    int numberOfAcquisitions = 1000;
    int numberOfCycles = (int)numberOfAcquisitions/fifoLength;

    // Add one cycle for the remainder or if nbCycle == 0 (ie nbAcq < 100)
    if (numberOfAcquisitions % fifoLength != 0 || numberOfCycles == 0) {numberOfCycles++;} 

    for (int cycle = 0; cycle < numberOfCycles; cycle++) {

        // TODO: 
        // Set number of channels outside this functions
        int numberOfChannels = 1;

        // Determine the number of acquisitions in a cycle.
        // TODO: Understand why is it inside the loop.
        int acquisitionsInCycle = 0;
        if (numberOfAcquisitions >= fifoLength) {
            acquisitionsInCycle = fifoLength;
        } else if (numberOfAcquisitions < fifoLength) {
            acquisitionsInCycle = numberOfAcquisitions;
        }

        // Subaddress 45:
        // Number of acquisitions to save in FIFO 
        // before reading it.
        // usbStatus = sendInt(CITIROC_usbID, 45, acquisitionsInCycle);

        usbStatus = CITIROC_sendWord(CITIROC_usbID, 43, "10000000");

        int dataCount = (numberOfChannels+1)*acquisitionsInCycle;
        byte* fifo20, fifo21, fifo23, fifo24;
        usbStatus = CITIROC_readWord(CITIROC_usbID, "20", fifo20, dataCount);

        usbStatus = CITIROC_sendWord(CITIROC_usbID, 43, "00000000");

    }

    return true;


}

void CITIROC_raiseException() {
    /** 
     *  Find the error raised by the LALUsb API.
     */
    printf("LALUsb raised the following expection:\n");
    USB_Perror(USB_GetLastError());
}

bool CITIROC_convertToBits(int numberToConvert, const int numberOfBits, int* binary) {
    /** 
     *  Convert integer into binary. 
     *  Returns an array with size *numberOfBits*, 
     *  with remaining values set to zero.
     *  Little-endian format.
     @param numberToConvert: decimal integer to convert to binary
     @param numberOfBits: number of bits you want
     @param binary: array with binary number
     @return true
     */ 
    for (int j=numberOfBits-1; j>=0; j--) {
        if (numberToConvert <= 0) {binary[j] = 0;}
        else {binary[j] = numberToConvert%2;}
        numberToConvert = numberToConvert/2;
    }
    return true;
}

bool CITIROC_sendASIC(const int CITIROC_usbID) {
    /** 
     * Create ASIC bit-stack and send to FPGA.
     @param odbdir_asic_addresses: global odbxx object.
     @param odbdir_asic_values: global odbxx object.
     @return true if usbStatus successful.
     */

    printf("Preparing ASIC buffer...\n");

    int bitCounter = 0;
    std::array<int, 1144> asicStack;
    std::vector<int> asicVector;

    std::string bitStack;
    midas::odb asic_subaddress(odbdir_asic_addresses);
    midas::odb asic_values(odbdir_asic_values);
    midas::odb asic_sizes(odbdir_asic_sizes);

    std::vector<int> chn          = (std::vector<int>)asic_values["chn"];
    std::vector<int> calibDacQ    = (std::vector<int>)asic_values["calibDacQ"];
    std::vector<int> shapTimeLg   = (std::vector<int>)asic_values["shapingTimeLg"];
    std::vector<int> shapTimeHg   = (std::vector<int>)asic_values["shapingTimeHg"];
    std::vector<int> inputDAC     = (std::vector<int>)asic_values["inputDac"];
    std::vector<int> cmdInputDAC  = (std::vector<int>)asic_values["sc_cmdInputDac"];
    std::vector<int> highGain     = (std::vector<int>)asic_values["paHgGain"];
    std::vector<int> lowGain      = (std::vector<int>)asic_values["paLgGain"];
    std::vector<int> testHighGain = (std::vector<int>)asic_values["CtestHg"];
    std::vector<int> testLowGain  = (std::vector<int>)asic_values["CtestLg"];
    std::vector<int> enablePA     = (std::vector<int>)asic_values["enPa"];
    
    for (midas::odb& subkey : asic_values) {
        std::vector<int> genericVector = (std::vector<int>)asic_values[subkey.get_name().c_str()];
        for (int i=0; i < genericVector.size(); i++) {
            int n = (int)genericVector.at(i);
            const int numberOfBits = (int)asic_sizes[subkey.get_name().c_str()];
            int binary[numberOfBits]; 
            CITIROC_convertToBits(n, numberOfBits, binary);
            for (int j=0; j<numberOfBits; j++) {asicVector.push_back(binary[j]);}
        }
    }

    int bitCounterChn = 0;
    int bitCalibDacQ  = 0; 
    for (int i=0; i<chn.size(); i++) {
        int binary4[4] = {0, 0, 0, 0};

        CITIROC_convertToBits(chn.at(i), 4, binary4);
        for (int j=0; j<4; j++) {asicStack[0+i*4+j]=binary4[3-j]; bitCounterChn++;}
        for (int j=0; j<4; j++) {asicVector.at(0+i*4+j) = binary4[3-j];} 

        CITIROC_convertToBits(calibDacQ.at(i), 4, binary4);
        for (int j=0; j<4; j++) {asicStack[128+i*4+j]=binary4[3-j]; bitCalibDacQ++;}
        for (int j=0; j<4; j++) {asicVector.at(128+i*4+j) = binary4[3-j];} 

    }
    printf("Channel 0 to 31 4-bit_t: (should be 128): %d\n", bitCounterChn);
    printf("Channel 0 to 31 4-bit:   (should be 128): %d\n", bitCalibDacQ);

    int bitCounterLGshaper = 0;
    int bitCounterHGshaper = 0;
    for (int i=0; i<shapTimeLg.size(); i++) {
        int binary3[3] = {0, 0, 0};

        CITIROC_convertToBits(shapTimeLg.at(i), 3, binary3);
        for (int j=0; j<3; j++) {asicStack[315+i*3+j]=binary3[2-j]; bitCounterLGshaper++;}
        for (int j=0; j<3; j++) {asicVector.at(315+i*3+j) = binary3[2-j];} 

        CITIROC_convertToBits(shapTimeHg.at(i), 3, binary3);
        for (int j=0; j<3; j++) {asicStack[320+i*3+j]=binary3[2-j]; bitCounterHGshaper++;}
        for (int j=0; j<3; j++) {asicVector.at(320+i*3+j) = binary3[2-j];} 

    }
    printf("Time constant LG shaper (should be 3): %d\n", bitCounterLGshaper);
    printf("Time constant HG shaper (should be 3): %d\n", bitCounterHGshaper);

    int bitCounterPA = 0;
    for (int i=0; i < highGain.size(); i++) {
        int binary6[6] = {0, 0, 0, 0, 0, 0};
        
        CITIROC_convertToBits(highGain.at(i), 6, binary6);
        // for (int j=0; j<6; j++) {asicStack[619+i*15+j]=binary6[j]; bitCounterPA++;}
        // for (int j=0; j<6; j++) {asicVector.at(619+i*15+j)=binary6[j];}
        for (int j=0; j<6; j++) {asicStack[619+i*15+j]=binary6[5-j]; bitCounterPA++;}
        for (int j=0; j<6; j++) {asicVector.at(619+i*15+j)=binary6[5-j];}

        CITIROC_convertToBits(lowGain.at(i), 6, binary6);
        // for (int j=0; j<6; j++) {asicStack[619+i*15+j]=binary6[j]; bitCounterPA++;}
        // for (int j=0; j<6; j++) {asicVector.at(625+i*15+j) = binary6[j];}
        for (int j=0; j<6; j++) {asicStack[625+i*15+j]=binary6[5-j]; bitCounterPA++;}
        for (int j=0; j<6; j++) {asicVector.at(625+i*15+j) = binary6[5-j];}


        asicStack[631+i*15] = testHighGain.at(i); bitCounterPA++;
        asicVector.at(631+i*15) = testHighGain.at(i);

        asicStack[632+i*15] = testLowGain.at(i); bitCounterPA++;
        asicVector.at(632+i*15) = testLowGain.at(i);

        asicStack[633+i*15] = enablePA.at(i); bitCounterPA++;
        asicVector.at(633+i*15) = enablePA.at(i);

    }
    printf("Channel 0-31 PA bits (should be 480): %d\n", bitCounterPA );

    int bitCounterDAC = 0;
    for (int i=0; i < inputDAC.size(); i++) {
        int binary8[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        CITIROC_convertToBits(inputDAC.at(i), 8, binary8);
        
        for (int j=0; j<8; j++) {asicStack[331+i*9+j]=binary8[j]; bitCounterDAC++;}
        for (int j=0; j<8; j++) {asicVector.at(331+i*9+j) = binary8[j];}

        asicStack[339+i*9] = cmdInputDAC.at(i); bitCounterDAC++;
        asicVector.at(339+i*9) = cmdInputDAC.at(i);
        }
    printf("Input 8-bit DAC (should be 288): %d\n", bitCounterDAC );
            
    printf("ASIC stack: ");
    for (int i=0; i < 1144; i++) {printf("%d", asicVector.at(i));}
    printf("\nSize of stack: %d\n", asicVector.size() );

    CITIROC_writeASIC(CITIROC_usbID, asicVector, asicVector.size()/8);

    return true;
}

bool CITIROC_writeASIC(const int CITIROC_usbID, std::vector<int> asicVector, const int numberOfWords) {
    /** 
     *  Reverse asicVector. 
     *  Split asicVector into an array of 8-character elements.
     *  Send words to board.
     *  Please refer to "citiroc_fpga.xls" document.
     @param asicVector: Contain vector with bits.
     @param numberOfWords: Number of words inside asicVector.
     */

    bool usbStatus;
    int realCount = 0;
    int reverseAsicString[1144];
    byte asicWords[numberOfWords];
    midas::odb firmware(odbdir_firmware);
    std::vector<int> reverseAsicVector;
    std::vector<const char*> words;

    printf("ASIC size: %d\n", numberOfWords);

    for (int i=0; i<asicVector.size(); i++) {
        reverseAsicString[i] = asicVector.at(asicVector.size()-1-i);
        // reverseAsicString[i] = asicVector.at(i);
    } 

    for (int i=0; i<numberOfWords; i++) {
        // char reversedTemporaryWord[9] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        std::string reversedTemporaryWord="";
        for (int j=0; j<8; j++) {reversedTemporaryWord+=std::to_string(reverseAsicString[8*i+7-j]);} // <<-USE THIS ONE
        // for (int j=0; j<8; j++) {reversedTemporaryWord+=std::to_string(reverseAsicString[8*i+j]);}
        byte* binary = (byte*)reversedTemporaryWord.c_str();
        long reversedIntegerWord = strtol(binary, NULL, 2);
        asicWords[i] = (byte)reversedIntegerWord;
        if (CITIROC_DEBUG_FLAG) {
            printf("asic[%d]: %u\n", i, asicWords[i]);
        }
        
    }

    std::string rstbPa        = (firmware["rstbPa"] == true) ? "1" : "0";
    std::string readOutSpeed  = (firmware["readOutSpeed"] == true) ? "1" : "0";
    std::string NOR32polarity = (firmware["OR32polarity"] == true ) ? "1" : "0";
    std::string disReadAdc    = (firmware["disReadAdc"] == true ) ? "1" : "0";
    std::string enSerialLink  = (firmware["enSerialLink"] == true ) ? "1" : "0";
    std::string selRazChn     = (firmware["selRazChn"] == true ) ? "1" : "0";
    std::string valEvt        = (firmware["valEvt"] == true ) ? "1" : "0";
    std::string razChn        = (firmware["razChn"] == true ) ? "1" : "0";
    std::string selValEvt     = (firmware["selValEvt"] == true ) ? "1" : "0";

    // Select slow-control parameters on FPGA
    usbStatus = CITIROC_sendWord(CITIROC_usbID, 1, ("111"+rstbPa+readOutSpeed+NOR32polarity+"00").c_str());
    if (CITIROC_DEBUG_FLAG) {CITIROC_readFPGASubAddress(CITIROC_usbID, 1);}
    if (usbStatus == false) {USB_Perror(USB_GetLastError()); return false;}

    // Send ASIC bits to FPGA
    realCount = UsbWrt(CITIROC_usbID, 10, asicWords, 143);
    if (realCount != numberOfWords) {USB_Perror(USB_GetLastError()); return false;}

    // Start shifting parameters
    usbStatus = CITIROC_sendWord(CITIROC_usbID, 1, ("111"+rstbPa+readOutSpeed+NOR32polarity+"10").c_str());    
    if (CITIROC_DEBUG_FLAG) {CITIROC_readFPGASubAddress(CITIROC_usbID, 1);}
    if (usbStatus == false) {USB_Perror(USB_GetLastError()); return false;}

    // Stop shifting parameters
    usbStatus = CITIROC_sendWord(CITIROC_usbID, 1, ("111"+rstbPa+readOutSpeed+NOR32polarity+"00").c_str());
    if (CITIROC_DEBUG_FLAG) {CITIROC_readFPGASubAddress(CITIROC_usbID, 1);}
    if (usbStatus == false) {USB_Perror(USB_GetLastError()); return false;}

    // Slow control test checksum -> test query
    usbStatus = CITIROC_sendWord(CITIROC_usbID, 0, ("10"+disReadAdc+enSerialLink+selRazChn+valEvt+razChn+selValEvt).c_str());
    if (CITIROC_DEBUG_FLAG) {CITIROC_readFPGASubAddress(CITIROC_usbID, 1);}
    if (usbStatus == false) {USB_Perror(USB_GetLastError()); return false;}

    // Send-slow control parameters to FPGA
    realCount = UsbWrt(CITIROC_usbID, 10, asicWords, 143);
    if (realCount != numberOfWords) {USB_Perror(USB_GetLastError()); return false;}

    // Start shifting parameters
    usbStatus = CITIROC_sendWord(CITIROC_usbID, 1, ("111"+rstbPa+readOutSpeed+NOR32polarity+"10").c_str());    
    if (CITIROC_DEBUG_FLAG) {CITIROC_readFPGASubAddress(CITIROC_usbID, 1);}
    if (usbStatus == false) {USB_Perror(USB_GetLastError()); return false;}

    // Stop shifting parameters
    usbStatus = CITIROC_sendWord(CITIROC_usbID, 1, ("111"+rstbPa+readOutSpeed+NOR32polarity+"00").c_str());
    if (CITIROC_DEBUG_FLAG) {CITIROC_readFPGASubAddress(CITIROC_usbID, 1);}
    if (usbStatus == false) {USB_Perror(USB_GetLastError()); return false;}

    // Correlation test result
    char correlationResult;
    usbStatus = CITIROC_readWord(CITIROC_usbID, 4, &correlationResult, 1);
    CITIROC_readFPGASubAddress(CITIROC_usbID, 4);
    CITIROC_checksumQuery(CITIROC_usbID);
    // printf("Correlation: %s\n", &correlationResult);
    // std::string correlationResultString = std::string(&correlationResult);
    // std::string correlationResultString = correlationResult;
    // int stringComparison = correlationResultString.compare("00000000");
    // printf("Word4 comparison result (0 is good): %d\n.", stringComparison);
    // if (stringComparison != 0) {USB_Perror(USB_GetLastError()); return false;}

    // Reset slow-control checksum test query
    usbStatus = CITIROC_sendWord(CITIROC_usbID, 0, ("00"+disReadAdc+enSerialLink+selRazChn+valEvt+razChn+selValEvt).c_str());

    usbStatus = CITIROC_sendFirmwareSettings(CITIROC_usbID);

    return false;

}

bool CITIROC_checksumQuery(const int CITIROC_usbId) {
    // TO-DO: Check casting of char to unsigned char in sprintf.
    char array0;
    bool readStatus   = CITIROC_readWord(CITIROC_usbId, 4, &array0, 1);
    printf("Test query: ");
    char final[1024];
    int bits[8];
    sprintf(final, "0x%x", (unsigned char)array0);
    long ret = strtol(final, NULL, 16);
    CITIROC_convertToBits(ret, 8, bits);
        printf("%ld (", ret);
        for (int j=0; j<8; j++) {
            printf("%d", bits[j]);
        }
        printf(")\n");
    if (bits[0] != 0) {printf("ASIC string is not correct.\n"); return false;}
    return true;
}