#!/bin/bash

name=$1
shift
../bin/minion -redump $name | ../bin/minion $* --
