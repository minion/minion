#!/bin/bash

#!/bin/bash
mkdir machines
rm -rf machines/*
initial_path=`pwd`
# life3d 
for i in labs_two labs_three labs_four life pegsol and_constraint;
do
	echo $i
	for j in 1 2 3 4 5 6 7 8 9 10;
	do
		./build_machine.sh $i False False H 7
		grep 'Number of nodes:' machines/$i.False.False.H.7.vm_out
	done
	echo Symmetry
	for j in 1 2 3 4 5 6 7 8 9 10;
	do
		./build_machine.sh $i False True H 7
		grep 'Number of nodes:' machines/$i.False.True.H.7.vm_out
    done
done

# Can also do labs five, no symmetry as long as you don't compile it. 
#./build_machine.sh labs_five True False H 5


# Ones that only work with vm and symmetry
#./build_machine.sh lifeBriansBrain True True H 5


#for i in labs_five labs_six lifeImmigration;
#do
#    ./build_machine.sh $i False True H 5
#    ./build_machine.sh $i True True H 5
#    # This next line to make sure building fails, if it isn't there.
#    rm ../minion/constraints/generated_constraint_code.h
#    cp machines/$i.False.True.H.5.vm_out ../minion/constraints/generated_constraint_code.h
#    ( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; cp minion $initial_path/machines/minion-$i-sym )
#done

