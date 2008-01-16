make veryclean
nice make minion -j 2 $*
cp bin/minion* bin/min-original
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DNIGHTINGALE" -j 2 $*
cp bin/minion* bin/min-night
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DTRIES" -j 2 $*
cp bin/minion* bin/min-tries
rm bin/minion*
find bin/* | grep TABLE | xargs rm
make minion MYFLAGS="-DREGINLHOMME" -j 2 $*
cp bin/minion* bin/min-regin
rm bin/minion*
