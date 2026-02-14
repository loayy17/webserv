#!/usr/bin/perl
use strict;
use warnings;

my $body = do { local $/; <STDIN> };

print "Content-Type: text/html\r\n\r\n";
print "<h1>Perl CGI OK</h1>";
print "<pre>$body</pre>";
