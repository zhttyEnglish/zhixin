#!/bin/bash

rm -rf libs
rm -rf lib32
rm -rf lib64

make clean
make APP_32BIT=y
mv libs lib32

make clean
make APP_32BIT=n
mv libs lib64
