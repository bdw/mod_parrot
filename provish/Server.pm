#!/usr/bin/perl
use strict;
use warnings;
package Server;
use File::Copy;
use Cwd;
use File::Path qw/make_path/;
use Slurp;

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
	my %conf = @_;
	print $conf{ServerRoot}, "\n";
	qx/httpd -d $conf{ServerRoot}-f $conf{File} -X/;
	Slurp::to_scalar($conf{PidFile});
}

=head2 Start the web server, returning the instance

=cut

sub start {
    my ($class, %conf) = @_;
	$conf{ServerRoot} ||= getcwd();
	$conf{PidFile} ||= $conf{ServerRoot} . '/httpd.pid';
	$conf{DocumentRoot} ||= $conf{ServerRoot} . '/docs'; 
	$conf{ErrorLog} ||= $conf{ServerRoot} . '/error.log';
	$conf{Listen} ||= 8000;
	make_path($conf{DocumentRoot}) unless -d $conf{DocumentRoot};
	$conf{File} = writeconf(%conf);
	$conf{Pid} = startprocess(%conf);	
	bless \%conf, $class;
}


sub serve {
	my ($self, $file) = @_;
	copy($file, $self->{DocumentRoot});
}

sub stop {
	my $self = shift;
	kill "SIGQUIT", $self->{Pid};
	unlink $self->{PidFile};	
}

sub errors {
	my $self = shift;
	open my $log, '<', $self->{ErrorLog};
	my @errors = <$log>;
	close $log;
	return @errors;
}

1;
