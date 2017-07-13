/*====================================================
 * LABEL
 *====================================================*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mpc/mpc.h"
#include "dmt/dmt.h"
#include "byte.h"
#include "util.h"

static void *zrealloc(void *ptr, size_t size) {
  if (ptr && size == 0) {
    dmt_free(ptr);
    return NULL;
  }
  void *p = dmt_realloc(ptr, size);
  if (!p) ERROR("out of memory");
  return p;
}

static void zfree(void *ptr) {
  zrealloc(ptr, 0);
}

/*====================================================
 * STATE
 *====================================================*/

State *state_new(void) {
  State *volatile S;
  S = zrealloc(NULL, sizeof(*S));
  if (!S) return NULL;
  memset(S, 0, sizeof(*S));
  return S;
}

static void state_close(State *S) {
  gc_deinit(S);
  zfree(S);
}

static void state_push(State *S, Value *v) {
  /* extend the stack's capacity if it has reached the cap */
  if (S->gc_stack_idx == S->gc_stack_cap) {
    size_t size = (S->gc_stack_cap << 1) | !S->gc_stack_cap;
    S->gc_stack = zrealloc(S->gc_stack, size * sizeof(*S->gc_stack));
    S->gc_stack_cap = size;
  }
  /* push the value to the stack */
  S->gc_stack[S->gc_stack_idx++] = v;
}

Value *state_pop(State *S) {
  ASSERT(S->gc_stack_idx > 0, "stack underflow");
  return S->gc_stack[--S->gc_stack_idx];
}

/*====================================================
 * VALUE
 *====================================================*/

Value *new_value(State *S, int type) {
  Value *v;
  /* check if it is time to run GC */
  S->gc_count--;
  if (S->gc_count < 0) {
    gc_run(S);
  }
  /* if there are no values in the pool, create init a new chunk */
  if (!S->gc_pool) {
    size_t i;
    // puts("kjhn");
    Chunk *c = zrealloc(NULL, sizeof(*c));
    c->next = S->gc_chunks;
    S->gc_chunks = c;
    /* init all the chunk's values, link them together, set
    the currently-empty pool to  point at this list*/
    for (i = 0; i < CHUNK_LEN; i++) {
      c->values[i].type = VAL_TNIL;
      c->values[i].next = (c->values + i + 1);
    }
    c->values[CHUNK_LEN - 1].next = NULL;
    S->gc_pool = c->values;
  }
  /* get a value from the pool */
  v = S->gc_pool;
  S->gc_pool = v->next;

  /* init the value */
  v->type = type;
  v->mark = 0;
  state_push(S, v);
  return v;
}

Value *new_nil(State *S) {
  Value *v = new_value(S, VAL_TNIL);
  return v;
}

Value *new_number(State *S, long num) {
  Value *v = new_value(S, VAL_TNUMBER);
  v->num.value = num;
  return v;
}

Value *new_string(State *S, char *str) {
  Value *v = new_value(S, VAL_TSTRING);
  // v->str.value = str;
  v->str.len = strlen(str);
  zrealloc(v->str.value, sizeof(char)*v->str.len);
  return v;
}

Value *new_pair(State *S) {
  Value *v = new_value(S, VAL_TPAIR);
  v->pair.head = state_pop(S);
  v->pair.tail = state_pop(S);
  return v;
}

/*====================================================
 * GARBAGE COLLECTOR
 *====================================================*/

static void gc_free(State *S, Value *v) {
  switch (v->type) {
    case VAL_TSTRING:
      zfree(v->str.value);
      v->str.len = 0;
      break;
  }
  v->type = VAL_TNIL;
  v->next = S->gc_pool;
  S->gc_pool = v;

}

static void gc_deinit(State *S) {
  size_t i;
  Chunk *c, *next;
  /* free all the cunks values */
  c = S->gc_chunks;
  while (c) {
    next = c->next;
    for (i = 0; i < CHUNK_LEN; i++) {
      gc_free(S, c->values + i);
    }
    /* free the chunk itself */
    zfree(c);
    c = next;
  }
  /* free the stack */
  zfree(S->gc_stack);
}

static void gc_mark(State *S, Value *v) {
  begin:
    if (!v || v->mark) return;
    v->mark = 1;
    switch (v->type) {
      case VAL_TPAIR: {
        gc_mark(S, v->pair.head);
        v = v->pair.tail;
        goto begin;
        break;
      }
    }
}

static void gc_run(State *S) {
  size_t i, clean = 0, dirty = 0;
  Chunk *c;
  /* mark the values on the stack */
  for (i = 0; i < S->gc_stack_idx; i++) gc_mark(S, S->gc_stack[i]);
  /* free unmarked values, count and unmark remaining values */
  c = S->gc_chunks;
  while (c) {
    for (i = 0; i < CHUNK_LEN; i++) {
      if (c->values[i].type != VAL_TNIL) {
        if (!c->values[i].mark) {
          gc_free(S, c->values + i);
          dirty++;
        } else {
          c->values[i].mark = 0;
          clean++;
        }
      }
    }
    c = c->next;
  }
  /* reset GC counter and output debug info */
  S->gc_count = clean;
  GCINFO(clean, dirty);
}

/*====================================================
 * STANDALONE
 *====================================================*/

int main(void) {
  State *S = state_new();
  new_number(S, 8);
  new_number(S, 8);
  printf("[(ld, ld), (ld, %ld)]\n", state_pop(S)->num.value);
  state_close(S);
}
