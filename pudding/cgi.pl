#!/usr/bin/perl -w
use strict;
use warnings;
use Server;
use Client;
use Test::More tests => 7;
use config;
use Data::Dumper;

my $server = Server->new($config::HTTPD);
$server->loadModule( 
    mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so'
);

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

my $withStatus = <<WINXED;
function main[main]() {
    say("HTTP/1.1 202 Accepted");
    say("hello, world!");
}
WINXED

my $otherStatus = <<WINXED;
function main[main]() {
    say("Status: 202");
    say('bye world');
}

WINXED

$server->serve('runs-code.winxed', $winxed, 0755);
$server->serve('has-headers.winxed', $withHeaders, 0755);
$server->serve('unexecutable.winxed', $winxed, 0644);
$server->serve('statusCode.winxed', $withStatus, 0755);
$server->serve('otherStatus.wxd', $otherStatus, 0755);
$server->start();
Client::setup($server);
is(content('runs-code.winxed'), 'hello world', 'obligatory hello world');
is(headers('has-headers.winxed')->{'x-foo'}, 'bar', 'setting a header');
is(content('has-headers.winxed'), "Some content\n", 'but also some content');
is(status('statusCode.winxed'), 202, 'custom status code');
is(status('otherStatus.wxd'), 202, 'another way to report a custom error code');
# errors
is(status('unexecutable.winxed'), 403, 'not executable');
is(status('does-not-exist.winxed'), 404, 'not found');

