#ifndef BYTE_H
#define BYTE_H

#include "vec/vec.h"

#define STACK_SIZE 1024 /* for later use? */
#define CHUNK_LEN 1024 /* max number of values in a given chunk */

typedef vec_t(char*) vec_chptr_t; /* resizable array for program instructions */

typedef struct State State;
typedef struct Value Value;
typedef struct Chunk Chunk;
typedef struct Program Program;

enum {
  OP_HLT, /* tell the program to halt */
  OP_PSH, /* push a value to the stack */
  OP_POP, /* pop a value from the stack */
  OP_ADD, /* add 2 values together */
  OP_SUB, /* subtract two values */
  OP_MUL, /* multiply 2 values */
  OP_DIV, /* divide 2 values */
  OP_EXP, /* raise one value to the power of another */
  OP_MOD, /* perform the mod operation on two values */
};

enum {
  VAL_TNIL, /* the nil value type */
  VAL_TNUMBER, /* the number value type */
  VAL_TSTRING, /* the string value type */
  VAL_TPAIR /* the pair value type */
};

struct State {
  Program *program_crnt; /* current set of instructions to be executed */
  Program *program_next; /* a list of instructions sets to execute next */
  Value **program_stack; /* registers for the executing programs */
  Value **gc_stack;      /* array of all live (in use) values */
  size_t gc_stack_idx;   /* current index for the top of gc_stack */
  size_t gc_stack_cap;   /* max capacity of gc_stack */
  Value *gc_pool;        /* a list of dead (can be reused) values */
  Chunk *gc_chunks;      /* a linked list of all the chunks */
  long gc_count;         /* countdown of new values until next GC cycle */
};

struct Value {
  unsigned char type; /* the value's type */
  unsigned char mark; /* to determine if the value is reachable */
  union {
    struct { long value;               } num;
    struct { char *value; size_t len;  } str;
    struct { Value *head, *tail;       } pair;
  }; /* tagged union of possible types and their contents */
};

struct Chunk {
  Value values[CHUNK_LEN]; /* array of values in current chunk */
  Chunk *next;             /* next chunk in chunk list */
};

struct Program {
  char *name;       /* name of the program */
  vec_chptr_t inst; /* instructions to execute */
};

State *state_new(void);                         /* create a new state */
static void state_close(State *S);          /* close give state */
static void state_push(State *S, Value *v); /* push a value in to the stack then add to current chunk */
Value *state_pop(State *S);                 /* pop a value from the stack */


Value *new_value(State *S, int type);   /* creates then returns a new emtyp value */
Value *new_nil(State *S);               /* creates then returns a new nil value */
Value *new_number(State *S, long num);  /* creates then returns a new number */
Value *new_string(State *S, char *str); /* creates then returns a new string */
Value *new_pair(State *S);              /* pops top 2 values, creates then returns a pair */

static void gc_free(State *S, Value *v); /* set a value to nil */
static void gc_deinit(State *S);         /* free all the values in all the chunks */
static void gc_mark(State *S, Value *v); /* mark all reachable objetcs */
static void gc_run(State *S);            /* perform a full GC cycle: run gc_mark and a sweep */

#endif
