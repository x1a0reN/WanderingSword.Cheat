#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <vector>

namespace InlineHook {
namespace ExternalHde64 {

/*
 * Hacker Disassembler Engine 64
 * Copyright (c) 2008-2009, Vyacheslav Patkov.
 * All rights reserved.
 *
 * hde64.h: C/C++ header file
 *
 */



 // Integer types for HDE. stdint.h

// table64.h
#define C_NONE    0x00
#define C_MODRM   0x01
#define C_IMM8    0x02
#define C_IMM16   0x04
#define C_IMM_P66 0x10
#define C_REL8    0x20
#define C_REL32   0x40
#define C_GROUP   0x80
#define C_ERROR   0xff

#define PRE_ANY  0x00
#define PRE_NONE 0x01
#define PRE_F2   0x02
#define PRE_F3   0x04
#define PRE_66   0x08
#define PRE_67   0x10
#define PRE_LOCK 0x20
#define PRE_SEG  0x40
#define PRE_ALL  0xff

#define DELTA_OPCODES      0x4a
#define DELTA_FPU_REG      0xfd
#define DELTA_FPU_MODRM    0x104
#define DELTA_PREFIXES     0x13c
#define DELTA_OP_LOCK_OK   0x1ae
#define DELTA_OP2_LOCK_OK  0x1c6
#define DELTA_OP_ONLY_MEM  0x1d8
#define DELTA_OP2_ONLY_MEM 0x1e7

static const unsigned char hde64_table[] = {
  0xa5,0xaa,0xa5,0xb8,0xa5,0xaa,0xa5,0xaa,0xa5,0xb8,0xa5,0xb8,0xa5,0xb8,0xa5,
  0xb8,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xac,0xc0,0xcc,0xc0,0xa1,0xa1,
  0xa1,0xa1,0xb1,0xa5,0xa5,0xa6,0xc0,0xc0,0xd7,0xda,0xe0,0xc0,0xe4,0xc0,0xea,
  0xea,0xe0,0xe0,0x98,0xc8,0xee,0xf1,0xa5,0xd3,0xa5,0xa5,0xa1,0xea,0x9e,0xc0,
  0xc0,0xc2,0xc0,0xe6,0x03,0x7f,0x11,0x7f,0x01,0x7f,0x01,0x3f,0x01,0x01,0xab,
  0x8b,0x90,0x64,0x5b,0x5b,0x5b,0x5b,0x5b,0x92,0x5b,0x5b,0x76,0x90,0x92,0x92,
  0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x6a,0x73,0x90,
  0x5b,0x52,0x52,0x52,0x52,0x5b,0x5b,0x5b,0x5b,0x77,0x7c,0x77,0x85,0x5b,0x5b,
  0x70,0x5b,0x7a,0xaf,0x76,0x76,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,
  0x5b,0x5b,0x86,0x01,0x03,0x01,0x04,0x03,0xd5,0x03,0xd5,0x03,0xcc,0x01,0xbc,
  0x03,0xf0,0x03,0x03,0x04,0x00,0x50,0x50,0x50,0x50,0xff,0x20,0x20,0x20,0x20,
  0x01,0x01,0x01,0x01,0xc4,0x02,0x10,0xff,0xff,0xff,0x01,0x00,0x03,0x11,0xff,
  0x03,0xc4,0xc6,0xc8,0x02,0x10,0x00,0xff,0xcc,0x01,0x01,0x01,0x00,0x00,0x00,
  0x00,0x01,0x01,0x03,0x01,0xff,0xff,0xc0,0xc2,0x10,0x11,0x02,0x03,0x01,0x01,
  0x01,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0x10,
  0x10,0x10,0x10,0x02,0x10,0x00,0x00,0xc6,0xc8,0x02,0x02,0x02,0x02,0x06,0x00,
  0x04,0x00,0x02,0xff,0x00,0xc0,0xc2,0x01,0x01,0x03,0x03,0x03,0xca,0x40,0x00,
  0x0a,0x00,0x04,0x00,0x00,0x00,0x00,0x7f,0x00,0x33,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0xff,0xbf,0xff,0xff,0x00,0x00,0x00,0x00,0x07,0x00,0x00,0xff,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
  0x00,0x00,0x00,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x00,
  0xff,0x40,0x40,0x40,0x40,0x41,0x49,0x40,0x40,0x40,0x40,0x4c,0x42,0x40,0x40,
  0x40,0x40,0x40,0x40,0x40,0x40,0x4f,0x44,0x53,0x40,0x40,0x40,0x44,0x57,0x43,
  0x5c,0x40,0x60,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
  0x40,0x40,0x64,0x66,0x6e,0x6b,0x40,0x40,0x6a,0x46,0x40,0x40,0x44,0x46,0x40,
  0x40,0x5b,0x44,0x40,0x40,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x01,0x06,
  0x06,0x02,0x06,0x06,0x00,0x06,0x00,0x0a,0x0a,0x00,0x00,0x00,0x02,0x07,0x07,
  0x06,0x02,0x0d,0x06,0x06,0x06,0x0e,0x05,0x05,0x02,0x02,0x00,0x00,0x04,0x04,
  0x04,0x04,0x05,0x06,0x06,0x06,0x00,0x00,0x00,0x0e,0x00,0x00,0x08,0x00,0x10,
  0x00,0x18,0x00,0x20,0x00,0x28,0x00,0x30,0x00,0x80,0x01,0x82,0x01,0x86,0x00,
  0xf6,0xcf,0xfe,0x3f,0xab,0x00,0xb0,0x00,0xb1,0x00,0xb3,0x00,0xba,0xf8,0xbb,
  0x00,0xc0,0x00,0xc1,0x00,0xc7,0xbf,0x62,0xff,0x00,0x8d,0xff,0x00,0xc4,0xff,
  0x00,0xc5,0xff,0x00,0xff,0xff,0xeb,0x01,0xff,0x0e,0x12,0x08,0x00,0x13,0x09,
  0x00,0x16,0x08,0x00,0x17,0x09,0x00,0x2b,0x09,0x00,0xae,0xff,0x07,0xb2,0xff,
  0x00,0xb4,0xff,0x00,0xb5,0xff,0x00,0xc3,0x01,0x00,0xc7,0xff,0xbf,0xe7,0x08,
  0x00,0xf0,0x02,0x00
};
//end

#define F_MODRM         0x00000001
#define F_SIB           0x00000002
#define F_IMM8          0x00000004
#define F_IMM16         0x00000008
#define F_IMM32         0x00000010
#define F_IMM64         0x00000020
#define F_DISP8         0x00000040
#define F_DISP16        0x00000080
#define F_DISP32        0x00000100
#define F_RELATIVE      0x00000200
#define F_ERROR         0x00001000
#define F_ERROR_OPCODE  0x00002000
#define F_ERROR_LENGTH  0x00004000
#define F_ERROR_LOCK    0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ  0x01000000
#define F_PREFIX_REPX   0x02000000
#define F_PREFIX_REP    0x03000000
#define F_PREFIX_66     0x04000000
#define F_PREFIX_67     0x08000000
#define F_PREFIX_LOCK   0x10000000
#define F_PREFIX_SEG    0x20000000
#define F_PREFIX_REX    0x40000000
#define F_PREFIX_ANY    0x7f000000

#define PREFIX_SEGMENT_CS   0x2e
#define PREFIX_SEGMENT_SS   0x36
#define PREFIX_SEGMENT_DS   0x3e
#define PREFIX_SEGMENT_ES   0x26
#define PREFIX_SEGMENT_FS   0x64
#define PREFIX_SEGMENT_GS   0x65
#define PREFIX_LOCK         0xf0
#define PREFIX_REPNZ        0xf2
#define PREFIX_REPX         0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67

#pragma pack(push,1)

typedef struct {
    uint8_t len;
    uint8_t p_rep;
    uint8_t p_lock;
    uint8_t p_seg;
    uint8_t p_66;
    uint8_t p_67;
    uint8_t rex;
    uint8_t rex_w;
    uint8_t rex_r;
    uint8_t rex_x;
    uint8_t rex_b;
    uint8_t opcode;
    uint8_t opcode2;
    uint8_t modrm;
    uint8_t modrm_mod;
    uint8_t modrm_reg;
    uint8_t modrm_rm;
    uint8_t sib;
    uint8_t sib_scale;
    uint8_t sib_index;
    uint8_t sib_base;
    union {
        uint8_t imm8;
        uint16_t imm16;
        uint32_t imm32;
        uint64_t imm64;
    } imm;
    union {
        uint8_t disp8;
        uint16_t disp16;
        uint32_t disp32;
    } disp;
    uint32_t flags;
} hde64s;

#pragma pack(pop)

//hde64.c
#pragma warning(push)
#pragma warning(disable:4701)
#pragma warning(disable:4706)
#pragma warning(disable:26451)

static inline unsigned int hde64_disasm(const void* code, hde64s* hs)
{
    uint8_t x, c = 0, * p = (uint8_t*)code, cflags, opcode, pref = 0;
    const uint8_t* ht = hde64_table;
    uint8_t m_mod, m_reg, m_rm, disp_size = 0;
    uint8_t op64 = 0;

    std::memset(hs, 0, sizeof(hde64s));
    for (x = 16; x; x--)
        switch (c = *p++) {
        case 0xf3:
            hs->p_rep = c;
            pref |= PRE_F3;
            break;
        case 0xf2:
            hs->p_rep = c;
            pref |= PRE_F2;
            break;
        case 0xf0:
            hs->p_lock = c;
            pref |= PRE_LOCK;
            break;
        case 0x26: case 0x2e: case 0x36:
        case 0x3e: case 0x64: case 0x65:
            hs->p_seg = c;
            pref |= PRE_SEG;
            break;
        case 0x66:
            hs->p_66 = c;
            pref |= PRE_66;
            break;
        case 0x67:
            hs->p_67 = c;
            pref |= PRE_67;
            break;
        default:
            goto pref_done;
        }
pref_done:

    hs->flags = (uint32_t)pref << 23;

    if (!pref)
        pref |= PRE_NONE;

    if ((c & 0xf0) == 0x40) {
        hs->flags |= F_PREFIX_REX;
        if ((hs->rex_w = (c & 0xf) >> 3) && (*p & 0xf8) == 0xb8)
            op64++;
        hs->rex_r = (c & 7) >> 2;
        hs->rex_x = (c & 3) >> 1;
        hs->rex_b = c & 1;
        if (((c = *p++) & 0xf0) == 0x40) {
            opcode = c;
            goto error_opcode;
        }
    }

    if ((hs->opcode = c) == 0x0f) {
        hs->opcode2 = c = *p++;
        ht += DELTA_OPCODES;
    }
    else if (c >= 0xa0 && c <= 0xa3) {
        op64++;
        if (pref & PRE_67)
            pref |= PRE_66;
        else
            pref &= ~PRE_66;
    }

    opcode = c;
    cflags = ht[ht[opcode / 4] + (opcode % 4)];

    if (cflags == C_ERROR) {
    error_opcode:
        hs->flags |= F_ERROR | F_ERROR_OPCODE;
        cflags = 0;
        if ((opcode & -3) == 0x24)
            cflags++;
    }

    x = 0;
    if (cflags & C_GROUP) {
        uint16_t t;
        t = *(uint16_t*)(ht + (cflags & 0x7f));
        cflags = (uint8_t)t;
        x = (uint8_t)(t >> 8);
    }

    if (hs->opcode2) {
        ht = hde64_table + DELTA_PREFIXES;
        if (ht[ht[opcode / 4] + (opcode % 4)] & pref)
            hs->flags |= F_ERROR | F_ERROR_OPCODE;
    }

    if (cflags & C_MODRM) {
        hs->flags |= F_MODRM;
        hs->modrm = c = *p++;
        hs->modrm_mod = m_mod = c >> 6;
        hs->modrm_rm = m_rm = c & 7;
        hs->modrm_reg = m_reg = (c & 0x3f) >> 3;

        if (x && ((x << m_reg) & 0x80))
            hs->flags |= F_ERROR | F_ERROR_OPCODE;

        if (!hs->opcode2 && opcode >= 0xd9 && opcode <= 0xdf) {
            uint8_t t = opcode - 0xd9;
            if (m_mod == 3) {
                ht = hde64_table + DELTA_FPU_MODRM + t * 8;
                t = ht[m_reg] << m_rm;
            }
            else {
                ht = hde64_table + DELTA_FPU_REG;
                t = ht[t] << m_reg;
            }
            if (t & 0x80)
                hs->flags |= F_ERROR | F_ERROR_OPCODE;
        }

        if (pref & PRE_LOCK) {
            if (m_mod == 3) {
                hs->flags |= F_ERROR | F_ERROR_LOCK;
            }
            else {
                const uint8_t* table_end;
                uint8_t op = opcode;
                if (hs->opcode2) {
                    ht = hde64_table + DELTA_OP2_LOCK_OK;
                    table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
                }
                else {
                    ht = hde64_table + DELTA_OP_LOCK_OK;
                    table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
                    op &= -2;
                }
                for (; ht != table_end; ht++)
                    if (*ht++ == op) {
                        if (!((*ht << m_reg) & 0x80))
                            goto no_lock_error;
                        else
                            break;
                    }
                hs->flags |= F_ERROR | F_ERROR_LOCK;
            no_lock_error:
                ;
            }
        }

        if (hs->opcode2) {
            switch (opcode) {
            case 0x20: case 0x22:
                m_mod = 3;
                if (m_reg > 4 || m_reg == 1)
                    goto error_operand;
                else
                    goto no_error_operand;
            case 0x21: case 0x23:
                m_mod = 3;
                if (m_reg == 4 || m_reg == 5)
                    goto error_operand;
                else
                    goto no_error_operand;
            }
        }
        else {
            switch (opcode) {
            case 0x8c:
                if (m_reg > 5)
                    goto error_operand;
                else
                    goto no_error_operand;
            case 0x8e:
                if (m_reg == 1 || m_reg > 5)
                    goto error_operand;
                else
                    goto no_error_operand;
            }
        }

        if (m_mod == 3) {
            const uint8_t* table_end;
            if (hs->opcode2) {
                ht = hde64_table + DELTA_OP2_ONLY_MEM;
                table_end = ht + sizeof(hde64_table) - DELTA_OP2_ONLY_MEM;
            }
            else {
                ht = hde64_table + DELTA_OP_ONLY_MEM;
                table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
            }
            for (; ht != table_end; ht += 2)
                if (*ht++ == opcode) {
                    if (*ht++ & pref && !((*ht << m_reg) & 0x80))
                        goto error_operand;
                    else
                        break;
                }
            goto no_error_operand;
        }
        else if (hs->opcode2) {
            switch (opcode) {
            case 0x50: case 0xd7: case 0xf7:
                if (pref & (PRE_NONE | PRE_66))
                    goto error_operand;
                break;
            case 0xd6:
                if (pref & (PRE_F2 | PRE_F3))
                    goto error_operand;
                break;
            case 0xc5:
                goto error_operand;
            }
            goto no_error_operand;
        }
        else
            goto no_error_operand;

    error_operand:
        hs->flags |= F_ERROR | F_ERROR_OPERAND;
    no_error_operand:

        c = *p++;
        if (m_reg <= 1) {
            if (opcode == 0xf6)
                cflags |= C_IMM8;
            else if (opcode == 0xf7)
                cflags |= C_IMM_P66;
        }

        switch (m_mod) {
        case 0:
            if (pref & PRE_67) {
                if (m_rm == 6)
                    disp_size = 2;
            }
            else
                if (m_rm == 5)
                    disp_size = 4;
            break;
        case 1:
            disp_size = 1;
            break;
        case 2:
            disp_size = 2;
            if (!(pref & PRE_67))
                disp_size <<= 1;
        }

        if (m_mod != 3 && m_rm == 4) {
            hs->flags |= F_SIB;
            p++;
            hs->sib = c;
            hs->sib_scale = c >> 6;
            hs->sib_index = (c & 0x3f) >> 3;
            if ((hs->sib_base = c & 7) == 5 && !(m_mod & 1))
                disp_size = 4;
        }

        p--;
        switch (disp_size) {
        case 1:
            hs->flags |= F_DISP8;
            hs->disp.disp8 = *p;
            break;
        case 2:
            hs->flags |= F_DISP16;
            hs->disp.disp16 = *(uint16_t*)p;
            break;
        case 4:
            hs->flags |= F_DISP32;
            hs->disp.disp32 = *(uint32_t*)p;
        }
        p += disp_size;
    }
    else if (pref & PRE_LOCK)
        hs->flags |= F_ERROR | F_ERROR_LOCK;

    if (cflags & C_IMM_P66) {
        if (cflags & C_REL32) {
            if (pref & PRE_66) {
                hs->flags |= F_IMM16 | F_RELATIVE;
                hs->imm.imm16 = *(uint16_t*)p;
                p += 2;
                goto disasm_done;
            }
            goto rel32_ok;
        }
        if (op64) {
            hs->flags |= F_IMM64;
            hs->imm.imm64 = *(uint64_t*)p;
            p += 8;
        }
        else if (!(pref & PRE_66)) {
            hs->flags |= F_IMM32;
            hs->imm.imm32 = *(uint32_t*)p;
            p += 4;
        }
        else
            goto imm16_ok;
    }


    if (cflags & C_IMM16) {
    imm16_ok:
        hs->flags |= F_IMM16;
        hs->imm.imm16 = *(uint16_t*)p;
        p += 2;
    }
    if (cflags & C_IMM8) {
        hs->flags |= F_IMM8;
        hs->imm.imm8 = *p++;
    }

    if (cflags & C_REL32) {
    rel32_ok:
        hs->flags |= F_IMM32 | F_RELATIVE;
        hs->imm.imm32 = *(uint32_t*)p;
        p += 4;
    }
    else if (cflags & C_REL8) {
        hs->flags |= F_IMM8 | F_RELATIVE;
        hs->imm.imm8 = *p++;
    }

disasm_done:

    if ((hs->len = (uint8_t)(p - (uint8_t*)code)) > 15) {
        hs->flags |= F_ERROR | F_ERROR_LENGTH;
        hs->len = 15;
    }

    return (unsigned int)hs->len;
}
#pragma warning(pop)

static constexpr uint32_t kFlagModRm = F_MODRM;
static constexpr uint32_t kFlagRelative = F_RELATIVE;
static constexpr uint32_t kFlagError = F_ERROR;

#undef C_NONE
#undef C_MODRM
#undef C_IMM8
#undef C_IMM16
#undef C_IMM_P66
#undef C_REL8
#undef C_REL32
#undef C_GROUP
#undef C_ERROR
#undef PRE_ANY
#undef PRE_NONE
#undef PRE_F2
#undef PRE_F3
#undef PRE_66
#undef PRE_67
#undef PRE_LOCK
#undef PRE_SEG
#undef PRE_ALL
#undef DELTA_OPCODES
#undef DELTA_FPU_REG
#undef DELTA_FPU_MODRM
#undef DELTA_PREFIXES
#undef DELTA_OP_LOCK_OK
#undef DELTA_OP2_LOCK_OK
#undef DELTA_OP_ONLY_MEM
#undef DELTA_OP2_ONLY_MEM
#undef F_MODRM
#undef F_SIB
#undef F_IMM8
#undef F_IMM16
#undef F_IMM32
#undef F_IMM64
#undef F_DISP8
#undef F_DISP16
#undef F_DISP32
#undef F_RELATIVE
#undef F_ERROR
#undef F_ERROR_OPCODE
#undef F_ERROR_LENGTH
#undef F_ERROR_LOCK
#undef F_ERROR_OPERAND
#undef F_PREFIX_REPNZ
#undef F_PREFIX_REPX
#undef F_PREFIX_REP
#undef F_PREFIX_66
#undef F_PREFIX_67
#undef F_PREFIX_LOCK
#undef F_PREFIX_SEG
#undef F_PREFIX_REX
#undef F_PREFIX_ANY
#undef PREFIX_SEGMENT_CS
#undef PREFIX_SEGMENT_SS
#undef PREFIX_SEGMENT_DS
#undef PREFIX_SEGMENT_ES
#undef PREFIX_SEGMENT_FS
#undef PREFIX_SEGMENT_GS
#undef PREFIX_LOCK
#undef PREFIX_REPNZ
#undef PREFIX_REPX
#undef PREFIX_OPERAND_SIZE
#undef PREFIX_ADDRESS_SIZE

} // namespace ExternalHde64



struct HookEntry {
    uintptr_t TargetAddr = 0;
    uintptr_t Trampoline = 0;
    uint8_t OriginalBytes[32] = {};
    size_t PatchSize = 0;
    bool Installed = false;
};

class HookManager {
private:
    struct DecodedInstruction {
        size_t Length = 0;
        bool UsesRipRelative = false;
        bool UsesRelativeControlFlow = false;
    };

