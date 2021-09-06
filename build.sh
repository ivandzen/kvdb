#!/usr/bin/sh

if [ ! -d "./build" ]
then
   mkdir build
fi

cd ./build
cmake -G "Unix Makefiles" ../
numProcessors=$(cat /proc/cpuinfo | grep processor | wc -l)
make -j${numProcessors}
