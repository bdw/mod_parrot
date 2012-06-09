#!/usr/bin/perl
use warnings;
use strict;

use Server;
use Client;
use Cwd qw(getcwd abs_path);
use File::Path;

my $server = Server->start(ServerRoot => (getcwd() . '/../serve/'));
$server->stop();
