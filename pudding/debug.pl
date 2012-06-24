#!/usr/bin/perl
use warnings;
use strict;
use Test::More;
use Server;
use Client;
use config;
use Data::Dumper;
use Carp;




my $server = Server->new($config::HTTPD);
$server->loadModule( mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so');
$server->configure( 
    ParrotLoaderPath => $config::BUILDDIR . '/build', 
    ParrotLanguage => "winxed .winxed",
    ParrotLoader => 'cgi.pbc',
);
$server->debug(); # this ends the script and starts gdb. You should have gdb.
