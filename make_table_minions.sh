nice make minion -j 2
cp bin/minion bin/min-old
rm bin/objdir-minion/build_constraints/*TABLE*
make minion MYFLAGS="-DNIGHTINGALE" -j 2
cp bin/minion bin/min-night
rm bin/objdir-minion/build_constraints/*TABLE*
make minion MYFLAGS="-DTRIES" -j 2
cp bin/minion bin/min-tries
rm bin/objdir-minion/build_constraints/*TABLE*
#make minion MYFLAGS="-DREGINLHOMME"
#cp bin/minion bin/min-regin


