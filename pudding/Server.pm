#!/usr/bin/perl
package Server;
use strict;
use warnings;

use File::Copy;
use File::Basename;
use File::Slurp;
use File::Path qw/make_path/;
use File::Temp qw/tempfile tempdir/;
use Data::Dumper;
use Carp;

=head1 Apache as an object.

This class implements a simple interface to apache, so it can be
tested against by a script.

=cut

sub writeconf {
	my %conf = @_;
	open my $out, '>', my $filename = $conf{ServerRoot} . '/httpd.conf';
	local ($\, $,) = ("\n", " ");
	for (qw(ServerName ServerRoot PidFile DocumentRoot ErrorLog Listen)) {
		print $out $_, $conf{$_};
	}	
    if(ref (my $modules = $conf{LoadModule}) eq 'HASH') {
        print $out 'LoadModule', $_, $conf{LoadModule}->{$_} for keys(%$modules);
    }

    if(ref (my $options = $conf{Options}) eq 'HASH') {
        print $out $_, $conf{Options}->{$_} for keys(%$options);
    }
    print $out $conf{RawOptions} if (defined $conf{RawOptions});
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
		ServerName => 'nifftuk.dev',
        ServerRoot => $directory,
        PidFile => $directory . '/httpd.pid',
        DocumentRoot => $directory . '/docs',
        ErrorLog => $directory . '/error.log',
        Listen => 8000 + @servers,
    }, $class;
}


=head2 Load a module 

=cut

sub loadModule {
    my ($self, %opts) = @_;
    $self->{LoadModule} = \%opts;
    $self->restart() if $self->{Pid};
}

=head2 Configure the web server
    
Restart the web server if needed

=cut

sub configure {
    my $self = shift;
    if(@_ > 1) {
        my %opts = @_;
        $self->{Options}->{$_} = $opts{$_} for keys (%opts);
    } else {
        my $raw = shift;
        $self->{RawOptions} = $raw;
    }
    $self->restart() if $self->{Pid};
}

sub addFile {
    my ($self, $name, $content) = @_;
    write_file($self->{ServerRoot} . '/' . $name, $content);
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
	$self->{Pid} = startprocess($self);	
    push @servers, $self;
}

=head2 Serve a file (allowing it to be tested)

Pass it either the name of an existing file or a new file name with
supplied contents. The served file will be placed in DocumentRoot.

=cut

sub serve {
	my ($self, $file, $contents, $mode) = @_;
	$mode ||= 0644;
	my $new = $self->{DocumentRoot}  . '/' . $file;
	# ensure the availability of a document root
    make_path($self->{DocumentRoot}) unless -d $self->{DocumentRoot};

    if ( -f $file) {
        copy($file, $new);
    } elsif(defined $contents) {
        write_file($new, $contents);
    }
	chmod $mode, $new;
}

=head2 Debug a server using gdb

=cut

sub debug {
    my $self = shift;
	make_path($self->{DocumentRoot}) unless -d $self->{DocumentRoot};
    $self->{File} = writeconf(%$self);
    exec("gdb", "--args", $self->{Process}, "-d", $self->{ServerRoot},
         "-f", $self->{File}, "-X");
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
        kill "SIGQUIT", $_->{Pid};
        print $_->errors if $ENV{VERBOSE};
    }
}

1;
