#!/usr/bin/perl
use strict;
use warnings;
use Carp;
use Cwd;
use Data::Dumper;
=head1 Configure building mod_parrot

This script makes:
src/module/config.h
src/module/config.mk
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
	BUILD_DIR => `parrot_config build_dir`,
);

chomp $config{$_} for (keys %config);
write_definitions('src/module/config.h', '#define %s "%s"', \%config);

my %make = (
    LIBTOOL => qx/$apxs -q LIBTOOL/,
	BUILDDIR => getcwd(),
    APXS => $apxs,
    HTTPD => $httpd
);
chomp $make{$_} for (keys(%make));

my %map = ( -l => 'LIB', -L => 'LDPATH', -I => 'INC');
my $flags = qx/parrot_config embed-ldflags/ . ' ' . qx/parrot_config embed-cflags/;
for (split /\s+/, $flags) {
    $make{$map{substr $_, 0, 2}} .= substr($_, 0, 2) . ' ' . substr($_, 2) . ' ';
}
write_definitions('src/module/config.mk', '%s=%s', \%make);

write_definitions('pudding/config.pm', '$config::%s="%s";', \%make, 'package config;', '1;');
