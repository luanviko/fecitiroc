#!/bin/sh

./kill_daq.sh

odbedit -c clean
mhttpd -e citiroc -D 
sleep 2
./fecitiroc.exe

#echo Please point your web browser to http://localhost:8080

#end file
