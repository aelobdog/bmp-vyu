#!/bin/bash
clang -o bmp.out bmp.c -lraylib -lGL -lm -pthread -ldl && ./bmp.out $1
