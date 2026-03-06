// InlineHook.cpp — All implementations extracted from InlineHook.hpp
#include "InlineHook.hpp"

namespace InlineHook {
namespace ExternalHde64 {

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

#pragma warning(push)
#pragma warning(disable:4701)
#pragma warning(disable:4706)
#pragma warning(disable:26451)

unsigned int hde64_disasm(const void* code, hde64s* hs)
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

const uint32_t kFlagModRm = F_MODRM;
const uint32_t kFlagRelative = F_RELATIVE;
const uint32_t kFlagError = F_ERROR;
const uint32_t kFlagImm8 = F_IMM8;
const uint32_t kFlagImm16 = F_IMM16;
const uint32_t kFlagImm32 = F_IMM32;
const uint32_t kFlagDisp32 = F_DISP32;

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

// ── HookManager static member definitions ──
std::vector<HookEntry> HookManager::s_Hooks;
uint32_t HookManager::s_NextHookId = 0;
std::mutex HookManager::s_Mutex;

// ── HookManager private helper methods ──

int HookManager::HexNibble(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return -1;
}

bool HookManager::IsPatternSeparator(char ch) {
    const unsigned char uc = static_cast<unsigned char>(ch);
    return std::isspace(uc) != 0 || ch == ',' || ch == ';';
}

bool HookManager::IsWildcardNibble(char ch) {
    return ch == '?' || ch == '*';
}

bool HookManager::IsReadableProtection(DWORD protect) {
    if (protect & (PAGE_GUARD | PAGE_NOACCESS)) {
        return false;
    }
    return (protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
        PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

bool HookManager::IsExecutableProtection(DWORD protect) {
    if (protect & (PAGE_GUARD | PAGE_NOACCESS)) {
        return false;
    }
    return (protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
        PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

bool HookManager::IsScannableRegion(const MEMORY_BASIC_INFORMATION& mbi, bool executableOnly) {
    if (mbi.State != MEM_COMMIT) {
        return false;
    }
    return executableOnly ? IsExecutableProtection(mbi.Protect) : IsReadableProtection(mbi.Protect);
}

bool HookManager::IsReadableSectionCharacteristics(DWORD ch) {
    return (ch & (IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA)) != 0;
}

bool HookManager::MatchMaskedByte(uint8_t data, uint8_t value, uint8_t mask) {
    return ((data ^ value) & mask) == 0;
}

bool HookManager::MatchPatternAt(const uint8_t* data, const ParsedAobPattern& pattern) {
    for (const size_t idx : pattern.CheckOrder) {
        if (!MatchMaskedByte(data[idx], pattern.Bytes[idx], pattern.ByteMasks[idx])) {
            return false;
        }
    }
    return true;
}

bool HookManager::TryMakeRel32(uintptr_t from_next, uintptr_t to, int32_t& out) {
    const int64_t delta = static_cast<int64_t>(to) - static_cast<int64_t>(from_next);
    if (delta < INT32_MIN || delta > INT32_MAX) {
        return false;
    }
    out = static_cast<int32_t>(delta);
    return true;
}

uintptr_t HookManager::AlignDown(uintptr_t value, uintptr_t alignment) {
    if (alignment == 0) {
        return value;
    }
    return value & ~(alignment - 1);
}

uintptr_t HookManager::AlignUp(uintptr_t value, uintptr_t alignment) {
    if (alignment == 0) {
        return value;
    }
    const uintptr_t mask = alignment - 1;
    if (value > ((std::numeric_limits<uintptr_t>::max)() - mask)) {
        return value;
    }
    return (value + mask) & ~mask;
}

bool HookManager::IsShortJccOpcode(uint8_t op0) {
    return (op0 >= 0x70 && op0 <= 0x7F) || op0 == 0xE3 || op0 == 0xEB;
}

bool HookManager::IsNearJccOpcode(uint8_t op0, uint8_t op1) {
    return op0 == 0x0F && (op1 >= 0x80 && op1 <= 0x8F);
}

size_t HookManager::GetInstructionImmSize(uint32_t flags) {
    if (flags & ExternalHde64::kFlagImm32) return 4;
    if (flags & ExternalHde64::kFlagImm16) return 2;
    if (flags & ExternalHde64::kFlagImm8) return 1;
    return 0;
}

bool HookManager::ParseAobPattern(const char* pattern, ParsedAobPattern& outPattern, size_t* outErrorOffset) {
    outPattern = {};
    if (outErrorOffset) {
        *outErrorOffset = 0;
    }

    if (!pattern) {
        return false;
    }

    const char* p = pattern;
    while (*p) {
        while (*p && IsPatternSeparator(*p)) {
            ++p;
        }

        if (!*p) {
            break;
        }

        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            p += 2;
            if (!*p || IsPatternSeparator(*p)) {
                if (outErrorOffset) {
                    *outErrorOffset = static_cast<size_t>(p - pattern);
                }
                return false;
            }
        }

        if (IsWildcardNibble(*p) &&
            (p[1] == '\0' || IsPatternSeparator(p[1]) || IsWildcardNibble(p[1]))) {
            const char wildcardCh = *p++;
            if (*p == wildcardCh) {
                ++p;
            }

            outPattern.Bytes.push_back(0);
            outPattern.ByteMasks.push_back(0x00);

            if (outPattern.Bytes.size() > kMaxAobPatternLength) {
                return false;
            }
            continue;
        }

        const char c0 = p[0];
        if (c0 == '\0') {
            break;
        }
        const char c1 = p[1];
        if (c1 == '\0' || IsPatternSeparator(c1)) {
            if (outErrorOffset) {
                *outErrorOffset = static_cast<size_t>(p - pattern);
            }
            return false;
        }

        const bool hiWild = IsWildcardNibble(c0);
        const bool loWild = IsWildcardNibble(c1);
        const int hi = hiWild ? 0 : HexNibble(c0);
        const int lo = loWild ? 0 : HexNibble(c1);
        if ((!hiWild && hi < 0) || (!loWild && lo < 0)) {
            if (outErrorOffset) {
                *outErrorOffset = static_cast<size_t>(p - pattern);
            }
            return false;
        }

        const uint8_t value = static_cast<uint8_t>((hi << 4) | lo);
        const uint8_t mask = static_cast<uint8_t>((hiWild ? 0x00 : 0xF0) | (loWild ? 0x00 : 0x0F));

        outPattern.Bytes.push_back(static_cast<uint8_t>(value & mask));
        outPattern.ByteMasks.push_back(mask);

        if (outPattern.Bytes.size() > kMaxAobPatternLength) {
            if (outErrorOffset) {
                *outErrorOffset = static_cast<size_t>(p - pattern);
            }
            return false;
        }
        p += 2;
    }

    outPattern.Length = outPattern.Bytes.size();
    if (outPattern.Length == 0) {
        return false;
    }

    for (size_t i = outPattern.Length; i > 0; --i) {
        const size_t idx = i - 1;
        if (outPattern.ByteMasks[idx] == 0xFF) {
            outPattern.FastAnchorIndex = idx;
            outPattern.FastAnchorByte = outPattern.Bytes[idx];
            outPattern.HasFastAnchor = true;
            break;
        }
    }

    outPattern.ShiftTable.fill(1);

    if (outPattern.HasFastAnchor) {
        bool hasWildcardOrMaskedBeforeAnchor = false;
        for (size_t i = 0; i < outPattern.FastAnchorIndex; ++i) {
            if (outPattern.ByteMasks[i] != 0xFF) {
                hasWildcardOrMaskedBeforeAnchor = true;
                break;
            }
        }

        outPattern.CanUseShiftTable = !hasWildcardOrMaskedBeforeAnchor;
        if (outPattern.CanUseShiftTable) {
            const size_t defaultShift = outPattern.FastAnchorIndex + 1;
            outPattern.ShiftTable.fill(defaultShift);

            for (size_t i = 0; i < outPattern.FastAnchorIndex; ++i) {
                if (outPattern.ByteMasks[i] == 0xFF) {
                    outPattern.ShiftTable[outPattern.Bytes[i]] = outPattern.FastAnchorIndex - i;
                }
            }
        }
    }

    outPattern.CheckOrder.reserve(outPattern.Length);

    for (size_t i = outPattern.Length; i > 0; --i) {
        const size_t idx = i - 1;
        if (outPattern.ByteMasks[idx] != 0xFF) {
            continue;
        }
        if (outPattern.HasFastAnchor && idx == outPattern.FastAnchorIndex) {
            continue;
        }
        outPattern.CheckOrder.push_back(idx);
    }

    for (size_t i = outPattern.Length; i > 0; --i) {
        const size_t idx = i - 1;
        const uint8_t mask = outPattern.ByteMasks[idx];
        if (mask == 0x00 || mask == 0xFF) {
            continue;
        }
        outPattern.CheckOrder.push_back(idx);
    }

    return true;
}

void HookManager::ScanBufferFast(const uint8_t* data, size_t dataSize, uintptr_t baseAddress,
    const ParsedAobPattern& pattern, size_t maxResults, std::vector<uintptr_t>& outResults) {

    if (!data || pattern.Length == 0 || dataSize < pattern.Length) {
        return;
    }

    const size_t lastOffset = dataSize - pattern.Length;

    if (!pattern.HasFastAnchor) {
        for (size_t i = 0; i <= lastOffset; ++i) {
            if (MatchPatternAt(data + i, pattern)) {
                outResults.push_back(baseAddress + i);
                if (maxResults > 0 && outResults.size() >= maxResults) {
                    return;
                }
            }
        }
        return;
    }

    const size_t anchor = pattern.FastAnchorIndex;
    const uint8_t anchorByte = pattern.FastAnchorByte;
    size_t i = 0;
    while (i <= lastOffset) {
        const uint8_t probe = data[i + anchor];
        if (probe != anchorByte) {
            i += pattern.CanUseShiftTable ? pattern.ShiftTable[probe] : 1;
            continue;
        }

        if (MatchPatternAt(data + i, pattern)) {
            outResults.push_back(baseAddress + i);
            if (maxResults > 0 && outResults.size() >= maxResults) {
                return;
            }
        }
        ++i;
    }
}

void HookManager::ScanMemoryRange(uintptr_t rangeStart, uintptr_t rangeEnd, const ParsedAobPattern& pattern,
    size_t maxResults, bool executableOnly, std::vector<uintptr_t>& outResults) {

    if (pattern.Length == 0) {
        return;
    }

    if (rangeStart > rangeEnd) {
        const uintptr_t tmp = rangeStart;
        rangeStart = rangeEnd;
        rangeEnd = tmp;
    }

    if (rangeStart >= rangeEnd) {
        return;
    }

    uintptr_t cursor = rangeStart;
    while (cursor < rangeEnd) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(reinterpret_cast<void*>(cursor), &mbi, sizeof(mbi))) {
            constexpr uintptr_t kFallbackStep = 0x1000;
            if (cursor > (std::numeric_limits<uintptr_t>::max)() - kFallbackStep) {
                break;
            }
            cursor += kFallbackStep;
            continue;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        uintptr_t next = regionBase + mbi.RegionSize;
        if (next <= regionBase || next <= cursor) {
            constexpr uintptr_t kFallbackStep = 0x1000;
            if (cursor > (std::numeric_limits<uintptr_t>::max)() - kFallbackStep) {
                break;
            }
            cursor += kFallbackStep;
            continue;
        }

        if (IsScannableRegion(mbi, executableOnly)) {
            const uintptr_t scanBegin = (std::max)(regionBase, rangeStart);
            const uintptr_t scanEnd = (std::min)(next, rangeEnd);
            if (scanEnd > scanBegin) {
                const uint8_t* scanData = reinterpret_cast<const uint8_t*>(scanBegin);
                const size_t scanDataSize = static_cast<size_t>(scanEnd - scanBegin);

                __try {
                    ScanBufferFast(scanData, scanDataSize, scanBegin, pattern, maxResults, outResults);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                }

                if (maxResults > 0 && outResults.size() >= maxResults) {
                    return;
                }
            }
        }

        cursor = next;
    }
}

bool HookManager::SafeReadMemory(uintptr_t address, void* buffer, size_t size) {
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

bool HookManager::SafeWriteMemory(uintptr_t address, const void* data, size_t size, bool flushInstructionCache) {
    if (!data || size == 0) {
        return false;
    }

    uintptr_t cursor = address;
    const uint8_t* src = reinterpret_cast<const uint8_t*>(data);
    size_t remaining = size;

    while (remaining > 0) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(reinterpret_cast<void*>(cursor), &mbi, sizeof(mbi))) {
            return false;
        }

        if (mbi.State != MEM_COMMIT) {
            return false;
        }

        const DWORD protect = mbi.Protect;
        if (protect & (PAGE_NOACCESS | PAGE_GUARD)) {
            return false;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t regionOffset = cursor - regionBase;
        if (regionOffset >= mbi.RegionSize) {
            return false;
        }

        const size_t writableInRegion = static_cast<size_t>(mbi.RegionSize - regionOffset);
        const size_t chunkSize = (std::min)(remaining, writableInRegion);

        DWORD oldProtect = 0;
        if (!VirtualProtect(reinterpret_cast<void*>(cursor), chunkSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }

        std::memcpy(reinterpret_cast<void*>(cursor), src, chunkSize);
        VirtualProtect(reinterpret_cast<void*>(cursor), chunkSize, oldProtect, &oldProtect);

        src += chunkSize;
        cursor += chunkSize;
        remaining -= chunkSize;
    }

    if (flushInstructionCache) {
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(address), size);
    }

    return true;
}

bool HookManager::DecodeInstruction64(const uint8_t* code, size_t maxLen, DecodedInstruction& out) {
    out = {};
    if (!code || maxLen == 0) {
        return false;
    }

    ExternalHde64::hde64s hs = {};
    unsigned int len = ExternalHde64::hde64_disasm(code, &hs);
    if (len == 0 || (hs.flags & ExternalHde64::kFlagError)) {
        return false;
    }
    if (len > maxLen) {
        return false;
    }

    out.Length = len;
    out.UsesRipRelative = (hs.flags & ExternalHde64::kFlagModRm) &&
        (hs.modrm_mod != 3) && (hs.modrm_rm == 5) && (hs.modrm_mod == 0);
    out.UsesRelativeControlFlow = (hs.flags & ExternalHde64::kFlagRelative) != 0;
    return true;
}

bool HookManager::CalculatePatchSize(const uint8_t* code, size_t available, size_t minRequired, size_t& outPatchSize) {
    outPatchSize = 0;
    size_t totalLen = 0;
    while (totalLen < minRequired && totalLen < available) {
        DecodedInstruction insn = {};
        if (!DecodeInstruction64(code + totalLen, available - totalLen, insn)) {
            return false;
        }
        if (insn.Length == 0) {
            return false;
        }
        totalLen += insn.Length;
    }

    if (totalLen < minRequired) {
        return false;
    }

    outPatchSize = totalLen;
    return true;
}

void* HookManager::TryAllocTrampolineAt(HANDLE hProcess, uintptr_t address) {
    return VirtualAllocEx(
        hProcess,
        reinterpret_cast<void*>(address),
        kTrampolineSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
}

void* HookManager::TryAllocNearRel32Trampoline(HANDLE hProcess, uintptr_t targetAddr, uintptr_t candidate) {
    void* trampoline = TryAllocTrampolineAt(hProcess, candidate);
    if (!trampoline) {
        return nullptr;
    }

    int32_t rel = 0;
    if (!TryMakeRel32(targetAddr + kRelJumpSize, reinterpret_cast<uintptr_t>(trampoline), rel)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return nullptr;
    }

    return trampoline;
}

void* HookManager::AllocateTrampolineNearTarget(HANDLE hProcess, uintptr_t targetAddr) {
    SYSTEM_INFO sys = {};
    GetSystemInfo(&sys);

    const uintptr_t granularity = (sys.dwAllocationGranularity > 0)
        ? static_cast<uintptr_t>(sys.dwAllocationGranularity)
        : static_cast<uintptr_t>(0x10000);
    const uintptr_t minApp = reinterpret_cast<uintptr_t>(sys.lpMinimumApplicationAddress);
    const uintptr_t maxApp = reinterpret_cast<uintptr_t>(sys.lpMaximumApplicationAddress);
    const uintptr_t rel32Range = 0x7FFFFFFFULL;

    const uintptr_t lowBound = (targetAddr > rel32Range) ? (targetAddr - rel32Range) : minApp;
    const uintptr_t highBound = (targetAddr < (std::numeric_limits<uintptr_t>::max)() - rel32Range)
        ? (targetAddr + rel32Range)
        : maxApp;
    const uintptr_t highExclusive = (highBound < (std::numeric_limits<uintptr_t>::max)())
        ? (highBound + 1)
        : highBound;

    uintptr_t center = AlignDown(targetAddr, granularity);
    if (center < lowBound) {
        center = AlignDown(lowBound, granularity);
    }
    if (center > highBound) {
        center = AlignDown(highBound, granularity);
    }

    const uintptr_t maxSpanUp = (highBound > center) ? (highBound - center) : 0;
    const uintptr_t maxSpanDown = (center > lowBound) ? (center - lowBound) : 0;
    const uintptr_t maxSpan = (std::max)(maxSpanUp, maxSpanDown);

    for (uintptr_t span = 0; span <= maxSpan; span += granularity) {
        uintptr_t candidates[2] = {};
        size_t candidateCount = 0;

        if (span <= maxSpanUp) {
            candidates[candidateCount++] = center + span;
        }
        if (span != 0 && span <= maxSpanDown) {
            candidates[candidateCount++] = center - span;
        }

        for (size_t i = 0; i < candidateCount; ++i) {
            const uintptr_t candidate = candidates[i];
            if (candidate < lowBound || candidate > highBound) {
                continue;
            }

            void* trampoline = TryAllocNearRel32Trampoline(hProcess, targetAddr, candidate);
            if (!trampoline) {
                continue;
            }
            return trampoline;
        }
    }

    // Fallback pass: enumerate MEM_FREE regions in rel32 window
    uintptr_t cursor = lowBound;
    while (cursor < highExclusive) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(reinterpret_cast<void*>(cursor), &mbi, sizeof(mbi))) {
            if (cursor > (std::numeric_limits<uintptr_t>::max)() - granularity) {
                break;
            }
            cursor += granularity;
            continue;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        uintptr_t regionEnd = regionBase + mbi.RegionSize;
        if (regionEnd <= regionBase || regionEnd <= cursor) {
            if (cursor > (std::numeric_limits<uintptr_t>::max)() - granularity) {
                break;
            }
            cursor += granularity;
            continue;
        }

        const uintptr_t scanBegin = (std::max)(regionBase, lowBound);
        const uintptr_t scanEnd = (std::min)(regionEnd, highExclusive);

        if (mbi.State == MEM_FREE && scanEnd > scanBegin && (scanEnd - scanBegin) >= kTrampolineSize) {
            const uintptr_t probeMin = AlignUp(scanBegin, granularity);
            const uintptr_t probeMax = AlignDown(scanEnd - kTrampolineSize, granularity);

            if (probeMin <= probeMax) {
                uintptr_t preferred = AlignDown(targetAddr, granularity);
                if (preferred < probeMin) preferred = probeMin;
                if (preferred > probeMax) preferred = probeMax;

                const uintptr_t upSpan = (probeMax > preferred) ? (probeMax - preferred) : 0;
                const uintptr_t downSpan = (preferred > probeMin) ? (preferred - probeMin) : 0;
                const uintptr_t probeSpan = (std::max)(upSpan, downSpan);

                for (uintptr_t probeS = 0; probeS <= probeSpan; probeS += granularity) {
                    uintptr_t probes[2] = {};
                    size_t probeCount = 0;

                    if (probeS <= upSpan) {
                        probes[probeCount++] = preferred + probeS;
                    }
                    if (probeS != 0 && probeS <= downSpan) {
                        probes[probeCount++] = preferred - probeS;
                    }

                    for (size_t i = 0; i < probeCount; ++i) {
                        void* trampoline = TryAllocNearRel32Trampoline(hProcess, targetAddr, probes[i]);
                        if (trampoline) {
                            return trampoline;
                        }
                    }
                }
            }
        }

        cursor = regionEnd;
    }

    return nullptr;
}

bool HookManager::BuildJumpPatch(uintptr_t from, uintptr_t to, size_t patchSize, uint8_t* outPatch) {
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

bool HookManager::BuildAbsJumpPatch(uintptr_t to, size_t patchSize, uint8_t* outPatch) {
    if (!outPatch || patchSize < kAbsJumpSize) {
        return false;
    }

    std::memset(outPatch, 0x90, patchSize);
    outPatch[0] = 0xFF;
    outPatch[1] = 0x25;
    outPatch[2] = 0x00;
    outPatch[3] = 0x00;
    outPatch[4] = 0x00;
    outPatch[5] = 0x00;
    std::memcpy(outPatch + 6, &to, sizeof(to));
    return true;
}

bool HookManager::AppendBytesToTrampoline(unsigned char* trampoline, size_t& trampOffset, const void* data, size_t len) {
    if (!trampoline || !data || len == 0) {
        return false;
    }
    if (trampOffset + len > kTrampolineSize) {
        return false;
    }
    std::memcpy(trampoline + trampOffset, data, len);
    trampOffset += len;
    return true;
}

bool HookManager::AppendAbsJumpToTrampoline(unsigned char* trampoline, size_t& trampOffset, uintptr_t to) {
    uint8_t patch[kAbsJumpSize] = {};
    if (!BuildAbsJumpPatch(to, sizeof(patch), patch)) {
        return false;
    }
    return AppendBytesToTrampoline(trampoline, trampOffset, patch, sizeof(patch));
}

bool HookManager::AppendAbsCallToTrampoline(unsigned char* trampoline, size_t& trampOffset, uintptr_t to) {
    // mov r11, imm64 ; call r11
    const uint8_t seqPrefix[] = { 0x49, 0xBB };
    const uint8_t seqCall[] = { 0x41, 0xFF, 0xD3 };
    if (!AppendBytesToTrampoline(trampoline, trampOffset, seqPrefix, sizeof(seqPrefix))) {
        return false;
    }
    if (!AppendBytesToTrampoline(trampoline, trampOffset, &to, sizeof(to))) {
        return false;
    }
    if (!AppendBytesToTrampoline(trampoline, trampOffset, seqCall, sizeof(seqCall))) {
        return false;
    }
    return true;
}

bool HookManager::RelocateRelativeInstruction(
    const uint8_t* srcInsn,
    const ExternalHde64::hde64s& hs,
    uintptr_t srcInsnAddr,
    uintptr_t dstInsnAddr,
    unsigned char* trampoline,
    size_t& trampOffset) {

    if (!srcInsn || hs.len == 0) {
        return false;
    }

    const uint8_t op0 = srcInsn[0];
    const uint8_t op1 = (hs.len > 1) ? srcInsn[1] : 0;
    const size_t immSize = GetInstructionImmSize(hs.flags);
    if (immSize == 0 || immSize > hs.len) {
        return false;
    }

    int64_t oldDisp = 0;
    if (immSize == 1) {
        oldDisp = static_cast<int8_t>(srcInsn[hs.len - 1]);
    }
    else if (immSize == 2) {
        int16_t v = 0;
        std::memcpy(&v, srcInsn + hs.len - 2, sizeof(v));
        oldDisp = v;
    }
    else if (immSize == 4) {
        int32_t v = 0;
        std::memcpy(&v, srcInsn + hs.len - 4, sizeof(v));
        oldDisp = v;
    }
    else {
        return false;
    }

    const uintptr_t branchTarget = static_cast<uintptr_t>(static_cast<int64_t>(srcInsnAddr + hs.len) + oldDisp);

    // CALL rel
    if (op0 == 0xE8) {
        int32_t rel32 = 0;
        uint8_t seq[5] = { 0xE8, 0, 0, 0, 0 };
        if (TryMakeRel32(dstInsnAddr + sizeof(seq), branchTarget, rel32)) {
            std::memcpy(seq + 1, &rel32, sizeof(rel32));
            return AppendBytesToTrampoline(trampoline, trampOffset, seq, sizeof(seq));
        }
        return AppendAbsCallToTrampoline(trampoline, trampOffset, branchTarget);
    }

    // JMP rel8/rel32
    if (op0 == 0xE9 || op0 == 0xEB) {
        int32_t rel32 = 0;
        uint8_t seq[5] = { 0xE9, 0, 0, 0, 0 };
        if (TryMakeRel32(dstInsnAddr + sizeof(seq), branchTarget, rel32)) {
            std::memcpy(seq + 1, &rel32, sizeof(rel32));
            return AppendBytesToTrampoline(trampoline, trampOffset, seq, sizeof(seq));
        }
        return AppendAbsJumpToTrampoline(trampoline, trampOffset, branchTarget);
    }

    // Jcc short/near
    if (IsShortJccOpcode(op0) || IsNearJccOpcode(op0, op1)) {
        uint8_t cond = 0;
        if (IsShortJccOpcode(op0)) {
            cond = static_cast<uint8_t>(op0 & 0x0F);
        }
        else {
            cond = static_cast<uint8_t>(op1 & 0x0F);
        }

        int32_t rel32 = 0;
        uint8_t nearJcc[6] = { 0x0F, static_cast<uint8_t>(0x80 | cond), 0, 0, 0, 0 };
        if (TryMakeRel32(dstInsnAddr + sizeof(nearJcc), branchTarget, rel32)) {
            std::memcpy(nearJcc + 2, &rel32, sizeof(rel32));
            return AppendBytesToTrampoline(trampoline, trampOffset, nearJcc, sizeof(nearJcc));
        }

        // Fallback: inverse-jcc skip abs-jmp
        const uint8_t invCond = static_cast<uint8_t>(cond ^ 0x1);
        if (IsShortJccOpcode(op0)) {
            const uint8_t invShort[2] = { static_cast<uint8_t>(0x70 | invCond), 0x0E };
            if (!AppendBytesToTrampoline(trampoline, trampOffset, invShort, sizeof(invShort))) {
                return false;
            }
            return AppendAbsJumpToTrampoline(trampoline, trampOffset, branchTarget);
        }

        const uint8_t invNearPrefix[2] = { 0x0F, static_cast<uint8_t>(0x80 | invCond) };
        int32_t skip = 14;
        if (!AppendBytesToTrampoline(trampoline, trampOffset, invNearPrefix, sizeof(invNearPrefix))) {
            return false;
        }
        if (!AppendBytesToTrampoline(trampoline, trampOffset, &skip, sizeof(skip))) {
            return false;
        }
        return AppendAbsJumpToTrampoline(trampoline, trampOffset, branchTarget);
    }

    return false;
}

bool HookManager::RelocateRipRelativeDisp32(
    const uint8_t* srcInsn,
    const ExternalHde64::hde64s& hs,
    uintptr_t srcInsnAddr,
    uintptr_t dstInsnAddr,
    unsigned char* trampoline,
    size_t& trampOffset) {

    if (!srcInsn || hs.len == 0 || hs.len > 15) {
        return false;
    }

    uint8_t buf[16] = {};
    std::memcpy(buf, srcInsn, hs.len);

    const size_t immSize = GetInstructionImmSize(hs.flags);
    if (hs.len < (4 + immSize)) {
        return false;
    }
    const size_t dispOff = hs.len - immSize - 4;

    int32_t oldDisp = 0;
    std::memcpy(&oldDisp, buf + dispOff, sizeof(oldDisp));
    const uintptr_t memTarget = static_cast<uintptr_t>(static_cast<int64_t>(srcInsnAddr + hs.len) + oldDisp);

    int32_t newDisp = 0;
    if (!TryMakeRel32(dstInsnAddr + hs.len, memTarget, newDisp)) {
        return false;
    }
    std::memcpy(buf + dispOff, &newDisp, sizeof(newDisp));

    return AppendBytesToTrampoline(trampoline, trampOffset, buf, hs.len);
}

bool HookManager::BuildRelocatedOriginalCode(
    uintptr_t originalBase,
    const uint8_t* originalBytes,
    size_t originalLen,
    uintptr_t trampolineBase,
    unsigned char* trampoline,
    size_t& outWritten) {

    if (!originalBytes || !trampoline || originalLen == 0) {
        return false;
    }

    size_t writeOffset = outWritten;
    size_t consumed = 0;
    while (consumed < originalLen) {
        ExternalHde64::hde64s hs = {};
        const uint8_t* srcInsn = originalBytes + consumed;
        const unsigned int len = ExternalHde64::hde64_disasm(srcInsn, &hs);
        if (len == 0 || hs.len == 0 || hs.len > 15 || (hs.flags & ExternalHde64::kFlagError)) {
            return false;
        }
        if (consumed + hs.len > originalLen) {
            return false;
        }

        const uintptr_t srcInsnAddr = originalBase + consumed;
        const uintptr_t dstInsnAddr = trampolineBase + writeOffset;

        bool handled = false;
        if (hs.flags & ExternalHde64::kFlagRelative) {
            handled = RelocateRelativeInstruction(srcInsn, hs, srcInsnAddr, dstInsnAddr, trampoline, writeOffset);
        }
        else if ((hs.flags & ExternalHde64::kFlagModRm) && hs.modrm_mod == 0 && hs.modrm_rm == 5 &&
            (hs.flags & ExternalHde64::kFlagDisp32)) {
            handled = RelocateRipRelativeDisp32(srcInsn, hs, srcInsnAddr, dstInsnAddr, trampoline, writeOffset);
        }
        else {
            handled = AppendBytesToTrampoline(trampoline, writeOffset, srcInsn, hs.len);
        }

        if (!handled) {
            return false;
        }

        consumed += hs.len;
    }

    outWritten = writeOffset;
    return true;
}

// ── HookManager public methods ──

bool HookManager::ReadMemory(uintptr_t address, void* outBuffer, size_t size) {
    if (!outBuffer || size == 0) {
        return false;
    }
    return SafeReadMemory(address, outBuffer, size);
}

std::vector<uint8_t> HookManager::ReadMemory(uintptr_t address, size_t size) {
    std::vector<uint8_t> bytes;
    if (size == 0) {
        return bytes;
    }

    bytes.resize(size);
    if (!SafeReadMemory(address, bytes.data(), size)) {
        bytes.clear();
    }
    return bytes;
}

bool HookManager::WriteMemory(uintptr_t address, const void* data, std::size_t numBytes, bool flushInstructionCache) {
    return SafeWriteMemory(address, data, numBytes, flushInstructionCache);
}

std::vector<uintptr_t> HookManager::AobScan(const char* pattern, size_t maxResults,
    uintptr_t rangeStart, uintptr_t rangeEnd, bool executableOnly) {

    ParsedAobPattern parsed = {};
    size_t errorOffset = 0;
    if (!ParseAobPattern(pattern, parsed, &errorOffset)) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScan pattern parse failed: "
            << (pattern ? pattern : "<null>") << " at offset " << errorOffset << "\n";
        return {};
    }

    SYSTEM_INFO sys = {};
    GetSystemInfo(&sys);

    const uintptr_t minAddress = reinterpret_cast<uintptr_t>(sys.lpMinimumApplicationAddress);
    const uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sys.lpMaximumApplicationAddress);
    const uintptr_t fullEndExclusive = (maxAddress < (std::numeric_limits<uintptr_t>::max)())
        ? (maxAddress + 1)
        : maxAddress;

    uintptr_t start = (rangeStart == 0) ? minAddress : (std::max)(rangeStart, minAddress);
    uintptr_t end = (rangeEnd == 0) ? fullEndExclusive : (std::min)(rangeEnd, fullEndExclusive);
    if (start > end) {
        const uintptr_t tmp = start;
        start = end;
        end = tmp;
        start = (std::max)(start, minAddress);
        end = (std::min)(end, fullEndExclusive);
    }

    if (start >= end) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScan invalid range: start=0x"
            << std::hex << rangeStart << " end=0x" << rangeEnd << std::dec << "\n";
        return {};
    }

    if (maxResults == 0 && parsed.CheckOrder.empty()) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScan rejected: fully wildcard pattern with unlimited results\n";
        return {};
    }

