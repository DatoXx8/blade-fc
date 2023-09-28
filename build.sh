#!/bin/sh
set -xe
time clang main.c chess.c nn.c -ggdb -o blade -mavx2 -O3 -Wall -Wpedantic -Wextra -lm
./blade > out.txt
