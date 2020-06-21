#!/bin/bash
reset
OUTPUT_FILE=basic-libuv-test
COMPILER_FLAGS="-Wall -fno-stack-protector -fpermissive -fPIC -g"
rm -rf $OUTPUT_FILE
g++ $COMPILER_FLAGS -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" /usr/local/lib/libuv.a -o $(pwd)/bin/$OUTPUT_FILE $(pwd)/$OUTPUT_FILE.cpp -luv -lpthread
chmod 777 basic-libuv-test

#valkyrie --track-origins=yes --show-reachable=yes basic-libuv-test
valgrind -v --num-callers=500 --track-origins=yes --leak-check=full --show-leak-kinds=all bin/./basic-libuv-test
