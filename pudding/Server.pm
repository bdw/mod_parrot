#!/usr/bin/perl
package Server;
use strict;
use warnings;

use File::Copy;
use File::Basename;
use File::Slurp;
use File::Path qw/make_path/;
use File::Temp qw/tempfile tempdir/;

=head1 Apache as an object.

This class implements a simple interface to apache, so it can be
tested against by script.

=cut

sub writeconf {
	my %conf = @_;
	open my $out, '>', my $filename = $conf{ServerRoot} . '/httpd.conf';
	local ($\, $,) = ("\n", " ");
	for (qw(ServerRoot PidFile DocumentRoot ErrorLog Listen)) {
		print $out $_, $conf{$_};
	}	
    
	close $out;
	return $filename;
}

sub startprocess {
    my $self = shift;
    exec($self->{Process}, '-d', $self->{ServerRoot}, '-f', $self->{File}, '-X') 
        unless my $pid = fork();
    sleep(1); return $pid;
}

=head2 Initialize the web server

Takes the ServerRoot directory as its single optional argument.  If
none is given the current working directory is assumed.  From the
directory, the DocumentRoot, PidFile and ErrorLog arguments are also
assumed.

TODO: Add the executable name to the argument. It is important.
=cut

our @servers;

sub new {
    my ($class, $process, $directory) = @_;
    $process ||= 'httpd';
    $directory  ||= tempdir();
    bless {
        Process => $process,
        ServerRoot => $directory,
        PidFile => $directory . '/httpd.pid',
        DocumentRoot => $directory . '/docs',
        ErrorLog => $directory . '/error.log',
        Listen => 8000,
    }, $class;
}

=head2 Configure the web server
    
Restart the web server if needed

=cut

sub configure {
    my ($self, %opts) = @_;
    $self->{$_} = $opts{$_} for keys (%opts);
    $self->restart() if($self->{Pid})
}


=head2 Start the web server.

Starts the web server, creating the paths and configuration files if
nececcary. It uses fork() and exec() so I will not be a bit surprised
if it fails on windows.

=cut

sub start {
    my $self = shift;
	make_path($self->{DocumentRoot}) unless -d $self->{DocumentRoot};
	$self->{File} = writeconf(%$self);
	push @servers, $self->{Pid} = startprocess($self);	
}

=head2 Serve a file (allowing it to be tested)

Pass it either the name of an existing file or a new file name with
supplied contents. The served file will be placed in DocumentRoot.

=cut

sub serve {
	my ($self, $file, $contents) = @_;
    if ( -f $file) {
        copy($file, $self->{DocumentRoot});
    } elsif(defined $contents) {
        write_file($self->{DocumentRoot} . '/' . basename($file), $contents);
    }
}



=head2 Stop the web server.

Make sure to call this at the end of your script or apache will be a
zombie. TODO: add this to the END{} block, probably.

=cut

sub stop {
	my $self = shift;
	kill "SIGQUIT", $self->{Pid};
    unlink $self->{PidFile} if -f $self->{PidFile};	
    wait;
}

=head2 Restart the web server

=cut

sub restart {
    my $self = shift;
    $self->stop();
    $self->start();
}

=head2 Get the error log

Read the error log nas lines.

=cut
sub errors {
	my $self = shift;
    return read_file($self->{ErrorLog});
}


END {
    for (@servers) {
        kill "SIGQUIT", $_;
    }
}

1;
