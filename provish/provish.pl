#!/usr/bin/perl
use warnings;
use strict;

use Server;
use Client;
use Data::Dumper;
use Carp;




my $server = Server->new();
$server->start();
$server->serve('index.html', <<INDEX
               <html>
                 <head>
                 </head>
               <body>
               <p>Hello, world</p>
               <body>
               </html>
INDEX
);
my $client = Client->new($server);
$client->is_ok('index.html') or carp('index does not exist');
$client->is_status('foobar.html', 404) or carp('there is a foobar.html');

$server->stop();
