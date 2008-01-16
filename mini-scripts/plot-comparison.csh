#! /bin/csh

echo $1 $2

set tmpdir = $HOME/tmp

if ($3 == "") set dir = "."
if ($3 != "") set dir = $3 
echo $dir


# perl analyse_minion.pl $1/* > /tmp/$1.$$
# perl analyse_minion.pl $2/* > /tmp/$2.$$

analyse_minion.pl $1/* > $tmpdir/$1.$$
analyse_minion.pl $2/* > $tmpdir/$2.$$


if (! -e $dir) mkdir $dir

join $tmpdir/$1.$$ $tmpdir/$2.$$ > $dir/comparison.$1.$2.txt
rm $tmpdir/$1.$$
rm $tmpdir/$2.$$


gnuplot << EOF
set title "Comparison of $1 with $2"
set term png
set output "$dir/comparison.$1.$2.png"
set log x
set ytics (0.001,0.01,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.95,0.99,1,1.01,1.05,1.1,1.2,1.3,1.4,1.5,1.6,1.7,1.8,1.9,2,3,4,5,10,100,1000)
set log y
set autoscale yfix
set xrange [0.1:*]
set xlabel "run time of $2 in secs"
set ylabel "%age improvement  in $1 from $2 "
set grid
set size 1.5,1.2
set pointsize 2
set autoscale yfix
set key outside box 
plot "$dir/comparison.$1.$2.txt" using 19:((\$7/\$21-1)*100) t "overall nodes/s", \
     "$dir/comparison.$1.$2.txt" using 19:((\$13/\$27-1)*100) t "search nodes/s", \
     "$dir/comparison.$1.$2.txt" using 19:((\$6/\$20-1)*100) t "total nodes" 
EOF

set prefixes = `cat $dir/comparison.$1.$2.txt | awk -F'[ 0-9]' '{print $1}' | sort | uniq` 

echo $prefixes

cat << EOF > $tmpdir/$1.$2.$$.gnu
set title "Comparison of $1 with $2"
set term png
set output "$dir/breakdown.$1.$2.png"
set log x
set log y
set xrange [0.1:*]
set xlabel "run time of $2 in secs"
set ytics (0.001,0.01,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.95,0.99,1,1.01,1.05,1.1,1.2,1.3,1.4,1.5,1.6,1.7,1.8,1.9,2,3,4,5,10,100,1000)
set autoscale yfix
set ylabel "%age improvement  in $1 from $2 "
set grid
set size 1.5,1.2
set pointsize 2
set key outside box 
plot 1 not, \
EOF

foreach i ($prefixes)
     egrep "^${i}[ 0-9]" $dir/comparison.$1.$2.txt > $tmpdir/$1.$2.$$.$i
     cat << EOF >> $tmpdir/$1.$2.$$.gnu
"$tmpdir/$1.$2.$$.$i" using 19:((\$7/\$21)) t "$i", \
EOF
     end

echo "1 not" >> $tmpdir/$1.$2.$$.gnu 
gnuplot < $tmpdir/$1.$2.$$.gnu

rm $tmpdir/$1.$2.$$.*

