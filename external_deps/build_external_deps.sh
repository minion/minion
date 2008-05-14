#!/bin/bash
echo Building libbz2.a
cd bzip2-1.0.5
make libbz2.a CFLAGS="$*"
cd ..
echo Building libz.a
cd zlib-1.2.3
make libz.a CFLAGS="$*"
cd ..
