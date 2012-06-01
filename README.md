mod_parrot
==========

The awesome module that will run your parrot programs from within apache!

What is there:

* running a parrot bytecode file
* redirecting standard output to apache

What is not there:

* everything else

Where everything else includes configuration, loading external scripts,
writing http headers, reading standard CGI values, tests, and more. But
still!

### How to build

You will need apxs, parrot, parrot_config, and apache httpd.  apxs is
usually provided by a package like 'apache2-dev', and parrot_config comes
with parrot. You will need the header files for the APR (apache portable
runtime) but those should come with your aapache development package.

If your build does not work, please contact me (github.com/bdw). I can only
test so many systems and I'm deliberetaly trying to keep everything simple.

Parrot should be >= 4.4.0 as I require some of the newer embedding api
functions. Httpd should be >= 2.2, It might work on 2.0, but I have not
tested.

