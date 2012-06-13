#!/usr/bin/perl
use strict;
use warnings;
package Client;
use HTTP::Tiny;
use Server;

=head1 Testing request againts output

TODO: this should have a procedural (or functional, even) interface.


=cut

sub new {
	my ($class, $server) = @_;
    bless {
        host => $server->{Hostname} || 'localhost',
        port => $server->{Listen} || 80,
        http => HTTP::Tiny->new(),
    }, $class;
}	

sub url {
    sprintf('http://%s:%s/%s', $_[0]->{host}, $_[0]->{port}, $_[1]);
}

sub is_get {
	my ($self, $uri, $exp) = @_;
    print $self->{http}->get(url(@_))->{content};
}

sub is_ok {
    my ($self, $uri) = @_;
    $self->{http}->get(url(@_))->{status} == 200;
}

sub is_status {
    my ($self, $uri, $status) = @_;
    $self->{http}->get(url(@_))->{status} == $status;
}

1;
