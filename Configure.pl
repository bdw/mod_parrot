#!/usr/bin/perl
use strict;
use warnings;
	

my %flags = ( 
	'-l' => 'LIBS',
	'-L' => 'LDPATH',
	'-I' => 'INCLUDES'
);

my %info = (
	LIBTOOL => `apxs -q LIBTOOL`,
	PWD => `pwd`
);

my $cflags = `parrot_config embed-cflags`;
my $ldflags = `parrot_config embed-ldflags`;
my @input = split (/\s+/, $cflags.' '.$ldflags);
my %split = map {
	my $flag = $_; 
	$flag => [ map { substr $_, 2 } grep m/^$flag/, @input ] 
} keys(%flags);

open my $in, '<', 'Makefile.in';
open my $out, '>', 'Makefile'; 
for my $flag (keys(%flags)) {
	print $out $flags{$flag},'=',join(' ', map { $flag . ' ' . $_ } @{$split{$flag}}), "\n";
}
my $makefile = do { local $/ = <$in> };
for my $key (keys(%info)) {
	print $out $key,'=',$info{$key};
}

print $out $makefile;

close $in;
close $out;
print "Type make to build\n";


