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

static void *zrealloc(State *S, void *ptr, size_t size) {
  if (ptr && size == 0) {
    dmt_free(ptr);
    return NULL;
  }
  void *p = dmt_realloc(ptr, size);
  if (!p) error_str(S, "out of memory");
  return p;
}

static void zfree(State *S, void *ptr) {
  zrealloc(S, ptr, 0);
}

/*====================================================
 * ERROR
 *====================================================*/

void error_out(State *S, Value *err) {
  fprintf(stderr, "%s[BYTE ERROR]:%s %s:%d %s(): ", color_red, color_reset, __FILE__, __LINE__, __func__);
  fprintf(stderr, "%s", value_to_string(S, err));
  fprintf(stderr, "\n");
  abort();
}

void error_str(State *S, const char *fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);
  error_out(S, new_string(S, buf));
}

/*====================================================
 * STATE
 *====================================================*/

State *state_new(void) {
  State *volatile S = NULL;
  S = zrealloc(S, NULL, sizeof(*S));
  if (!S) return NULL;
  memset(S, 0, sizeof(*S));
  return S;
}

static void state_close(State *S) {
  gc_deinit(S);
  zfree(S, S);
}

static void state_push(State *S, Value *v) {
  /* extend the stack's capacity if it has reached the cap */
  if (S->gc_stack_idx == S->gc_stack_cap) {
    size_t size = (S->gc_stack_cap << 1) | !S->gc_stack_cap;
    S->gc_stack = zrealloc(S, S->gc_stack, size * sizeof(*S->gc_stack));
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
  /* if (!S->gc_pool && S->gc_count < 0) { -- rxi's implementation */
  if (!S->gc_pool || S->gc_count < 0) { /* my implementation */
    gc_run(S);
  }
  /* if there are no values in the pool, create init a new chunk */
  if (!S->gc_pool) {
    size_t i;
    Chunk *c = zrealloc(S, NULL, sizeof(*c));
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

Value *new_number(State *S, double num) {
  Value *v = new_value(S, VAL_TNUMBER);
  v->num.value = num;
  return v;
}

Value *new_stringl(State *S, char *str, size_t len) {
  Value *v = new_value(S, VAL_TSTRING);
  v->str.value = NULL;
  v->str.value = zrealloc(S, NULL, len + 1);
  v->str.value[len] = '\0';
  if (str) {
    memcpy(v->str.value, str, len);
  }
  v->str.len = len;
  return v;
}

Value *new_string(State *S, char *str) {
  if (str == NULL) return NULL;
  return new_stringl(S, str, strlen(str));
}

Value *new_pair(State *S, Value *head, Value *tail) {
  Value *v = new_value(S, VAL_TPAIR);
  v->pair.head = head;
  v->pair.tail = tail;
  return v;
}

int value_type(Value *v) {
  return v ? v->type : VAL_TNIL;
}

Value *Value_to_string(State *S, Value *v) {
  char buf[128];
  switch (value_type(v)) {
    case VAL_TNIL:
      return new_string(S, "nil");
    case VAL_TNUMBER:
      sprintf(buf, "%.14g", v->num.value);
      return new_string(S, buf);
    case VAL_TSTRING:
      sprintf(buf, "%s", v->str.value);
      return new_string(S, buf);
    case VAL_TPAIR:
      sprintf(buf, "(%s, %s)", value_to_string(S, v->pair.head),
        value_to_string(S, v->pair.tail));
      return new_string(S, buf);
    default:
      sprintf(buf, "[%s %p]", value_type_str(value_type(v)), (void*) v);
      return new_string(S, buf);
  }
}

const char *value_to_stringl(State *S, Value *v, size_t *len) {
  v = Value_to_string(S, v);
  if (len) *len = v->str.len;
  return v->str.value;
}

const char *value_to_string(State *S, Value *v) {
  return value_to_stringl(S, v, NULL);
}

const char *value_type_str(int type) {
  switch (type) {
    case VAL_TNIL    : return "nil";
    case VAL_TNUMBER : return "number";
    case VAL_TSTRING : return "string";
    case VAL_TPAIR   : return "pair";
    default          : return "unknown";
  }
}

Value *value_check(State *S, Value *v, int type) {
  if (value_type(v) != type) {
    error_str(S, "expected %s got %s\n",\
    value_type_str(type),\
    value_type_str(value_type(v)));
  }
  return v;
}


/*====================================================
 * GARBAGE COLLECTOR
 *====================================================*/

static void gc_free(State *S, Value *v) {
  switch (v->type) {
    case VAL_TSTRING:
      zfree(S, v->str.value);
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
    zfree(S, c);
    c = next;
  }
  /* free the stack */
  zfree(S, S->gc_stack);
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
  new_number(S, 000);
  new_number(S, 001);
  new_number(S, 010);
  new_number(S, 011);
  new_number(S, 100);
  new_number(S, 101);
  new_number(S, 110);
  new_number(S, 110);
  state_pop(S);
  state_pop(S);
  state_pop(S);
  new_number(S, 110);
  new_number(S, 111);
  new_pair(S, new_number(S, 000), new_number(S, 111));
  puts(value_to_string(S, state_pop(S)));
  state_close(S);
}
