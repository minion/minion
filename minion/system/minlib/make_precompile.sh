#!/bin/bash
rm -rf minlib.hpp.gch

../dominion_build minlib.hpp -g
mv minlib.hpp.gch minlib.hpp.gch.1
../dominion_build minlib.hpp -O2 -fomit-frame-pointer
mv minlib.hpp.gch minlib.hpp.gch.2
../dominion_build minlib.hpp -g -DDOM_ASSERT -D_GLIBCXX_DEBUG
mv minlib.hpp.gch minlib.hpp.gch.3
../dominion_build minlib.hpp -g -O2 -fomit-frame-pointer -DDOM_ASSERT -D_GLIBCXX_DEBUG
mv minlib.hpp.gch minlib.hpp.gch.4
../dominion_build minlib.hpp -O2 -fomit-frame-pointer -DDOM_SINGLESOL
mv minlib.hpp.gch minlib.hpp.gch.5
mkdir minlib.hpp.gch
mv minlib.hpp.gch.* minlib.hpp.gch/
