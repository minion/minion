#! /usr/bin/perl

use Getopt::Long;

my $homedir = $ENV{HOME};	# probably not portable!
my $bindir = "$homedir/bin-minion-mac";
my $benchdir = "$homedir/myminion/MinionBenchmarks";
my $exe = "";
my $exedefault = "minion";
my $timelimit = 1200;
my $interval = 5;
my $help = "";
my $resultdir ;
my $filelist = "regression.filelist";
my $xgrid = "";

GetOptions(
	    'filelist=s' => \$filelist,
            'timelimit=i' => \$timelimit,
            'minion=s' => \$exe,
            'instancedir=s' => \$benchdir,
            'bindir=s' => \$bindir,
            'xgrid!' => \$xgrid,
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

print "\#! /bin/sh\n";
while (<DATA>)
{
  if (not ( /^\s*\#/ or /^\s*$/))	# nothing to see here, move along
  {
# following should match: 
# whitespace non-whitespace=$1 whitespace command-options-if-any=$2 #ignore anything after # character

       if ( /^\s*(\S+)\s*(.*?)(\#.*)?$/) { $file = $1 ; $options = $2; }
       $command =  "$minionexe -timelimit $timelimit $options $benchdir/$file";
       if ($xgrid) 
       { print "xgrid -job run -se $resultdir/tmperr.$file.\$\$ -so $resultdir/out.$file.\$\$ $command &\n";
         print "sleep $interval\n";
       }
       else 
       { print "$command\n";}
  }
}

exit;

