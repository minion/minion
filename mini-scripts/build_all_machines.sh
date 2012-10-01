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
    cp machines/$i.False.False.H.5.vm_out ../minion/constraints/generated_constraint_code.h
    ( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; touch ../minion/constraints/generated_constraint_code.h; time (make minion 2>&1) &> $initial_path/machines/$i-nosym.minion_time; cp minion $initial_path/machines/minion-$i-nosym )
    cp machines/$i.False.True.H.5.vm_out ../minion/constraints/generated_constraint_code.h
    ( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; touch ../minion/constraints/generated_constraint_code.h; time (make minion 2>&1) &> $initial_path/machines/$i-sym.minion_time; cp minion $initial_path/machines/minion-$i-sym )
done

# Can also do labs five, no symmetry as long as you don't compile it. 
./build_machine.sh labs_five True False H 5


# Ones that only work with vm and symmetry
./build_machine.sh lifeBriansBrain True True H 5


for i in labs_five labs_six lifeImmigration;
do
    ./build_machine.sh $i False True H 5
    ./build_machine.sh $i True True H 5
    # This next line to make sure building fails, if it isn't there.
    rm ../minion/constraints/generated_constraint_code.h
    cp machines/$i.False.True.H.5.vm_out ../minion/constraints/generated_constraint_code.h
    ( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; touch ../minion/constraints/generated_constraint_code.h; time (make minion 2>&1) &> $initial_path/machines/$i-sym.minion_time; cp minion $initial_path/machines/minion-$i-sym )
done

