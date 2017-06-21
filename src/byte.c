#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "byte.h"
#include "util.h"

State *state_new() {
  State *S = calloc(1, sizeof(State));
  S->stack = calloc(STACK_SIZE, sizeof(Value*));
  S->stackSize = 0;
  // S->value = NULL;
  memset()
  S->gc_count = 0;
  // printf("gc_count: %ld\n", S->gc_count);
  return S;
}

void state_close(State *S) {
  Value ** v = &S->value;
  free(S);
}

void state_push(State *S, Value *v) {
  assert(S->stackSize < STACK_SIZE, "stack overflow");
  S->stack[S->stackSize++] = v;
}

Value *state_pop(State *S) {
  assert(S->stackSize > 0, "stack underflow");
  return S->stack[--S->stackSize];
}



Value *new_value(State *S, int type) {
  printf("gc_count: %ld\n", S->gc_count);
  printf("stack_size: %ld\n", S->stackSize);
  printf("------------------------------\n");
  S->gc_count--;
  if (S->gc_count < 0) {
    gc_run(S);
  }
  Value *v = calloc(1, sizeof(Value));
  v->type = type;
  v->mark = 0;
  v->next = S->value;
  S->value = v;

  return v;
}

Value *new_number(State *S, long num) {
  Value *v = new_value(S, OBJ_NUM);
  v->num.value = num;
  state_push(S, v);
  return v;
}

Value *new_string(State *S, char *str) {
  Value *v = new_value(S, OBJ_STR);
  v->str.value = str;
  v->str.len = strlen(str);
  state_push(S, v);
  return v;
}

Value *new_pair(State *S) {
  Value *v = new_value(S, OBJ_PAIR);
  v->pair.head = state_pop(S);
  v->pair.tail = state_pop(S);
  state_push(S, v);
  return v;
}


void gc_mark(State *S) {
  for (size_t i = 0; i < S->stackSize; i++) {
    mark_value(S->stack[i]);
  }
}

void mark_value(Value *v) {
  if (v->mark) return;
  v->mark = 1;
  if (v->type == OBJ_PAIR) {
    mark_value(v->pair.head);
    mark_value(v->pair.tail);
  }
}

void gc_sweep(State *S) {
  Value **v = &S->value;
  long clean, dirty;
  clean, dirty = 0;
  while (*v) {
    puts("sweeping");
    if (!(*v)->mark) {
      Value *unreached = *v;
      *v = unreached->next;
      free(unreached);
      dirty++;
    } else {
      (*v)->mark = 0;
      v = &(*v)->next;
      clean++;
    }
  }
  S->gc_count = clean * 2;
  // GCINFO(clean, dirty);
}

void gc_run(State *S) {
  gc_mark(S);
  gc_sweep(S);
}


int main(void) {
  State *S = state_new();
  // printf("gc_count: %ld\n", S->gc_count);
  // new_string(S, "ho");
  new_number(S, 2);
  new_number(S, 2);
  // printf("gc_count: %ld\n", S->gc_count);
  new_pair(S);
  new_number(S, 8);
  new_number(S, 19);
  new_pair(S);
  new_pair(S);
  Value *a = state_pop(S);
  printf("[(%ld, %ld), (%ld, %ld)]\n", a->pair.head->pair.head->num.value, a->pair.head->pair.tail->num.value, a->pair.tail->pair.head->num.value, a->pair.tail->pair.tail->num.value);
  state_close(S);
}