#! /usr/bin/perl

#Script written by Ian Gent, 10/10/2006, based on ... 
#Script written by Ian Gent, 26/8/99

#use Descriptive;

if (@ARGV < 1) 
{ 
  print "usage: analyse_minion.pl {--headers} filename*\n" ;
  print "or:    perl analyse_minion.pl ...\n" ;
  print "    analyses minion results in the given filenames,\n";
  print "    gives out one line of data per result\n";
  print " --headers (optional) prints out one word summary of columns in order\n";
}


FILELOOP:
for($arg=0;$arg < @ARGV; $arg++) {


  if ($arg==0 and $ARGV[$arg] == "--headers") {print_headers();}
  else 
      {

      $filename = $ARGV[$arg];

      # my $field,$res,$resbin,$chafftime,$depth,$br,$cl,$lits,$imps;
      # my $convtime,$chafftime,$elapsedtime,$cpupercent;
      # my $n, $m, $p, $c;

      $res = "";

      open(FILENAME,$filename)
       || die "can't open $filename/n";

      $filename =~ /([0-9]*)\./ ;
      my $id = $1;
      $resbin = 0 ; 

      while (defined($line = <FILENAME>))
            {
              
              $line =~ s/^\s*//;
              @field = split(/\s+/,$line);
              next FILELOOP if ($line =~ /Segmentation\s*fault/);
              if ($line =~ /^# Minion Version/) 
              { 
                  print_line();    

              # initialisation for this instance
                  $resbin = 0;
                  $res = "Unknown";
                  $timeout = "Completed";

                  $version = $field[3] ;
              } 
              if ($line =~ /^# Input filename/) { $instance_name =  $field[3] ; }
              if ($line =~ /^Time out/) { $timeout = "Timeout"; } 
              if ($line =~ /^Problem solvable.*yes/) 
              { 
                  $res = "Solvable" ;
                  $resbin = 1;
              } 
              if ($line =~ /^Problem solvable.*no/) 
              { 
                  $res = "Unsolvable" ;
                  $resbin = 0;
              } 
              #if ($line =~ /^Parsing Time=/) { @heuristic =  split(/,/,$field[2]); }
              if ($line =~ /^Parsing Time/) { @parsing_time =  $field[2] ; }
              if ($line =~ /^Setup Time/) { @setup_time =  $field[2] ; }
              if ($line =~ /^First node time/) { @first_node_time =  $field[3] ; }
              if ($line =~ /^Solve Time/) { @solve_time =  $field[2] ; }
              if ($line =~ /^Total Time/) { $total_time =  $field[2] ; }
              if ($line =~ /^Bytes used in Backtrackable Memory/) { @bytes_used = $field[6] ; }
              if ($line =~ /^Total Nodes/) { $total_nodes =  $field[2]; }
              if ($line =~ /^Solutions Found/) { $num_solutions =  $field[2]; }
            }

      close(FILENAME);
      print_line();
      }
  }

exit;


sub print_line
{
 
  my $nodesper, $nodesper_solving;
  $nodesper = 0 if ($total_time == 0);
  $nodesper_solving = 0 if ($solve_time ==0);
  $nodesper = ($total_nodes/$total_time) unless ($total_time==0);
  $nodesper_solving = ($total_nodes/$solving_time) unless ($solve_time==0);

  #if changing this remember to update print_headers!
  print "$instance_name $version $res $num_solutions $total_time $total_nodes $nodesper $timeout @parsing_time @setup_time @first_node_time @solve_time $nodesper_solving $filename \n" 
    if (!($res eq ""));
}  

sub print_headers
{
  print "instance_filename minion_version solvable_or_unsolvable num_solutions total_time total_nodes nodes_per_sec completed_or_timeout parsing_time setup_time first_node_time solve_time nodes_per_sec_solving data_filename\n" ;
}  
