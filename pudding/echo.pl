#! perl
use Server;
use Client;
use config;
use Data::Dumper;
use Test::More tests => 7;
my $server = new Server($config::HTTPD);
$server->loadModule(
    mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so',
);
$server->configure(
    ParrotLanguage => 'winxed .winxed .wxd',
    ParrotLoaderPath => $config::BUILDDIR .'/build/',
    ParrotLoader => 'echo.pbc',
);
my $doc = <<HTML;
<html><head><title>Hi!</title></head><body><p>Hello</p></body></html>
HTML
$server->serve('foobar.html', $doc);
$server->start();
Client::setup($server);
is(status('foobar.winxed'), 203, 'custom header code');
is(status('foobar.wxd'), 203, 'also on a different registered suffix');
like(content('foobar.winxed'), qr/Loading compiler winxed/, 'it should mention we are loading winxed');
like(headers('foobar.winxed')->{'x-echo-user-agent'}, qr/HTTP-Tiny/, 'and it should echo our headers');
is(status('foobar.html'), 200, 'regular files should be served');

is(content('foobar.html'), $doc, 'just as they are');
is(status('quixquam.html'), 404, 'and it should throw 404s');
