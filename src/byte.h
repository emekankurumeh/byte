#ifndef BYTE_H
#define BYTE_H

#define STACK_SIZE 1024
#define CHUNK_SIZE 1024

typedef struct State State;
typedef struct Value Value;
typedef struct Chunk Chunk;

enum {
  OBJ_NUM,
  OBJ_STR,
  OBJ_PAIR
};

struct State {
  Value **stack;
  size_t stackSize;
  Value *value;
  long gc_count;
};

struct Value {
  unsigned char type, mark;
  Value *next;

  union {
    struct { long value;               } num;
    struct { char *value; size_t len;  } str;
    struct { Value *head, *tail;       } pair;
  };
};

struct Chunk {
  Value values[CHUNK_SIZE];
  Chunk *next;
};

State *state_new();
void state_close(State *S);
void state_push(State *S, Value *v);
Value *state_pop(State *S);

Value *new_value(State *S, int type);
Value *new_number(State *S, long num);
Value *new_string(State *S, char *str);
Value *new_pair(State *S);

void gc_mark(State *S);
void mark_value(Value *v);
void gc_sweep(State *S);
void gc_run(State *S);

#endif
