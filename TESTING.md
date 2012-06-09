Testing mod_parrot
==================

Testing mod_parrot is tricky because its behavior depends on (at
least) the following aspects:

* The configuration directives
* The request to the webserver
* The handler serving the request
* The script responding to the request
* The languages installed (and their behavior).

languages and requests are not really our business, but configuration,
handlers, and script are. Thus we need a script that can setup apache
with different configurations and scripts, run request and compare
outputs with a known result.


