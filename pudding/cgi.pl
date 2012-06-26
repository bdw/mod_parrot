#!/usr/bin/perl -w
use strict;
use warnings;
use Server;
use Client;
use Test::More tests => 2;
use config;
use Data::Dumper;
my $server = Server->new($config::HTTPD);
$server->loadModule( mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so');

$server->configure( 
    ParrotLoader => 'cgi.pbc',
    ParrotLoaderPath => $config::BUILDDIR . '/build',
    ParrotLanguage => 'winxed .winxed .wxd'
);

my $winxed =  <<WINXED;
    function main[main]() { print("hello world"); }
WINXED

my $withHeaders   = <<WINXED;
function main[main]() {
    say("X-Foo: bar");
    say("");
    say("Some content");
}
WINXED

$server->serve('example.winxed', $winxed, 0755);
$server->serve('example2.winxed', $withHeaders, 0755);
$server->start();
Client::setup($server);
is(content('example.winxed'), 'hello world');
is(headers('example2.winxed')->{'x-foo'}, 'bar');
is(content('example2.winxed'), "Some content\n");
