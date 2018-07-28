#!/bin/bash

LIBS="-lm"

if [[ $OS = *"Windows"* ]]; then
  LIBS="$LIBS -largp"
fi

gcc -Wall -O3 -D_GNU_SOURCE -o decodez80 src/main.c src/z80tables.c  $LIBS
