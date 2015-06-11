To build with emscripten:

```
mkdir build-js # or similar
cd build-js
time cmake .. -DCMAKE_C_COMPILER=emcc -DCMAKE_CXX_COMPILER=em++
time make minion
mv minion minion.bc
time emcc minion.bc -O0 -o minion-O0.js
time emcc minion.bc -O2 -o minion-O2.js
```

```
mkdir build-js-quick # or similar
cd build-js-quick
time cmake .. -DQUICK=1 -DCMAKE_C_COMPILER=emcc -DCMAKE_CXX_COMPILER=em++
time make minion
mv minion-quick minion-quick.bc
time emcc minion-quick.bc -O0 -o minion-quick-O0.js
time emcc minion-quick.bc -O2 -o minion-quick-O2.js
```

