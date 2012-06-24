#!/usr/bin/perl -w
use strict;
use warnings;
use Server;
use Client;
use Test::More;
use config;

my $server = Server->new($config::HTTPD);
$server->loadModule( mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so');

$server->configure( 
    ParrotLoader => 'cgi.pbc',
    ParrotLoaderPath => $config::BUILDDIR . '/build',
    ParrotLanguage => 'winxed .winxed .wxd'
);

my $winxed =  <<WINXED;
    function main[main]() { say("hello world"); }
WINXED

$server->serve('example.winxed', $winxed, 0755);
$server->start();
Client::setup($server);
print content('example.winxed');
print $server->errors;
