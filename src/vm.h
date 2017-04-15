#ifndef SPORK_H
#define SPORK_H

#define SP_VERSION "0.0.1"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdarg.h>
#include "opcode.h"

#define UNUSED(x) ((void) x)
#define new(T) (T*)malloc(sizeof(T))
#define delete(T) free(T)

#define MAX_STACK 0xFFFF

typedef struct {
  size_t ip;
  size_t size;
  OP_CODE* head;
  OP_CODE* tail;
} byteVM;

byteVM* byte_new();
void byte_destroy(byteVM *vm);
void byte_push(byteVM *vm, int i);
OP_CODE* byte_pop(byteVM *vm);

#endif
