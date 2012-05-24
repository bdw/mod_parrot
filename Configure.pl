#!/usr/bin/perl
use strict;
use warnings;
	
# todo: clean this up into something that doesn't look like a total hack
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
# write the output file
open my $in, '<', 'Makefile.in';
open my $out, '>', 'Makefile'; 
for my $flag (keys(%flags)) {
	print $out $flags{$flag},'=',join(' ', map { $flag . ' ' . $_ } @{$split{$flag}}), "\n";
}

for my $key (keys(%info)) {
	print $out $key,'=',$info{$key};
}

my $makefile = do { local $/ = <$in> };
print $out $makefile;
close $in;
close $out;

chomp $info{PWD};
my $dir = $info{PWD} . '/build';
mkdir $dir unless -d $dir;

open $in, '<', 'httpd.conf.in';
open $out, '>', $dir . '/httpd.conf';
my $httpdconf = do { local $/ = <$in> };

$httpdconf =~ s/\@BUILD\@/$dir/g;
print $out $httpdconf;
close $in;
close $out;

print "Type make to build\n";


