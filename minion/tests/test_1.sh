j=0
pass=0
for i in test*.cpp; do
j=$(($j + 1))
echo Test $i
g++ $i -O2 -DNO_PRINT -DNO_PRINT_SOLUTIONS -DWATCHEDLITERALS -o $i.exe -I.. 
if ./$i.exe &> /dev/null;
then pass=$(($pass + 1));
else
echo Fail $i;
fi
rm $i.exe
done
echo $pass of $j tests successful.
