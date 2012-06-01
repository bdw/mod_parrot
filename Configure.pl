#!/usr/bin/perl
use strict;
use warnings;

sub find_alternative {
    my ($names, $directories) = @_;
    for my $d (@$directories) {
        for my $n (@$names) {
            my $f = $d . '/' . $n;
            return $f if -x $f;
        }
    }
}

my $apxs = find_alternative(['apxs','apxs2'], ['/usr/bin', '/usr/sbin']);
my $httpd = find_alternative(['httpd', 'apache2'], ['/usr/bin','/usr/sbin','/sbin']);
my $bash = find_alternative(['sh','bash'], ['/bin','/usr/bin']);

# todo: clean this up into something that doesn't look like a total hack
my %flags = ( 
	'-l' => 'LIBS',
	'-L' => 'LDPATH',
	'-I' => 'INCLUDES'
);

my %info = (
	LIBTOOL => `$apxs -q LIBTOOL`,
	PWD => `pwd`,
    APXS => $apxs,
    HTTPD => $httpd
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
	print $out $key,'=',$info{$key}, "\n";
}



my $makefile = do { local $/ = <$in> };
print $out $makefile;
close $in;
close $out;

chomp $info{PWD};
my $dir = $info{PWD} . '/build';
mkdir $dir unless -d $dir;

open $in, '<', 'httpd.conf.in';
open $out, '>', $dir . '/httpd.conf' or die "Could not write httpd.conf";
my $httpdconf = do { local $/ = <$in> };
$httpdconf =~ s/\@BUILD\@/$dir/g;
print $out $httpdconf;
close $in;
close $out;

open $in, '<', 'start-server.sh.in';
my $starserver = do {  local $/ = <$in> };

open $out, '>', 'start-server.sh';

print $out '#!'.$bash."\n";
for my $key (keys(%info)) {
	print $out $key,'="',$info{$key}, "\"\n";
}
print $out $starserver;
close $in;
close $out;
chmod 0755, 'start-server.sh';

print "Type make to build\n";


