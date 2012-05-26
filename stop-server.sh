#!/bin/bash
kill `cat ./httpd.pid`
rm -f ./httpd.pid