    std::vector<uintptr_t> results;
    if (maxResults > 0) {
        results.reserve((std::min)(maxResults, static_cast<size_t>(4096)));
    }

    ScanMemoryRange(start, end, parsed, maxResults, executableOnly, results);
    return results;
}

uintptr_t HookManager::AobScanFirst(const char* pattern, uintptr_t rangeStart,
    uintptr_t rangeEnd, bool executableOnly) {

    std::vector<uintptr_t> hits = AobScan(pattern, 1, rangeStart, rangeEnd, executableOnly);
    return hits.empty() ? 0 : hits[0];
}

std::vector<uintptr_t> HookManager::AobScanModule(const char* moduleName, const char* pattern,
    size_t maxResults, bool executableOnly) {

    if (!moduleName || !*moduleName) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule invalid module name\n";
        return {};
    }

    ParsedAobPattern parsed = {};
    size_t errorOffset = 0;
    if (!ParseAobPattern(pattern, parsed, &errorOffset)) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule pattern parse failed: "
            << (pattern ? pattern : "<null>") << " at offset " << errorOffset << "\n";
        return {};
    }

    HMODULE module = GetModuleHandleA(moduleName);
    if (!module) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule module not found: " << moduleName << "\n";
        return {};
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule DOS header invalid: " << moduleName << "\n";
        return {};
    }
    if (dos->e_lfanew <= 0 || dos->e_lfanew > 0x100000) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule DOS e_lfanew invalid: " << moduleName << "\n";
        return {};
    }

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + static_cast<uintptr_t>(dos->e_lfanew));
    if (!nt || nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage == 0) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule NT header invalid: " << moduleName << "\n";
        return {};
    }

    const uintptr_t moduleEnd = base + static_cast<uintptr_t>(nt->OptionalHeader.SizeOfImage);
    if (moduleEnd <= base) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule image range overflow: " << moduleName << "\n";
        return {};
    }

    if (maxResults == 0 && parsed.CheckOrder.empty()) {
        LOGE_STREAM("InlineHook") << "[InlineHook] AobScanModule rejected: fully wildcard pattern with unlimited results\n";
        return {};
    }

    std::vector<uintptr_t> results;
    if (maxResults > 0) {
        results.reserve((std::min)(maxResults, static_cast<size_t>(4096)));
    }

    bool usedSectionScan = false;
    const uint16_t sectionCount = nt->FileHeader.NumberOfSections;
    if (sectionCount > 0 && sectionCount < 512) {
        const uintptr_t sectionTableStart = reinterpret_cast<uintptr_t>(IMAGE_FIRST_SECTION(nt));
        const uintptr_t sectionTableEnd = sectionTableStart +
            static_cast<uintptr_t>(sectionCount) * sizeof(IMAGE_SECTION_HEADER);

        if (sectionTableStart >= base && sectionTableStart < moduleEnd &&
            sectionTableEnd > sectionTableStart && sectionTableEnd <= moduleEnd) {
            const IMAGE_SECTION_HEADER* section = reinterpret_cast<const IMAGE_SECTION_HEADER*>(sectionTableStart);
            for (uint16_t i = 0; i < sectionCount; ++i, ++section) {
                const DWORD ch = section->Characteristics;
                if (executableOnly) {
                    if ((ch & IMAGE_SCN_MEM_EXECUTE) == 0) {
                        continue;
                    }
                }
                else if (!IsReadableSectionCharacteristics(ch)) {
                    continue;
                }

                const uintptr_t secStart = base + static_cast<uintptr_t>(section->VirtualAddress);
                const size_t secSize = (std::max)(
                    static_cast<size_t>(section->Misc.VirtualSize),
                    static_cast<size_t>(section->SizeOfRawData));
                if (secSize == 0) {
                    continue;
                }

                uintptr_t secEnd = secStart + static_cast<uintptr_t>(secSize);
                if (secEnd <= secStart) {
                    continue;
                }
                if (secStart < base || secStart >= moduleEnd) {
                    continue;
                }
                if (secEnd > moduleEnd) {
                    secEnd = moduleEnd;
                }

                usedSectionScan = true;
                ScanMemoryRange(secStart, secEnd, parsed, maxResults, executableOnly, results);
                if (maxResults > 0 && results.size() >= maxResults) {
                    return results;
                }
            }
        }
    }

    if (!usedSectionScan) {
        ScanMemoryRange(base, moduleEnd, parsed, maxResults, executableOnly, results);
    }

    return results;
}

