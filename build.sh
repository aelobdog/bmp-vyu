#!/bin/bash
gcc -o bmp.out bmp-vyu.c -lraylib -lGL -lm -pthread -ldl -ggdb && ./bmp.out $1
