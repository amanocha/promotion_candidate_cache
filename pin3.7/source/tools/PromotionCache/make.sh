#!/bin/bash

make clean
make

rm -rf obj-intel64
mkdir obj-intel64
make obj-intel64/roi.so
