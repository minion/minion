#! /usr/bin/perl

use Getopt::Long;

my $homedir = $ENV{HOME};	# probably not portable!
my $bindir = "$homedir/bin-minion-mac";
my $benchdir = "$homedir/myminion/MinionBenchmarks";
my $exe = "";
my $exedefault = "minion";
my $timelimit = 1200;
my $help = "";
my $resultdir ;
my $filelist = "regression.filelist";

GetOptions(
	    'filelist=s' => \$filelist,
            'timelimit=i' => \$timelimit,
            'minion=s' => \$exe,
            'instancedir=s' => \$benchdir,
            'bindir=s' => \$bindir,
	    'help' => \$help
	  );

if (@ARGV < 1 or $help)
{ 
   print "usage: (not finished, sorry) \n";
   exit;
}

$resultdir = $ARGV[0];

if (! $exe) {$exe = $resultdir;} 
if ($bindir) {$minionexe = "$bindir/$exe";}
else         {$minionexe = $exe;}


if (! -e $resultdir) {mkdir( $resultdir );}


if (! -d $resultdir) 
{ 
  print "Sorry, $resultdir is not a directory"; 
  exit;
}

open DATA, "< $filelist";

while (<DATA>)
{
  if (not ( /^\s*\#/ or /^\s*$/))	# nothing to see here, move along
  {
# following should match: 
# whitespace non-whitespace=$1 whitespace command-options-if-any=$2 #ignore anything after # character

       if ( /^\s*(\S+)\s*(.*?)(\#.*)?$/) { $file = $1 ; $options = $2; }
       # ($command, $options) = split /\s+/, $_, 2; 
       print "$minionexe -timelimit $timelimit $options $benchdir/$file\n";
  }
}

exit;

 print "xgrid -job run -se $1/tmp.$i.err -so $1/$i.out $minionexe -timelimit $timelimit $benchdir/$i.minion ";
