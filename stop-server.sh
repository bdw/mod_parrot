#!/bin/bash
kill `cat ./build/httpd.pid`
rm -f ./build/httpd.pid
