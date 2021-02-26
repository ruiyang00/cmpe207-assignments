#!/bin/bash
gcc enhanced-generic-client.c -o enhanced-generic-client
./enhanced-generic-client -m HelloWorld -h google.com -s echo -p tcp
