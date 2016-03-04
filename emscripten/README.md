
# To build minion with emscripten:

```
mkdir -p build-js # or similar
cd build-js
time cmake .. -DQUICK=1 -DCMAKE_C_COMPILER=emcc -DCMAKE_CXX_COMPILER=em++
time make minion -j4
mv minion-quick minion.bc
time emcc -O2 minion.bc -s TOTAL_MEMORY=100000000 -s DISABLE_EXCEPTION_CATCHING=0 --pre-js ../emscripten/pre.js --post-js ../emscripten/post.js -o minion.js
```


# To run the test in this directory

```
cp ../build-js/minion.js .
cp ../build-js/minion.js.mem .
python -m SimpleHTTPServer
```

Then, navigate to `localhost:8000/test.html` in your favourite browser.

