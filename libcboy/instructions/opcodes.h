// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_OPCODES_H
#define LIBCBOY_OPCODES_H

unsigned char decode_and_execute(unsigned char opcode, unsigned short arg);

void decode_and_execute_cb(unsigned char opcode);

unsigned char op_length(unsigned char opcode);

void init_ops();

#endif // LIBCBOY_OPCODES_H