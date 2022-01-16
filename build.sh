#!/bin/bash
clang -o bmp.out bmp-vyu.c -lraylib -lGL -lm -pthread -ldl && ./bmp.out $1