    static constexpr size_t kMaxStolenBytes = 32;
    static constexpr size_t kTrampolineSize = 4096;
    static constexpr size_t kAbsJumpSize = 14;
    static constexpr size_t kRelJumpSize = 5;

    static std::vector<HookEntry> s_Hooks;
    static uint32_t s_NextHookId;
    static std::mutex s_Mutex;

    static bool TryMakeRel32(uintptr_t from_next, uintptr_t to, int32_t& out) {
        const int64_t delta = static_cast<int64_t>(to) - static_cast<int64_t>(from_next);
        if (delta < INT32_MIN || delta > INT32_MAX) {
            return false;
        }
        out = static_cast<int32_t>(delta);
        return true;
    }

    static bool SafeReadMemory(uintptr_t address, void* buffer, size_t size) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi))) {
            return false;
        }

        if (mbi.State != MEM_COMMIT) {
            return false;
        }

        const DWORD protect = mbi.Protect;
        if (protect & (PAGE_NOACCESS | PAGE_GUARD)) {
            return false;
        }

        if (!(protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE |
            PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
            return false;
        }

        const uintptr_t regionOffset = address - reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        if (regionOffset + size > mbi.RegionSize) {
            return false;
        }

        std::memcpy(buffer, reinterpret_cast<void*>(address), size);
        return true;
    }

    static bool DecodeInstruction64(const uint8_t* code, size_t maxLen, DecodedInstruction& out) {
        out = {};
        if (!code || maxLen == 0) {
            return false;
        }

        ExternalHde64::hde64s hs = {};
        const unsigned int len = ExternalHde64::hde64_disasm(code, &hs);
        if (len == 0 || len > maxLen || hs.len == 0 || hs.len > 15) {
            return false;
        }
        if (hs.flags & ExternalHde64::kFlagError) {
            return false;
        }

        out.Length = hs.len;

        if ((hs.flags & ExternalHde64::kFlagModRm) && hs.modrm_mod == 0 && hs.modrm_rm == 5) {
            out.UsesRipRelative = true;
        }

        if (hs.flags & ExternalHde64::kFlagRelative) {
            out.UsesRelativeControlFlow = true;
        }

        return true;
    }

    static bool CalculatePatchSize(const uint8_t* code, size_t available, size_t minRequired, size_t& outPatchSize) {
        outPatchSize = 0;
        if (!code || available == 0 || minRequired == 0 || minRequired > available) {
            return false;
        }

        size_t total = 0;
        while (total < minRequired) {
            DecodedInstruction decoded = {};
            if (!DecodeInstruction64(code + total, available - total, decoded)) {
                return false;
            }

            if (decoded.UsesRipRelative || decoded.UsesRelativeControlFlow) {
                return false;
            }

            if (decoded.Length == 0 || total + decoded.Length > available) {
                return false;
            }

            total += decoded.Length;
        }

        outPatchSize = total;
        return true;
    }

    static void* AllocateTrampolineNearTarget(HANDLE hProcess, uintptr_t targetAddr) {
        const uintptr_t preferredAddr = (targetAddr & 0xFFFFFFFF00000000ULL) | 0x10000000ULL;
        void* trampoline = VirtualAllocEx(
            hProcess,
            reinterpret_cast<void*>(preferredAddr),
            kTrampolineSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );

        if (!trampoline) {
            trampoline = VirtualAllocEx(
                hProcess,
                nullptr,
                kTrampolineSize,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE
            );
        }

        return trampoline;
    }

    static bool BuildJumpPatch(uintptr_t from, uintptr_t to, size_t patchSize, uint8_t* outPatch) {
        if (!outPatch || patchSize < kRelJumpSize) {
            return false;
        }

        std::memset(outPatch, 0x90, patchSize);

        int32_t rel = 0;
        if (TryMakeRel32(from + kRelJumpSize, to, rel)) {
            outPatch[0] = 0xE9;
            std::memcpy(outPatch + 1, &rel, sizeof(rel));
            return true;
        }

        if (patchSize < kAbsJumpSize) {
            return false;
        }

        outPatch[0] = 0xFF;
        outPatch[1] = 0x25;
        outPatch[2] = 0x00;
        outPatch[3] = 0x00;
        outPatch[4] = 0x00;
        outPatch[5] = 0x00;
        std::memcpy(outPatch + 6, &to, sizeof(to));
        return true;
    }

