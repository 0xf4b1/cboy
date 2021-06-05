// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_CB_H
#define LIBCBOY_CB_H

#define DEFINE_CB_OP_REG(OP, REG) \
        void OP ## _ ## REG();

#define DEFINE_CB_OPS(REG) \
        DEFINE_CB_OP_REG(RLC, REG) \
        DEFINE_CB_OP_REG(RRC, REG) \
        DEFINE_CB_OP_REG(RL, REG) \
        DEFINE_CB_OP_REG(RR, REG) \
        DEFINE_CB_OP_REG(SLA, REG) \
        DEFINE_CB_OP_REG(SRA, REG) \
        DEFINE_CB_OP_REG(SWAP, REG) \
        DEFINE_CB_OP_REG(SRL, REG)

#define DEFINE_BIT_N_REG(N, REG) \
        void BIT_ ## N ## _ ## REG(); \
        void RES_ ## N ## _ ## REG(); \
        void SET_ ## N ## _ ## REG();

#define DEFINE_BIT_REG(REG) \
        DEFINE_BIT_N_REG(0, REG) \
        DEFINE_BIT_N_REG(1, REG) \
        DEFINE_BIT_N_REG(2, REG) \
        DEFINE_BIT_N_REG(3, REG) \
        DEFINE_BIT_N_REG(4, REG) \
        DEFINE_BIT_N_REG(5, REG) \
        DEFINE_BIT_N_REG(6, REG) \
        DEFINE_BIT_N_REG(7, REG)

#define DEFINE_CB(REG) \
        DEFINE_CB_OPS(REG) \
        DEFINE_BIT_REG(REG)

DEFINE_CB(B)
DEFINE_CB(C)
DEFINE_CB(D)
DEFINE_CB(E)
DEFINE_CB(H)
DEFINE_CB(L)
DEFINE_CB(A)
DEFINE_CB(HL)

#endif // LIBCBOY_CB_H