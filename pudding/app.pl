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
    ParrotApplication winxed://hello.wxd/Greeter#hello
</Location>

<Location "/foo">
    ParrotApplication winxed://hello.wxd/#bye
</Location>
CONF

$server->configure($conf);

my $winxed = <<WINXED;
class Greeter {
    function hello(var env) {
        return [ 203, {'x-hello':'world'}, "Hello World!"];
    }
}

function bye(var env) {
    return [ 203, {'x-bye': 'world'}, "Bye world!"];
}
WINXED

$server->serve('hello.wxd', $winxed, 0755);
$server->start();
Client::setup($server);
is(content(''), 'Hello World!', 'yay for app loading');
is(status(''), 203, 'correct status');
is(headers('')->{'x-hello'}, 'world', 'header output!');
is(content('foo'), 'Bye world!', 'multiple apps!');
done_testing();
