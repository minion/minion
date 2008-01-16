#! /bin/csh

echo $1 $2

if ($3 == "") set dir = "."
if ($3 != "") set dir = $3 
echo $dir


perl analyse_minion.pl $1/* > /tmp/$1.$$
perl analyse_minion.pl $2/* > /tmp/$2.$$

if (! -e $dir) mkdir $dir

join /tmp/$1.$$ /tmp/$2.$$ > $dir/comparison.$1.$2.txt
rm /tmp/$1.$$
rm /tmp/$2.$$


gnuplot << EOF
set title "Comparison of $1 with $2"
set term png
set output "$dir/comparison.$1.$2.png"
set log x
set xlabel "run time of $2 in secs"
set ylabel "percentage improvement in $1 in nodes per sec"
set grid
unset key
plot "comparison.$1.$2.txt" using 18:((\$7/\$20-1)*100)
EOF

