#! perl
use Server;
use Client;
use Test::More;
use config;
use strict;

my $server = Server->new($config::HTTPD);
$server->loadModule(
    mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so'
);

my $conf = <<CONF;
ParrotLoader psgi.pbc
ParrotLoaderPath $config::BUILDDIR/build

<Location "/">
    ErrorDocument 404 "special 404"
    ParrotApplication winxed://hello.wxd/Greeter#hello
</Location>
CONF

$server->configure($conf);

my $winxed = <<WINXED;
class Greeter {
    function hello(var env) {
        return [ 203, {}, "Hello World!"];
    }
}
WINXED

$server->serve('hello.wxd', $winxed, 0755);
$server->start();
Client::setup($server);
is(content(''), 'Hello World!', 'yay for directory loading');
ok(0, 'false');
done_testing();
