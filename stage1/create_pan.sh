#!/bin/sh

./spin -a conditioner.pml

gcc -pedantic -Wall -o pan pan.c 

rm pan.b pan.c pan.h pan.m pan.p pan.t

#./pan -d
./pan