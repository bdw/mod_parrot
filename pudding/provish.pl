#!/usr/bin/perl
use warnings;
use strict;

use Server;
use Client;
use config;
use Data::Dumper;
use Carp;




my $server = Server->new($config::HTTPD);
my $doc = <<INDEX
    <html>
    <head>
    <title>Hello, World</title>
    </head>
    <body>
    <p>Hello, world</p>
    <body>
    <body>
    </html>
INDEX
    ;

$server->start();
$server->serve('index.html', $doc);

my $client = Client->new($server);
$client->is_ok('index.html') or carp('index does not exist');
$client->is_get('index.html', $doc) or carp('doc looks different');
$client->is_status('foobar.html', 404) or carp('there is a foobar.html');
$server->stop();