public:
    static bool InstallHook(const char* moduleName, uint32_t offset,
        const void* trampolineCode, size_t trampolineCodeSize, uint32_t& outHookId) {

        std::lock_guard<std::mutex> lock(s_Mutex);
        outHookId = UINT32_MAX;

        if (trampolineCodeSize > (kTrampolineSize - kAbsJumpSize)) {
            std::cerr << "[InlineHook] TrampolineCodeSize too large: " << trampolineCodeSize << "\n";
            return false;
        }

        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) {
            std::cerr << "[InlineHook] Module not found: " << moduleName << "\n";
            return false;
        }

        const uintptr_t targetAddr = reinterpret_cast<uintptr_t>(hModule) + offset;

        uint8_t prologueBytes[kMaxStolenBytes] = {};
        if (!SafeReadMemory(targetAddr, prologueBytes, sizeof(prologueBytes))) {
            std::cerr << "[InlineHook] Target address not readable: 0x" << std::hex << targetAddr << std::dec << "\n";
            return false;
        }

        HANDLE hProcess = GetCurrentProcess();
        const uintptr_t trampolineAddr = reinterpret_cast<uintptr_t>(AllocateTrampolineNearTarget(hProcess, targetAddr));
        if (!trampolineAddr) {
            std::cerr << "[InlineHook] Failed to allocate trampoline\n";
            return false;
        }

        int32_t relCheck = 0;
        const bool canUseRel32ToTrampoline = TryMakeRel32(targetAddr + kRelJumpSize, trampolineAddr, relCheck);
        const size_t minPatchRequired = canUseRel32ToTrampoline ? kRelJumpSize : kAbsJumpSize;

        size_t patchSize = 0;
        if (!CalculatePatchSize(prologueBytes, sizeof(prologueBytes), minPatchRequired, patchSize)) {
            std::cerr << "[InlineHook] Unsupported/unsafe prologue, abort install\n";
            VirtualFree(reinterpret_cast<void*>(trampolineAddr), 0, MEM_RELEASE);
            return false;
        }

        HookEntry entry = {};
        entry.TargetAddr = targetAddr;
        entry.Trampoline = trampolineAddr;
        entry.PatchSize = patchSize;
        std::memcpy(entry.OriginalBytes, prologueBytes, patchSize);

        std::cout << "[InlineHook] Target: 0x" << std::hex << targetAddr << std::dec
            << ", Trampoline: 0x" << std::hex << trampolineAddr << std::dec
            << ", PatchSize: " << patchSize << "\n";

        unsigned char* trampoline = reinterpret_cast<unsigned char*>(entry.Trampoline);
        size_t trampOffset = 0;

        std::memcpy(trampoline + trampOffset, entry.OriginalBytes, entry.PatchSize);
        trampOffset += entry.PatchSize;

        if (trampolineCode && trampolineCodeSize > 0) {
            if (trampOffset + trampolineCodeSize + kAbsJumpSize > kTrampolineSize) {
                std::cerr << "[InlineHook] Trampoline code too large\n";
                VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
                return false;
            }
            std::memcpy(trampoline + trampOffset, trampolineCode, trampolineCodeSize);
            trampOffset += trampolineCodeSize;
        }

        if (trampOffset + kAbsJumpSize > kTrampolineSize) {
            std::cerr << "[InlineHook] Trampoline overflow\n";
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            return false;
        }

        uint8_t jumpBackPatch[kAbsJumpSize] = {};
        if (!BuildJumpPatch(entry.Trampoline + trampOffset, entry.TargetAddr + entry.PatchSize, kAbsJumpSize, jumpBackPatch)) {
            std::cerr << "[InlineHook] Failed to build trampoline return jump\n";
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            return false;
        }
        std::memcpy(trampoline + trampOffset, jumpBackPatch, kAbsJumpSize);
        trampOffset += kAbsJumpSize;

        (void)trampOffset;

        uint8_t hookPatch[kMaxStolenBytes] = {};
        if (!BuildJumpPatch(entry.TargetAddr, entry.Trampoline, entry.PatchSize, hookPatch)) {
            std::cerr << "[InlineHook] Failed to build hook jump patch\n";
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            return false;
        }

        bool installSuccess = true;
        bool verifyFailed = false;
        DWORD protectError = ERROR_SUCCESS;
        DWORD oldProtect = 0;

        if (!VirtualProtect(reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            installSuccess = false;
            protectError = GetLastError();
        }
        else {
            std::memcpy(reinterpret_cast<void*>(entry.TargetAddr), hookPatch, entry.PatchSize);

            uint8_t verify[kMaxStolenBytes] = {};
            const bool verifyOk = SafeReadMemory(entry.TargetAddr, verify, entry.PatchSize) &&
                std::memcmp(verify, hookPatch, entry.PatchSize) == 0;

            if (!verifyOk) {
                verifyFailed = true;
                installSuccess = false;
                std::memcpy(reinterpret_cast<void*>(entry.TargetAddr), entry.OriginalBytes, entry.PatchSize);
            }

            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize);
            VirtualProtect(reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize, oldProtect, &oldProtect);
        }

        if (!installSuccess) {
            if (protectError != ERROR_SUCCESS) {
                std::cerr << "[InlineHook] VirtualProtect failed: " << protectError << "\n";
            }
            else if (verifyFailed) {
                std::cerr << "[InlineHook] Patch verification failed, rollback\n";
            }
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            return false;
        }

        entry.Installed = true;
        const uint32_t hookId = s_NextHookId++;
        s_Hooks.push_back(entry);
        outHookId = hookId;

        std::cout << "[InlineHook] Installed hook ID: " << hookId << "\n";
        return true;
    }

    static bool UninstallHook(uint32_t hookId) {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (hookId >= s_Hooks.size()) {
            std::cerr << "[InlineHook] Invalid hook ID: " << hookId << "\n";
            return false;
        }

        HookEntry& entry = s_Hooks[hookId];

        if (!entry.Installed) {
            if (entry.Trampoline) {
                VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
                entry.Trampoline = 0;
            }
            return true;
        }

        bool uninstallSuccess = true;
        DWORD protectError = ERROR_SUCCESS;
        DWORD oldProtect = 0;
        if (!VirtualProtect(reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            uninstallSuccess = false;
            protectError = GetLastError();
        }
        else {
            std::memcpy(reinterpret_cast<void*>(entry.TargetAddr), entry.OriginalBytes, entry.PatchSize);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize);
            VirtualProtect(reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize, oldProtect, &oldProtect);
        }
        if (!uninstallSuccess) {
            std::cerr << "[InlineHook] Uninstall VirtualProtect failed: " << protectError << "\n";
            return false;
        }

        entry.Installed = false;
        if (entry.Trampoline) {
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            entry.Trampoline = 0;
        }

        std::cout << "[InlineHook] Uninstalled hook ID: " << hookId << "\n";
        return true;
    }

    static bool UninstallAll() {
        std::lock_guard<std::mutex> lock(s_Mutex);

        std::cout << "[InlineHook] Uninstalling all hooks...\n";

        bool allSuccess = true;
        uint32_t firstFailedId = UINT32_MAX;
        DWORD firstFailedError = ERROR_SUCCESS;

        for (uint32_t i = 0; i < s_Hooks.size(); ++i) {
            HookEntry& entry = s_Hooks[i];

            if (entry.Installed) {
                DWORD oldProtect = 0;
                if (!VirtualProtect(reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    allSuccess = false;
                    if (firstFailedId == UINT32_MAX) {
                        firstFailedId = i;
                        firstFailedError = GetLastError();
                    }
                    continue;
                }

                std::memcpy(reinterpret_cast<void*>(entry.TargetAddr), entry.OriginalBytes, entry.PatchSize);
                FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize);
                VirtualProtect(reinterpret_cast<void*>(entry.TargetAddr), entry.PatchSize, oldProtect, &oldProtect);
                entry.Installed = false;
            }

            if (entry.Trampoline) {
                VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
                entry.Trampoline = 0;
            }
        }

        if (!allSuccess) {
            std::cerr << "[InlineHook] UninstallAll failed, first error ID " << firstFailedId
                << ", error " << firstFailedError << "\n";
        }

        if (allSuccess) {
            s_Hooks.clear();
            s_NextHookId = 0;
        }

        return allSuccess;
    }
};

inline std::vector<HookEntry> HookManager::s_Hooks;
inline uint32_t HookManager::s_NextHookId = 0;
inline std::mutex HookManager::s_Mutex;

} // namespace InlineHook
