#!/usr/bin/perl
use strict;
use warnings;
use Carp;
use Cwd;
use Data::Dumper;
=head1 Configure building mod_parrot

This script makes:
module/config.h
module/config.mk
pudding/config.pm

=cut

sub find_alternative {
    my ($names, $directories) = @_;
    for my $d (@$directories) {
        for my $n (@$names) {
            my $f = $d . '/' . $n;
            return $f if -x $f;
        }
    }
}

sub write_definitions {
	my ($filename, $format, $values, $prefix, $postfix) = @_;
	open my $out, '>', $filename or croak 'Could not open '. $filename;
    print $out "$prefix\n" if defined $prefix;
	printf $out "$format\n", $_, $values->{$_} for (keys %$values);
    print $out "$postfix\n" if defined $postfix;
	close $out;
}



my @PATH = split(/:/, $ENV{PATH});
my $apxs = find_alternative(['apxs','apxs2'], [@PATH, '/usr/sbin']);
my $httpd = find_alternative(['httpd', 'apache2'], [@PATH, '/usr/local/sbin','/usr/sbin','/sbin']);
my $bash = find_alternative(['sh','bash'], \@PATH);
my $parrot_config = find_alternative(['parrot_config'], \@PATH);

# write the configuration header file
my %config = (
	LIBDIR => `parrot_config libdir`,
	VERSIONDIR => `parrot_config versiondir`,
	BUILDDIR => `parrot_config build_dir`,
	INSTALLDIR => `$apxs -q LIBEXECDIR`,
);

chomp $config{$_} for (keys %config);
write_definitions('module/config.h', '#define %s "%s"', \%config);

my %make = (
    LIBTOOL => qx/$apxs -q LIBTOOL/,
	BUILDDIR => getcwd(),
    APXS => $apxs,
    HTTPD => $httpd,
	FLAGS => qx/parrot_config embed-ldflags/ . ' ' . qx/parrot_config embed-cflags/,
);
chomp $make{$_} for (keys(%make));

$make{FLAGS} =~ s/(-[lLI])(\S+)/$1 $2/g;
$make{FLAGS} =~ s/\s+/ /g;
write_definitions('config.mk', '%s=%s', \%make);
write_definitions('pudding/config.pm', '$config::%s="%s";', \%make, 'package config;', '1;');
mkdir('build') unless -d 'build';
