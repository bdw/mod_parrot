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
ParrotLoader psgi
ParrotLoaderPath $config::BUILDDIR/build

<Directory "{$server->{DocumentRoot}}/">
    ParrotApplication winxed://hello.wxd/Greeter#hello
</Directory>
CONF

$server->configure($conf);

my $winxed = <<WINXED;
class Greeter {
    function hello(var env) {
        return [ 203, {}, "Hello World!"];
    }
}
WINXED

$server->serve('foo.wxd', $winxed);
$server->debug();
Client::setup($server);
is(content(''), 'Hello World!', 'yay for directory loading');
ok(0, 'false');
done_testing();