uintptr_t HookManager::AobScanModuleFirst(const char* moduleName, const char* pattern,
    bool executableOnly) {

    std::vector<uintptr_t> hits = AobScanModule(moduleName, pattern, 1, executableOnly);
    return hits.empty() ? 0 : hits[0];
}

uintptr_t HookManager::ScanModulePatternRobust(const char* moduleName, const char* pattern) {
    if (!moduleName || !pattern)
        return 0;

    uintptr_t addr = AobScanModuleFirst(moduleName, pattern, true);
    if (addr != 0)
        return addr;

    addr = AobScanModuleFirst(moduleName, pattern, false);
    if (addr != 0)
        return addr;

    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule)
        return 0;

    const uintptr_t base = reinterpret_cast<uintptr_t>(hModule);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + static_cast<uintptr_t>(dos->e_lfanew));
    if (!nt || nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage == 0)
        return 0;

    const uintptr_t end = base + static_cast<uintptr_t>(nt->OptionalHeader.SizeOfImage);
    if (end <= base)
        return 0;

    return AobScanFirst(pattern, base, end, false);
}

bool HookManager::InstallHook(const char* moduleName, uint32_t offset,
    const void* trampolineCode, size_t trampolineCodeSize, uint32_t& outHookId,
    bool executeUserCodeFirst, bool allowAbsEntryFallback,
    bool appendRelocatedOriginalCode) {

    std::lock_guard<std::mutex> lock(s_Mutex);
    outHookId = UINT32_MAX;

    if (trampolineCodeSize > (kTrampolineSize - kAbsJumpSize)) {
        LOGE_STREAM("InlineHook") << "[InlineHook] TrampolineCodeSize too large: " << trampolineCodeSize << "\n";
        return false;
    }

    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) {
        LOGE_STREAM("InlineHook") << "[InlineHook] Module not found: " << moduleName << "\n";
        return false;
    }

    const uintptr_t targetAddr = reinterpret_cast<uintptr_t>(hModule) + offset;

    uint8_t prologueBytes[kMaxStolenBytes] = {};
    if (!SafeReadMemory(targetAddr, prologueBytes, sizeof(prologueBytes))) {
        LOGE_STREAM("InlineHook") << "[InlineHook] Target address not readable: 0x" << std::hex << targetAddr << std::dec << "\n";
        return false;
    }

    HANDLE hProcess = GetCurrentProcess();
    uintptr_t trampolineAddr = reinterpret_cast<uintptr_t>(AllocateTrampolineNearTarget(hProcess, targetAddr));
    bool requireRel32Hook = true;
    if (!trampolineAddr) {
        if (!allowAbsEntryFallback) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Near trampoline not found and absolute-entry fallback is disabled\n";
            return false;
        }
        void* farTrampoline = VirtualAllocEx(
            hProcess,
            nullptr,
            kTrampolineSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);
        trampolineAddr = reinterpret_cast<uintptr_t>(farTrampoline);
        requireRel32Hook = false;
        if (!trampolineAddr) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Failed to allocate trampoline (near and far both failed)\n";
            return false;
        }
        LOGW_STREAM("InlineHook") << "[InlineHook] Near trampoline not found, fallback to absolute-entry hook\n";
    }

    const size_t minPatchRequired = requireRel32Hook ? kRelJumpSize : kAbsJumpSize;

    size_t patchSize = 0;
    if (!CalculatePatchSize(prologueBytes, sizeof(prologueBytes), minPatchRequired, patchSize)) {
        LOGE_STREAM("InlineHook") << "[InlineHook] Failed to calculate patch size (decode/boundary), abort install\n";
        VirtualFree(reinterpret_cast<void*>(trampolineAddr), 0, MEM_RELEASE);
        return false;
    }

    HookEntry entry = {};
    entry.TargetAddr = targetAddr;
    entry.Trampoline = trampolineAddr;
    entry.PatchSize = patchSize;
    std::memcpy(entry.OriginalBytes, prologueBytes, patchSize);

    LOGI_STREAM("InlineHook") << "[InlineHook] Target: 0x" << std::hex << targetAddr << std::dec
        << ", Trampoline: 0x" << std::hex << trampolineAddr << std::dec
        << ", PatchSize: " << patchSize << "\n";

    unsigned char* trampoline = reinterpret_cast<unsigned char*>(entry.Trampoline);
    size_t trampOffset = 0;

    auto appendUserCode = [&]() -> bool {
        if (!trampolineCode || trampolineCodeSize == 0) {
            return true;
        }
        if (trampOffset + trampolineCodeSize + kAbsJumpSize > kTrampolineSize) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Trampoline code too large\n";
            return false;
        }
        std::memcpy(trampoline + trampOffset, trampolineCode, trampolineCodeSize);
        trampOffset += trampolineCodeSize;
        return true;
    };

    auto appendRelocatedOriginal = [&]() -> bool {
        if (!BuildRelocatedOriginalCode(entry.TargetAddr, entry.OriginalBytes, entry.PatchSize,
            entry.Trampoline, trampoline, trampOffset)) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Failed to relocate original instructions into trampoline\n";
            return false;
        }
        return true;
    };

    bool appendOk = false;
    if (appendRelocatedOriginalCode) {
        appendOk = executeUserCodeFirst
            ? (appendUserCode() && appendRelocatedOriginal())
            : (appendRelocatedOriginal() && appendUserCode());
    }
    else {
        appendOk = appendUserCode();
    }
    if (!appendOk) {
        VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
        return false;
    }

    if (trampOffset + kAbsJumpSize > kTrampolineSize) {
        LOGE_STREAM("InlineHook") << "[InlineHook] Trampoline overflow\n";
        VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
        return false;
    }

    uint8_t jumpBackPatch[kAbsJumpSize] = {};
    if (!BuildJumpPatch(entry.Trampoline + trampOffset, entry.TargetAddr + entry.PatchSize, kAbsJumpSize, jumpBackPatch)) {
        LOGE_STREAM("InlineHook") << "[InlineHook] Failed to build trampoline return jump\n";
        VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
        return false;
    }
    std::memcpy(trampoline + trampOffset, jumpBackPatch, kAbsJumpSize);
    trampOffset += kAbsJumpSize;

    (void)trampOffset;

    uint8_t hookPatch[kMaxStolenBytes] = {};
    if (requireRel32Hook) {
        if (!BuildJumpPatch(entry.TargetAddr, entry.Trampoline, entry.PatchSize, hookPatch)) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Failed to build hook jump patch\n";
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            return false;
        }
        if (hookPatch[0] != 0xE9) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Hook patch is not rel32 (5-byte) jump, abort install\n";
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            return false;
        }
    }
    else {
        if (!BuildAbsJumpPatch(entry.Trampoline, entry.PatchSize, hookPatch)) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Failed to build absolute hook patch\n";
            VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
            return false;
        }
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
            LOGE_STREAM("InlineHook") << "[InlineHook] VirtualProtect failed: " << protectError << "\n";
        }
        else if (verifyFailed) {
            LOGE_STREAM("InlineHook") << "[InlineHook] Patch verification failed, rollback\n";
        }
        VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
        return false;
    }

    entry.Installed = true;
    const uint32_t hookId = s_NextHookId++;
    s_Hooks.push_back(entry);
    outHookId = hookId;

    LOGI_STREAM("InlineHook") << "[InlineHook] Installed hook ID: " << hookId << "\n";
    return true;
}

