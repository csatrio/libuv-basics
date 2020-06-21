#!/bin/bash
OUTPUT_FILE=tcp-echo
rm -rf $OUTPUT_FILE
g++ -fpermissive -fPIC -g -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" /usr/local/lib/libuv.a -o $(pwd)/bin/$OUTPUT_FILE $(pwd)/$OUTPUT_FILE.c -luv -lpthread
chmod 777 basic-libuv-test
valgrind -v --num-callers=500 --track-origins=yes --leak-check=full --show-leak-kinds=all $(pwd)/bin/./$OUTPUT_FILE
