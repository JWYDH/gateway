#!/bin/bash

cd build
rm -rf *
cmake ..

make -j2

cd ..