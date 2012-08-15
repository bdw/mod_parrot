#!/usr/bin/perl
use Server;
use Client;
use Test::More tests => 2;
use Data::Dumper;
use config;

$server = Server->new($config::HTTPD);
$server->loadModule(
    mod_parrot => $config::BUILDDIR . '/build/mod_parrot.so',
    mime_module => $config::INSTALLDIR . '/mod_mime.so',
);
$server->configure(
    ParrotLoader => 'echo.pbc',
    ParrotLoaderPath => $config::BUILDDIR . '/build',
);
$mime = <<MIME;
application/x-httpd-parrot wxd
text/html html
MIME
    
$server->addFile('mime.types', $mime);
$server->serve('foo.wxd', 'function main[main]() { say("hello world"); }');
$server->start();

Client::setup($server);
# todo, determine the compiler
like(content('foo.wxd'), qr/Loading compiler/);
like(content('foo.wxd'), qr/foo\.wxd/);



done_testing();
