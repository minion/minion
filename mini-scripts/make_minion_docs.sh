#!/bin/bash

set -e

cd $(dirname -- "$0")
cd ../minion
help/genhelp.sh > help/help.cpp
cd ..
./docs/genhelp/genhelp.sh
./docs/genhelp/genlatexhelp.sh
cd docs
pdflatex Manual.tex
bibtex Manual.aux
pdflatex Manual.tex
pdflatex Manual.tex
