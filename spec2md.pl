#!/usr/bin/env perl
use strict;
use warnings;

my $stage = 0;
my $example = 0;
my @match;
my $section = "";

while (<STDIN>) {
  if (/^\.$/) {
    if ($stage == 0) {
      $example++;
      print "\n<div class=\"example\" id=\"example-$example\" data-section=\"$section\">\n";
      print "<div class=\"examplenum\"><a href=\"#example-$example\">Example $example</a>&nbsp;&nbsp;<a class=\"dingus\" title=\"open in interactive dingus\">(interact)</a></div>\n\n";
      print "````````````````````````````````````````````````````````` markdown\n";
    } elsif ($stage == 1) {
      print "`````````````````````````````````````````````````````````\n\n";
      print "````````````````````````````````````````````````````````` html\n";
    } elsif ($stage == 2) {
      print "`````````````````````````````````````````````````````````\n\n";
      print "</div>\n\n";
    } else {
      die "Encountered unknown stage $stage";
    }
    $stage = ($stage + 1) % 3;
  } else {
    if ($stage == 0 && (@match = ($_ =~ /^#{1,6} *(.*)/))) {
        $section = $match[0];
    }
    if ($stage != 0) {
      $_ =~ s/ /␣/g;
    }
    print $_;
  }
}
