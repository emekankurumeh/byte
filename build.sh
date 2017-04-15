#!/bin/bash

OUTPUT=bin
SOURCE=src
BINARY=$OUTPUT/byte
MAIN=vm.c
CFLAGS="--std=c99 -Wall -Wextra -pedantic -O3 -L$OUTPUT -o $BINARY"
LFLAGS=""

echo "initing..."
rm -rf $OUTPUT && mkdir $OUTPUT

echo "building..."
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

echo "compiling..."
gcc $CFLAGS $SOURCE/$MAIN $LFLAGS

echo "stripping.."
strip $BINARY

echo "clearing up..."
for F in $SOURCE/*/*.c
	do
		filename=$(basename $F)
		rm -rf "$OUTPUT/${filename/.c/.o}"
		rm -rf "$OUTPUT/lib${filename/.c/.a}"
	done

echo "done."
