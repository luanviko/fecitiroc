# fecitiroc
A MIDAS frontend for the WEEROC CITIROC1A evaluation board.

## Dependencies

This frontend requires the installation of the following frameworks/ libraries/ drivers:

* ROOT
* MIDAS
* FTD2XX drivers
* LALUsb

and all their respective dependencies. 

## CITIROC API wrapper

The following functions are used to wrap FTD2XX and LALUsb functions 
and communicate with the board. 

#### bool CITIROC_printInfo(char* CITIROC_serialNumber);
#### bool CITIROC_testParameters();
#### bool CITIROC_initializeBoard(int* CITIROC_usbId);
#### bool CITIROC_sendWord(const int CITIROC_usbID, const char subAddress, const byte* binary);
#### bool CITIROC_readWord(const int CITIROC_usbID, const char subAddress, byte* word, const int wordCount);
#### bool CITIROC_enableDAQ(const int CITIROC_usbID);
#### bool CITIROC_disableDAQ(const int CITIROC_usbID);

