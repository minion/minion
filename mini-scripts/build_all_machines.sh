#!/bin/bash
mkdir machines
rm -rf machines/*
initial_path=`pwd`
# life3d 
for i in labs_two labs_three labs_four life pegsol and_constraint;
do
    ./build_machine.sh $i False False H 5
    ./build_machine.sh $i False True H 5
    ./build_machine.sh $i True False H 5
    ./build_machine.sh $i True True H 5
    # This next line to make sure building fails, if it isn't there.
    rm ../minion/constraints/generated_constraint_code.h
    cp machines/$i.False.False.vm_out ../minion/constraints/generated_constraint_code.h
    ( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; cp minion $initial_path/machines/minion-$i-nosym )
    cp machines/$i.False.True.vm_out ../minion/constraints/generated_constraint_code.h
    ( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; cp minion $initial_path/machines/minion-$i-sym )
done

# Can also do labs five, no symmetry as long as you don't compile it. 
./build_machine.sh labs_five True False H 5

for i in labs_five labs_six lifeImmigration lifeBriansBrain;
do
    ./build_machine.sh $i False True H 5
    ./build_machine.sh $i True True H 5
    # This next line to make sure building fails, if it isn't there.
    rm ../minion/constraints/generated_constraint_code.h
    cp machines/$i.False.True.vm_out ../minion/constraints/generated_constraint_code.h
    ( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; cp minion $initial_path/machines/minion-$i-sym )
done

