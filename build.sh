#!/bin/bash

flags="//Zi //TC"
outputFile="kriolu.exe"
trace="//DDEBUG_TRACE_EXECUTION"

if [[ $1 == "--release" ]]; then
  trace=""
fi

rm -rf build
rm -f kriolu
mkdir build
cd build
cl $flags ../src/*.c //Fe:$outputFile $trace
# mv kriolu ../
cd -