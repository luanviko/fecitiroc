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


* `bool CITIROC_printInfo(char* serialNumber):`\
Returns basic information about the board, 
such as serial number.


* `bool CITIROC_connectBoard(char* serialNumber, int* usbId):`\
Tries to connect with the board by using the `:serialNumber:`.
Returns `:usbId:`


* `bool CITIROC_initializeBoard(int* usbId):`\
Initializes the board 
by setting timeouts, buffer size, temperature settings, etc.
Returns `:usbId`:

* `bool CITIROC_sendWord(int usbID, char subAddress, byte* binary):`\
Send a 1-byte word (e.g., `10000000`) to a register of given sub address on the FPGA's memory.
`:binary:` is a `byte`, define as an `unsigned char`.


* `bool CITIROC_readWord(int usbID, char subAddress, byte* word, int wordCount):`\
Returns array `:word:` and its size `:wordCount:` from `:subAddress:` at the FPGA's memory. 


* `bool CITIROC_enableDAQ(int usbID):`\
Sends a word to the correct subaddress to start data-aquisition mode.


* `bool CITIROC_disableDAQ(int usbID):`\
Sends a word to the correct subaddress to stop data-aquisition mode.


## Using odbxx