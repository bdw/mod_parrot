#! perl
use Server;
use Client;
use config;
use Test::More tests => 9;
use Time::HiRes qw(time);
use strict;

my $server = Server->new($config::HTTPD);
$server->loadModule(
    mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so'
);

$server->configure(
    ParrotLoader => 'psgi.pbc',
    ParrotLoaderPath => $config::BUILDDIR . '/build',
    ParrotLanguage => 'winxed .wxd',
);

my $helloworld = <<WINXED;
function main(var env) {
    say("im not in output");
    return [ 203, { "x-hello": "world"}, "this is a sample psgi app"];
}
WINXED

my $delayed = <<WINXED;
function delayed(var responder) {
    responder(203, {'x-hello':'world'}, "a delayed response");
}

function main[main](var env) {
    return delayed;
}
WINXED

my $streaming = <<WINXED;
function delayed(var responder) {
    var writer = responder(203, {'x-hello':'world'});
    sleep(1);
   	writer.write('after a while'); 
}

function main[main](var env) {
    return delayed;
}
WINXED

$server->serve("hello.wxd", $helloworld, 0755);
$server->serve("delay.wxd", $delayed, 0755);
$server->serve('streaming.wxd', $streaming, 0755);
$server->start();
Client::setup($server);

is(content("hello.wxd"), "this is a sample psgi app", 'hello world works');
isnt(content("hello.wxd"), "im not in output", 'stdout is not send to client');
is(status("hello.wxd"), 203, 'status is set');
is(headers("hello.wxd")->{'x-hello'}, 'world', 'header is set');
is(content('delay.wxd'), "a delayed response", 'delayed response works');
is(headers('delay.wxd')->{'x-hello'}, 'world', 'header is also set on a delayed response');
is(status('delay.wxd'), 203, 'status in a delayed response' );
my $now = time;
is(content('streaming.wxd'), 'after a while', 'script can wait a bit');
my $then = time;
ok($then - $now > 0.5, 'and it takes that time');

