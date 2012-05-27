#!/bin/bash
kill -s 3 `cat ./httpd.pid`
rm -f ./httpd.pid
