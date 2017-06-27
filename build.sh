#!/bin/bash

OUTPUT=bin
SOURCE=src
MAIN=byte
BINARY=$OUTPUT/$MAIN
LFLAGS=""

while [[ "$#" ]]; do case $1 in
    debug)
    CFLAGS="--std=c99 -Wall -Wextra -pedantic -DSP_STANDALONE -g -L$OUTPUT -o $BINARY"
    TAG="[DEBUG]"
  break
  ;;
    release)
    CFLAGS="--std=c99 -Wall -Wextra -pedantic -DSP_STANDALONE -O3 -L$OUTPUT -o $BINARY"
    TAG="[RELEASE]"
    break
  ;;
    *)
    CFLAGS=" -Wall -Wextra -pedantic -DSP_STANDALONE -g -L$OUTPUT -o $BINARY"
    TAG="[DEBUG]"
    break
  ;;
  esac; shift; shift
done

echo "$TAG: initing..."
rm -rf $OUTPUT && mkdir $OUTPUT

echo "$TAG: building..."
for F in $SOURCE/*/*.c
  do
    filename=$(basename $F)
    gcc -c -o "$OUTPUT/${filename/.c/.o}" $F
  done

for F in $OUTPUT/*.o
  do
    filename=$(basename $F)
    ar rcs "$OUTPUT/lib${filename/.o/.a}" $F
    rm -rf $F
    LFLAGS="$LFLAGS -l${filename/.o/}"
  done

echo "$TAG: compiling..."
gcc $CFLAGS $SOURCE/$MAIN.c $LFLAGS

echo "$TAG: stripping.."
strip $BINARY

echo "$TAG: cleaning up..."
for F in $SOURCE/*/*.c
  do
    filename=$(basename $F)
    rm -rf "$OUTPUT/${filename/.c/.o}"
    rm -rf "$OUTPUT/lib${filename/.c/.a}"
  done

echo "$TAG: done."
