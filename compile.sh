#!/bin/sh

mkdir -p build
cd build

if [[ "$0" == "Release" ]]; then
    echo "Release"
    cmake -DCMAKE_BUILD_TYPE=Release ../cmake
    cmake --build .
else
    echo "Debug"
    cmake -DCMAKE_BUILD_TYPE=Debug ../cmake
    cmake --build .
fi

cd ..

