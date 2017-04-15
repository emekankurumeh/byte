#ifndef OPCODE_H
#define OPCODE_H

typedef struct OP_CODE OP_CODE;

typedef enum {
  OP_HLT = 0x00,
  OP_PSH = 0x01,
  OP_POP = 0x02,
  OP_ADD = 0x03,
  OP_SUB = 0x04,
  OP_MUL = 0x05,
  OP_DIV = 0x06,
  OP_MOD = 0x07,
  OP_SET = 0x08,
  OP_GET = 0x09,
} op_code;

struct OP_CODE {
  op_code value;
  OP_CODE *next;
};

#endif
