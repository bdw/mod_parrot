#!/usr/bin/perl
use strict;
use warnings;
package Client;
use Exporter 'import';
use HTTP::Tiny;
use Server;

our @EXPORT = qw(content status headers response);


our $server;
our $client;
sub setup {
	$server = shift;
    $client = HTTP::Tiny->new();
}	

sub url {
    sprintf('http://%s:%s/%s', $server->{Hostname} || 'localhost', $server->{Listen} || 80, shift);
}

sub content {
    $client->get(url(shift))->{content};
}

sub status {
    $client->get(url(shift))->{status};
}

sub headers {
    $client->get(url(shift))->{headers};
}

sub response {
    $client->get(url(shift));
}

1;
