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

#### CITIROC_sendWord()
#### CITIROC_readWord();
#### CITIROC_enableDAQ();
#### CITIROC_disableDAQ();