bool HookManager::UninstallHook(uint32_t hookId) {
    std::lock_guard<std::mutex> lock(s_Mutex);

    if (hookId >= s_Hooks.size()) {
        LOGE_STREAM("InlineHook") << "[InlineHook] Invalid hook ID: " << hookId << "\n";
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
        LOGE_STREAM("InlineHook") << "[InlineHook] Uninstall VirtualProtect failed: " << protectError << "\n";
        return false;
    }

    entry.Installed = false;
    if (entry.Trampoline) {
        VirtualFree(reinterpret_cast<void*>(entry.Trampoline), 0, MEM_RELEASE);
        entry.Trampoline = 0;
    }

    LOGI_STREAM("InlineHook") << "[InlineHook] Uninstalled hook ID: " << hookId << "\n";
    return true;
}

bool HookManager::UninstallAll() {
    std::lock_guard<std::mutex> lock(s_Mutex);

    LOGI_STREAM("InlineHook") << "[InlineHook] Uninstalling all hooks...\n";

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
        LOGE_STREAM("InlineHook") << "[InlineHook] UninstallAll failed, first error ID " << firstFailedId
            << ", error " << firstFailedError << "\n";
    }

    if (allSuccess) {
        s_Hooks.clear();
        s_NextHookId = 0;
    }

    return allSuccess;
}

} // namespace InlineHook
