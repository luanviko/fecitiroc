# fecitiroc
A MIDAS frontend for the WEEROC CITIROC1A evaluation board.

## Dependencies

This frontend requires the installation of the following frameworks/ libraries/ drivers:

* [ROOT framework](https://root.cern/install/),
* [MIDAS framework](https://daq00.triumf.ca/MidasWiki/index.php/Main_Page),
* [FTD2XX drivers](https://ftdichip.com/drivers/d2xx-drivers/),
* [LALUsb library](http://lalusb.free.fr/software.html),

and all their respective dependencies. 

## CITIROC API wrapper

The following functions are used to wrap FTD2XX and LALUsb functions 
and communicate with the board. 

* `bool CITIROC_connectBoard(char* serialNumber, int* usbId):`\
Tries to connect with the board by using the `serialNumber`.
Returns `usbId`


* `bool CITIROC_initializeBoard(int* usbId):`\
Initializes the board 
by setting timeouts, buffer size, temperature settings, etc.
Returns `usbId`

* `bool CITIROC_sendWord(int usbID, char subAddress, byte* binary):`\
Send a 1-byte word (e.g., `10000000`) to a register of given sub address on the FPGA's memory.
`binary` is a `byte`, define as an `unsigned char`.


* `bool CITIROC_readWord(int usbID, char subAddress, byte* word, int wordCount):`\
Returns array `word` and its size `wordCount` from `subAddress` at the FPGA's memory. 


* `bool CITIROC_enableDAQ(int usbID)`\
Sends a word to the correct subaddress to start data-aquisition mode.


* `bool CITIROC_disableDAQ(int usbID)`\
Sends a word to the correct subaddress to stop data-aquisition mode.


## Using odbxx

You can use obxx objects to inialize
the data-acquisition and slow-control parameters
on the online database. For example,
```c++
  midas::odb database_daq = {{"DAC 00", 15}};
  database_daq.connect("/Equipment/CITIROC1A_DAQ");
```
will first initiate a variable of name `DAC 00` and set it to `15`, then add this variable to the directory `"/Equipment/CITIROC1A_DAQ"` on the online database.

To access the values you add to the online database, 
you can simply create a dicitionary with the parameters
```c++
  midas::odb parameters("/Equipment/CITIROC1A_DAQ");
  int DAC00 = (int)parameters["DAC 00"];
```
and access the variable values by their key.
