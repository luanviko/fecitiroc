# fecitiroc
A MIDAS frontend for the WEEROC CITIROC1A evaluation board.

## Dependencies

The Makefile is thought around Linux systems and it was tested under Ubuntu 20.04, 
Ubuntu 22.04 and Fedora 32. 

This frontend requires the installation of the following frameworks/ libraries/ drivers:

* [ROOT framework](https://root.cern/install/),
* [MIDAS framework](https://daq00.triumf.ca/MidasWiki/index.php/Main_Page),
* [FTD2XX drivers](https://ftdichip.com/drivers/d2xx-drivers/),
* [LALUsb library](http://lalusb.free.fr/software.html),

and all their respective dependencies. 

After installing Midas at MIDASSYS directory, you may have to add a symbolic link or copy `$MIDASSYS/mxml/mxml.h` to your clone directory. 

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

# Communicating with the board

In the following, I will use the words register and subaddress interchangeably. 

This evaluation board has an Altera Cyclone III FPGA, 
and a CITIROC1A ASIC chip proprietary to Weeroc.
To communicate with the ASIC, you have to exchange information with the FPGA. 
In turn, all FPGA communication is handled by the FT2232HL microcontroller.

## 1 Microcontroller

The FT2232HL microcontroller chip is programmed by the FTD2XX driver API.
The LALUsb API wrapper is used here to provide extra data encapsulation
and error handling when writing and reading buffers with microcontroller.

For the `CITIROC` API wrapper, the FTD2XX drivers are directly used 
to generate a list of devices connected to the computer. 
Then, it uses LALUsb to write to and read from the board. 

The Weeroc CITIROCUI app at times makes use of FTD2XX.net for C#, 
which I am not sure is compatible with C/C++ under GNU/Linux.

## 2 FPGA

The FPGA memory is split into 8-bit registers, 
where each register is labeled by a subaddress.
Slow control parameters are written into such subaddresses.

To send the 8-bit word "10010100" to subaddress 3, use 
`CITIROC_sendWord(CITIROC_usbId, 3, "10010100")`.

Each subaddress contains an 8-bit word, 
where each bit sets a parameter to true (1) or false (0).
Each bit represents a firmware parameter, so please
refer to the `citiroc_fpga.xls` provided in the manual .

## 3 ASIC

The ASIC memory has 1144 bits, divided into registers of different sizes.
Differently than the FPGA, changing a bit in the ASIC requires 
rewriting the entire memory at the same time. 

You have to use the `CITIROC_sendASIC(CITIROC_usbId)`
function to generate the ASIC bit array from the ODB parameters
and write it on the ASIC memory, through the FPGA.

The `CITIROC_sendASIC` function will access the ODB parameters 
at the `ASIC_values` key and build the ASIC string accordingly.
It will then invoke the `CITIROC_writeASIC` function 
to put the FPGA in ASIC-writing mode and 
break the ASIC string into 8-bit words to be written on the board.

## 4 Data acquisition

You have to make sure that the ASIC is generating a trigger signal
for data acquisition (DAQ) to happen. 
If you do not see a trigger, 
please connect a probe to the `T<n>` pins to verify the signal.

<!-- `CITIROC_sendWord(... 43, "10000000")` -->
<!-- `CITIROC_sendWord(... 45, "") -->

# Running fecitiroc 


