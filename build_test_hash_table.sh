#!/usr/bin/sh

basedir="$PWD"
flags="//Zi //TC"
# echo "directory: ${dir}"

rm -rf $basedir/tests/hash_table/build
mkdir $basedir/tests/hash_table/build
cd $basedir/tests/hash_table/build
cl //Zi //TC //Fe:hash_test.exe //I $basedir/src $basedir/tests/hash_table/main.c $basedir/src/hash_table.c $basedir/src/string.c $basedir/src/object.c $basedir/src/value.c
cd -

# Run the program
# ./build/hash_test.exe