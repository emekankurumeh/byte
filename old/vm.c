// http://gameprogrammingpatterns.com/bytecode.html
// https://felixangell.com/blog/virtual-machine-in-c
// https://rsms.me/sol-a-sunny-little-virtual-machine
// https://github.com/jakogut/tinyvm
// http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
// https://www.youtube.com/watch?v=f8EW2uPz868

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "vm.h"

byteVM* byte_new() {
  byteVM *vm = new(byteVM);
  vm->ip = 0x00;
  vm->size = 0x00;
  vm->head = NULL;
  vm->tail = NULL;
  return vm;
}

void byte_destroy(byteVM *vm) {
  OP_CODE *t = byte_pop(vm);
  while (t) {
    printf("%i\n", t->value);
    delete(t);
    t = byte_pop(vm);
  }
}

void byte_push(byteVM *vm, int i) {
  OP_CODE *v = new(OP_CODE);
  v->value = i;
  if (vm->size > 0) {
    vm->tail->next = v;
    vm->tail = v;
  } else if (vm->size == 0) {
    vm->head = v;
    vm->tail = v;
  }
  vm->size++;
}

OP_CODE *byte_pop(byteVM *vm) {
  OP_CODE *v = new(OP_CODE);
  if (vm->size > 1) {
    v = vm->head;
    vm->head = vm->head->next;
  } else if (vm->size == 1) {
    v = vm->head;
    vm->head = NULL;
    vm->tail = NULL;
  } else {
    v = NULL;
  }
  vm->size--;
  return v;
}

byteVM *example;

static void shutdown() {
  byte_destroy(example);
}

int main() {
  atexit(shutdown);
  example = byte_new();
  byte_push(example, OP_HLT);
  byte_push(example, OP_MUL);
  return 0;
}
