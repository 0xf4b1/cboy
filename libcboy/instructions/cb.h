// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_CB_H
#define LIBCBOY_CB_H

unsigned char RLC(unsigned char value);
unsigned char RRC(unsigned char value);
unsigned char RL(unsigned char value);
unsigned char RR(unsigned char value);
unsigned char SLA(unsigned char value);
unsigned char SRA(unsigned char value);
unsigned char SWAP(unsigned char value);
unsigned char SRL(unsigned char value);
unsigned char BIT(unsigned char value, unsigned char i);
unsigned char RES(unsigned char value, unsigned char i);
unsigned char SET(unsigned char value, unsigned char i);

#endif // LIBCBOY_CB_H