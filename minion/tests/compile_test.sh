g++ $1 -O2 -DNO_PRINT -o $1.exe -I..
if ./$1.exe &> /dev/null;
then p
./$1.exe1
rm $1.exe