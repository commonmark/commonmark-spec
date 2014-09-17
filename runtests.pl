#!/usr/bin/env perl
use warnings;
use strict;
use Term::ANSIColor;
use IO::Handle;
use IPC::Open2;

my $usage="runtests.pl SPEC PROGRAM\nSet ANSI_COLORS_DISABLED=1 if you redirect to a file.\nSet PATT='...' to restrict tests to sections matching a regex.\n";

my $SPEC = shift @ARGV;
my @PROG = @ARGV;
my $PATT=$ENV{'PATT'};

if (!(@PROG && defined $SPEC)) {
  print STDERR $usage;
  exit 1;
}

my $passed = 0;
my $failed = 0;
my $skipped = 0;

# Markdown implementations vary on insignificant whitespace.
# Some leave blanks between block elements, others don't.
# This function tries to normalize the output so it can be
# compared with our test.  tidy takes two arguments: the
# string containing the actual output, and a pathname of the
# file to which the tidied output is to be saved.
sub tidy
{
  my $inpre = 0;
  my $out = "";
  my $outfh;
  open($outfh, '>', \$out);
  for (split /^/, $_[0]) {
    if (/<pre/) {
      $inpre = 1;
    } elsif (/<\/pre/) {
      $inpre = 0;
    }
    # remove \r to allow mixing linux/windows newlines
	  s/\r//;
	  if ($inpre) {
      print $outfh $_;
    } else {
      # remove leading spaces
      s/^  *//;
      # remove trailing spaces
      s/  *$//;
      # collapse consecutive spaces
      s/  */ /;
      # collapse space before /> in tag
      s/  *\/>/\/>/;
	  s/>\n$/>/;
      # skip blank line
      if (/^$/) {
        next;
      }
      print $outfh $_;
    }
  }
  close $outfh;
  return $out;
}

sub dotest
{
  my $markdown = $_[0];
  my $html = $_[1];
  my $testname = $_[2];
  my $actual = "";
  # We use → to indicate tab and ␣ space in the spec
  $markdown =~ s/→/\t/g;s/␣/ /g;
  $html =~ s/→/\t/g;s/␣/ /g;
  my $pid = open2(my $out, my $in, @PROG);
  print $in $markdown;
  close $in;
  flush $out;
  $actual = do { local $/; <$out>; };
  close $out;
  waitpid($pid, 0);
  $html   = &tidy($html);
  $actual = &tidy($actual);
  $actual =~ s/\&#39;/'/;

  if ($actual eq $html) {
    print colored("✓", "green");
    return 1;
  } else {
    print colored("\n✘ $testname", "red");
    print "\n";
    print color "cyan";
    print "=== markdown ===============\n";
    print $markdown;
    print "=== expected ===============\n";
    print $html;
	print "\n";
    print "=== got ====================\n";
    print $actual;
	print "\n";
    print color "black";
    return 0;
  }
}

my $stage = 0;
my $markdown = "";
my $html = "";
my $example = 0;
my $linenum = 0;
my $exampleline = 0;
my @secnums = ();
my $secheading;

open(SPEC, "< $SPEC");
while (<SPEC>) {
  $linenum++;
  if (/^\.$/) {
      $stage = ($stage + 1) % 3;
      if ($stage == 1) {
          $exampleline = $linenum;
      }
      if ($stage == 0) {
          $example++;
          if (!$PATT || $secheading =~ /$PATT/) {
            if (&dotest($markdown, $html,
                        "Example $example (line $exampleline)")) {
                $passed++;
            } else {
                $failed++;
            }
          } else {
            $skipped++;
          }
          $markdown = "";
          $html = "";
      }
  } elsif ($stage == 0 && $_ =~ /^<!-- END TESTS -->/) {
      last;
  } elsif ($stage == 0 && $_ =~ /^(#+) +(.*)/) {
      my $seclevel = length($1);
      $secheading = $2;
      if ($#secnums == $seclevel - 1) {
          $secnums[$#secnums]++;
      } elsif ($#secnums > $seclevel - 1) {
          @secnums = @secnums[0..($seclevel - 1)];
          $secnums[$#secnums]++;
      } else {
          while ($#secnums < $seclevel - 1) {
              push(@secnums, 1);
          }
      }
      if (!$PATT || $secheading =~ /$PATT/) {
        print ("\n", join(".", @secnums) . " " . $secheading, "  ");
      }
  } elsif ($stage == 1) {
      $markdown .= $_;
  } elsif ($stage == 2) {
      $html .= $_;
  }
}

print "\n";
print STDERR colored("$passed tests passed, $failed failed, $skipped skipped.", "bold");
print STDERR "\n";
exit $failed;
