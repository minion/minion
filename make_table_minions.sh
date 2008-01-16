nice make minion -j 2
cp bin/minion bin/min-old
rm bin/objdir-minion/BuildDynamicConstraint_GACTable.o
make minion MYFLAGS="-DNIGHTINGALE"
cp bin/minion bin/min-night
rm bin/objdir-minion/BuildDynamicConstraint_GACTable.o
make minion MYFLAGS="-DTRIES"
cp bin/minion bin/min-tries
#rm bin/objdir-minion/BuildDynamicConstraint_GACTable.o
#make minion MYFLAGS="-DREGINLHOMME"
#cp bin/minion bin/min-regin


