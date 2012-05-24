#!/bin/bash
httpd -d ./build -f ./httpd.conf -X &
sleep 1;