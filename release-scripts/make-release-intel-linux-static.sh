#!/bin/bash
cd ..

ln -s `g++ -print-file-name=libstdc++.a`

./build-all.sh MYFLAGS=" -march=pentium -static -static-libgcc -L."

cd release-scripts

./make-release.sh intel-linux-static
