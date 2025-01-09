#!/bin/bash

flags="//Zi //TC"
outputFile="kriolu.exe"

rm -rf build
rm -f kriolu
mkdir build
cd build
cl $flags //Fe:$outputFile ../src/*.c
mv kriolu ../
cd -