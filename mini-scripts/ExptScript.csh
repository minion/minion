#! /bin/csh

if ($3 == "") then
  echo "usage: ExptScript.csh InstanceDir ResultsDir ComparisonResultsDir {Minion}" 
  echo "  output graph and txt file goes to ResultsDir"
  echo "  optional final argument is minion executable (defaults to 'minion') "
  exit
endif

set dir = $2
set instancedir = $1
set comparisondir = $3

if ($4 == "") then
  set executable = minion
else
  set executable = $4
endif

if (! -e $dir ) mkdir $dir 
foreach i (ecaiPresentationBibdLine01 ecaiPresentationBibdLine02 ecaiPresentationBibdLine03 ecaiPresentationBibdLine04 ecaiPresentationBibdLine05 ecaiPresentationBibdLine06 ecaiPresentationBibdLine07 ecaiPresentationBibdLine08 ecaiPresentationBibdLine09 ecaiPresentationBibdLine10 ecaiPresentationBibdLine11 ecaiPresentationBibdLine12 ecaiPresentationBibdLine13 ecaiPresentationBibdLine14 ecaiPresentationBibdLine15 ecaiPresentationBibdLine16 ecaiPresentationBibdLine17 ecaiPresentationBibdLine18 ecaiPresentationBibdLine19 ecaiPresentationBibdLine20 ecaiPresentationBibdLine21 golomb.7.noimplied golomb.8.noimplied golomb.9.noimplied golomb.10.noimplied golomb.11.noimplied golomb.12.noimplied solitaire_benchmark_1 solitaire_benchmark_2 solitaire_benchmark_3 solitaire_benchmark_4 solitaire_benchmark_5 solitaire_benchmark_6 solitaire_benchmark_7 solitaire_benchmark_8)

   echo $i >> $dir/$i.Current.out
   $executable -noprintsols $instancedir/$i.minion >> $dir/$i.Current.out

   end
  
	
foreach i (steelmill_instance100 steelmill_instance40 steelmill_instance50 steelmill_instance60 steelmill_instance70 steelmill_instance80 steelmill_instance90)

 echo $i >> $dir/$i.Current.out
   $executable -timelimit 600 -noprintsols $instancedir/$i.minion >> $dir/$i.Current.out

   end
  
	
	
# And to do SAT QG experiments

foreach i (qg1-07 qg1-08 qg2-07 qg2-08 qg3-08 qg3-09 qg4-08 qg4-09 qg5-09 qg5-10 qg5-11 qg5-12 qg5-13 qg6-09 qg6-10 qg6-11 qg6-12 qg7-09 qg7-10 qg7-11 qg7-12 qg7-13)  
   echo $i >> $dir/$i.Current.out
   $executable -noprintsols $instancedir/$i.minion >> $dir/$i.Current.out
   end
  
	
# Then run the comparison 

./plot-comparison.csh $instancedir $comparisondir $instancedir

