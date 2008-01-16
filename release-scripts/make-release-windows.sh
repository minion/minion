#!/bin/bash
cd ..
./build-all-windows.bat
cd release-scripts
./make-release.sh windows
