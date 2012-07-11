#! perl
use Server;
use Client;
use config;
use Test::More;
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
    say(typeof(writer));
}

function main[main](var env) {
    return delayed;
}
WINXED

$server->serve("hello.wxd", $helloworld, 0755);
$server->serve("delay.wxd", $delayed, 0755);
$server->serve('streaming.wxd', $streaming, 0755);
$server->debug();
Client::setup($server);
is(content("hello.wxd"), "this is a sample psgi app");
is(status("hello.wxd"), 203);
is(headers("hello.wxd")->{'x-hello'}, 'world');
is(content('delay.wxd'), "a delayed response");
is(headers('delay.wxd')->{'x-hello'}, 'world');
is(status('delay.wxd'), 203);

my $now = time;
is(content('streaming.wxd'), 'some message');
my $then = time;
ok($then - $now > 0.5);

print $server->errors;
done_testing;
