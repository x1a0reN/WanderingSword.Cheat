#pragma once

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <array>
#include <algorithm>
#include <cctype>
#include <limits>
#include "Inject/Utility/Logging.hpp"
#include <mutex>
#include <vector>

namespace InlineHook {
namespace ExternalHde64 {

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

// Declared in InlineHook.cpp
unsigned int hde64_disasm(const void* code, hde64s* hs);

// kFlag constants (values defined in .cpp, declared here for external use)
extern const uint32_t kFlagModRm;
extern const uint32_t kFlagRelative;
extern const uint32_t kFlagError;
extern const uint32_t kFlagImm8;
extern const uint32_t kFlagImm16;
extern const uint32_t kFlagImm32;
extern const uint32_t kFlagDisp32;

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

    struct ParsedAobPattern {
        std::vector<uint8_t> Bytes;
        std::vector<uint8_t> ByteMasks;
        std::vector<size_t> CheckOrder;
        std::array<size_t, 256> ShiftTable{};
        size_t Length = 0;
        size_t FastAnchorIndex = 0;
        uint8_t FastAnchorByte = 0;
        bool HasFastAnchor = false;
        bool CanUseShiftTable = false;
    };

    static constexpr size_t kMaxStolenBytes = 32;
    static constexpr size_t kTrampolineSize = 4096;
    static constexpr size_t kAbsJumpSize = 14;
    static constexpr size_t kRelJumpSize = 5;
    static constexpr size_t kMaxAobPatternLength = 8192;

    static std::vector<HookEntry> s_Hooks;
    static uint32_t s_NextHookId;
    static std::mutex s_Mutex;

    // Private helpers — all implemented in InlineHook.cpp
    static int HexNibble(char ch);
    static bool IsPatternSeparator(char ch);
    static bool IsWildcardNibble(char ch);
    static bool ParseAobPattern(const char* pattern, ParsedAobPattern& outPattern, size_t* outErrorOffset = nullptr);
    static bool IsReadableProtection(DWORD protect);
    static bool IsExecutableProtection(DWORD protect);
    static bool IsScannableRegion(const MEMORY_BASIC_INFORMATION& mbi, bool executableOnly);
    static bool IsReadableSectionCharacteristics(DWORD ch);
    static bool MatchMaskedByte(uint8_t data, uint8_t value, uint8_t mask);
    static bool MatchPatternAt(const uint8_t* data, const ParsedAobPattern& pattern);
    static void ScanBufferFast(const uint8_t* data, size_t dataSize, uintptr_t baseAddress,
        const ParsedAobPattern& pattern, size_t maxResults, std::vector<uintptr_t>& outResults);
    static void ScanMemoryRange(uintptr_t rangeStart, uintptr_t rangeEnd, const ParsedAobPattern& pattern,
        size_t maxResults, bool executableOnly, std::vector<uintptr_t>& outResults);
    static bool TryMakeRel32(uintptr_t from_next, uintptr_t to, int32_t& out);
    static bool SafeReadMemory(uintptr_t address, void* buffer, size_t size);
    static bool SafeWriteMemory(uintptr_t address, const void* data, size_t size, bool flushInstructionCache = true);
    static bool DecodeInstruction64(const uint8_t* code, size_t maxLen, DecodedInstruction& out);
    static bool CalculatePatchSize(const uint8_t* code, size_t available, size_t minRequired, size_t& outPatchSize);
    static uintptr_t AlignDown(uintptr_t value, uintptr_t alignment);
    static uintptr_t AlignUp(uintptr_t value, uintptr_t alignment);
    static void* TryAllocTrampolineAt(HANDLE hProcess, uintptr_t address);
    static void* TryAllocNearRel32Trampoline(HANDLE hProcess, uintptr_t targetAddr, uintptr_t candidate);
    static void* AllocateTrampolineNearTarget(HANDLE hProcess, uintptr_t targetAddr);
    static bool BuildJumpPatch(uintptr_t from, uintptr_t to, size_t patchSize, uint8_t* outPatch);
    static bool BuildAbsJumpPatch(uintptr_t to, size_t patchSize, uint8_t* outPatch);
    static size_t GetInstructionImmSize(uint32_t flags);
    static bool IsShortJccOpcode(uint8_t op0);
    static bool IsNearJccOpcode(uint8_t op0, uint8_t op1);
    static bool AppendBytesToTrampoline(unsigned char* trampoline, size_t& trampOffset, const void* data, size_t len);
    static bool AppendAbsJumpToTrampoline(unsigned char* trampoline, size_t& trampOffset, uintptr_t to);
    static bool AppendAbsCallToTrampoline(unsigned char* trampoline, size_t& trampOffset, uintptr_t to);
    static bool RelocateRelativeInstruction(
        const uint8_t* srcInsn, const ExternalHde64::hde64s& hs,
        uintptr_t srcInsnAddr, uintptr_t dstInsnAddr,
        unsigned char* trampoline, size_t& trampOffset);
    static bool RelocateRipRelativeDisp32(
        const uint8_t* srcInsn, const ExternalHde64::hde64s& hs,
        uintptr_t srcInsnAddr, uintptr_t dstInsnAddr,
        unsigned char* trampoline, size_t& trampOffset);
    static bool BuildRelocatedOriginalCode(
        uintptr_t originalBase, const uint8_t* originalBytes, size_t originalLen,
        uintptr_t trampolineBase, unsigned char* trampoline, size_t& outWritten);

public:
    // Memory read/write
    static bool ReadMemory(uintptr_t address, void* outBuffer, size_t size);
    static std::vector<uint8_t> ReadMemory(uintptr_t address, size_t size);

    template <typename TValue>
    static bool ReadValue(uintptr_t address, TValue& outValue) {
        return SafeReadMemory(address, &outValue, sizeof(TValue));
    }

    static bool WriteMemory(uintptr_t address, const void* data, std::size_t numBytes, bool flushInstructionCache = true);

    template <typename TValue>
    static bool WriteValue(uintptr_t address, const TValue& value, bool flushInstructionCache = true) {
        return SafeWriteMemory(address, &value, sizeof(TValue), flushInstructionCache);
    }

    // AOB scanning
    static std::vector<uintptr_t> AobScan(const char* pattern, size_t maxResults = 1,
        uintptr_t rangeStart = 0, uintptr_t rangeEnd = 0, bool executableOnly = false);
    static uintptr_t AobScanFirst(const char* pattern, uintptr_t rangeStart = 0,
        uintptr_t rangeEnd = 0, bool executableOnly = false);
    static std::vector<uintptr_t> AobScanModule(const char* moduleName, const char* pattern,
        size_t maxResults = 1, bool executableOnly = false);
    static uintptr_t AobScanModuleFirst(const char* moduleName, const char* pattern,
        bool executableOnly = false);
    static uintptr_t ScanModulePatternRobust(const char* moduleName, const char* pattern);

    // Hook management
    static bool InstallHook(const char* moduleName, uint32_t offset,
        const void* trampolineCode, size_t trampolineCodeSize, uint32_t& outHookId,
        bool executeUserCodeFirst = false, bool allowAbsEntryFallback = true,
        bool appendRelocatedOriginalCode = true);
    static bool UninstallHook(uint32_t hookId);
    static bool UninstallAll();
};

} // namespace InlineHook
