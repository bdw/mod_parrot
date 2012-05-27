#!/bin/bash
if [ -f ./httpd.pid ]; then
	. ./stop-server.sh
fi
httpd -d ./build -f ./httpd.conf -X &
sleep 1;