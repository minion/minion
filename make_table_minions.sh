make veryclean
nice make minion MYFLAGS="-DOLDTABLE" -j 2 $* 
cp bin/minion* bin/min-original
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DNIGHTINGALE" -j 2 $*
cp bin/minion* bin/min-night
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DNIGHTINGALE -DLISTPERLIT" -j 2 $*
cp bin/minion* bin/min-night-listperlit
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DTRIES" -j 2 $*
cp bin/minion* bin/min-tries
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DREGINLHOMME" -j 2 $*
cp bin/minion* bin/min-regin
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DBINARY_SEARCH" -j 2 $*
cp bin/minion* bin/min-lecoutre
rm bin/minion*

