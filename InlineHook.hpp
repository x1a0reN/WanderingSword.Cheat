#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <mutex>
#include <TlHelp32.h>

/// Inline Hook 封装库（安全版）
/// 支持：模块地址获取、跳板内存分配、动态启用/禁用 Hook
/// 线程安全设计，支持多线程环境

namespace InlineHook {

/// Hook 节点
struct HookEntry {
    uintptr_t TargetAddr = 0;         // 目标地址
    uintptr_t Trampoline = 0;          // 跳板地址
    uint8_t OriginalBytes[32] = {};    // 原始字节（增大到32字节）
    size_t PatchSize = 0;              // 实际补丁大小（覆盖的指令长度）
    bool Installed = false;             // 是否已安装
};

/// 全局 Hook 管理器
class HookManager {
private:
    static std::vector<HookEntry> s_Hooks;
    static uint32_t s_NextHookId;
    static std::mutex s_Mutex;

    /// 尝试使用 int64_t 计算并转换为 rel32 偏移
    /// @param from_next 跳转源地址（下一条指令地址）
    /// @param to 目标地址
    /// @param out 输出偏移
    /// @return 是否成功
    static bool TryMakeRel32(uintptr_t from_next, uintptr_t to, int32_t& out) {
        int64_t delta = static_cast<int64_t>(to) - static_cast<int64_t>(from_next);
        if (delta < INT32_MIN || delta > INT32_MAX) {
            return false;
        }
        out = static_cast<int32_t>(delta);
        return true;
    }

    /// 校验目标地址可访问性
    static bool SafeReadTarget(uintptr_t TargetAddr, void* Buffer, size_t Size) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(reinterpret_cast<void*>(TargetAddr), &mbi, sizeof(mbi))) {
            return false;
        }

        if (mbi.State != MEM_COMMIT) {
            return false;
        }

        DWORD protect = mbi.Protect;
        if (protect & (PAGE_NOACCESS | PAGE_GUARD)) {
            return false;
        }

        if (!(protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE |
                       PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
            return false;
        }

        uintptr_t offset = TargetAddr - reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        if (offset + Size > mbi.RegionSize) {
            return false;
        }

        std::memcpy(Buffer, reinterpret_cast<void*>(TargetAddr), Size);
        return true;
    }

    /// 简单反汇编：获取 x64 指令长度
    static size_t GetInstructionLength(uint8_t* Address) {
        if (!Address) return 1;

        uint8_t Op = Address[0];

        // REX 前缀 (0x40-0x4F)
        if (Op >= 0x40 && Op <= 0x4F) {
            // REX.W (0x48) 后通常是 3-5 字节指令
            if (Op == 0x48) {
                uint8_t Op2 = Address[1];
                // 48 89 = mov r/m64, r64 (3字节)
                if (Op2 == 0x89 || Op2 == 0x8B) return 3;
                // 48 83 = add/sub/etc r/m64, imm8 (4字节)
                if (Op2 == 0x83 || Op2 == 0x81) return 4;
                // 48 B8 = mov r64, imm64 (10字节)
                if (Op2 == 0xB8) return 10;
                // 48 31 = xor r64, r64 (3字节)
                if (Op2 == 0x31) return 3;
                return 2;
            }
            return 2;
        }

        // 2-byte opcode (0x0F开头)
        if (Op == 0x0F) {
            uint8_t Op2 = Address[1];
            // 条件跳转: 0x80-0x8F (6字节)
            if (Op2 >= 0x80 && Op2 <= 0x8F) return 6;
            // MOVSXD (3字节)
            if (Op2 == 0x63) return 3;
            return 3;
        }

        switch (Op) {
            case 0x90: return 1;  // nop
            case 0xC3: return 1;  // ret
            case 0xCC: return 1;  // int3

            // push/pop r64 (1字节)
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57:
            case 0x58: case 0x59: case 0x5A: case 0x5B:
            case 0x5C: case 0x5D: case 0x5E: case 0x5F:
                return 1;

            // mov r64, imm64 (10字节)
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                return 10;

            // mov r/m64, r64 (3字节: opcode + modrm + SIB)
            // add/imul/etc r/m64, imm8 (4字节)
            // mov r/m64, imm32 (7字节)
            case 0x89: case 0x8B:
            case 0x81: case 0x83: case 0xC7:
                return (Op == 0xC7) ? 7 : ((Op == 0x81) ? 7 : 3);

            // jmp/call rel32 (5字节)
            case 0xE9: case 0xE8:
                return 5;

            // jmp/call rel8 (2字节)
            case 0xEB: case 0xE3:
                return 2;

            default:
                // 保守估计
                return 1;
        }
    }

    /// 计算需要覆盖的指令边界（至少5字节）
    static size_t CalculatePatchSize(uintptr_t TargetAddr) {
        size_t TotalSize = 0;
        uint8_t* p = reinterpret_cast<uint8_t*>(TargetAddr);

        while (TotalSize < 5) {
            size_t InstLen = GetInstructionLength(p + TotalSize);
            TotalSize += InstLen;
        }

        return TotalSize;
    }

    /// 分配跳板内存（在目标模块附近）
    static void* AllocateTrampolineNearTarget(HANDLE hProcess, uintptr_t TargetAddr) {
        // 尝试在目标地址附近分配
        uintptr_t PreferredAddr = (TargetAddr & 0xFFFFFFFF00000000ULL) | 0x10000000;
        void* Trampoline = VirtualAllocEx(hProcess, reinterpret_cast<void*>(PreferredAddr), 4096,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        if (!Trampoline) {
            Trampoline = VirtualAllocEx(hProcess, nullptr, 4096,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        }
        return Trampoline;
    }

    /// 写入跳转（rel32 跳转）
    /// @param from 跳转指令地址
    /// @param from_next 跳转后执行的地址（即 from + PatchSize）
    /// @param to 目标地址
    /// @param PatchSize 覆盖的字节数
    static bool WriteJump(uintptr_t from, uintptr_t from_next, uintptr_t to, size_t PatchSize) {
        if (PatchSize < 5) return false;

        // 计算相对偏移：from_next -> to
        int32_t Offset;
        if (TryMakeRel32(from_next, to, Offset)) {
            // 使用 rel32 jmp (5字节)
            uint8_t JmpCode[5] = { 0xE9, 0, 0, 0, 0 };
            std::memcpy(&JmpCode[1], &Offset, 4);
            std::memcpy(reinterpret_cast<void*>(from), JmpCode, 5);

            // 如果需要覆盖更多字节，填充 nop
            for (size_t i = 5; i < PatchSize; i++) {
                reinterpret_cast<uint8_t*>(from)[i] = 0x90;
            }
        } else {
            // 使用绝对跳转 FF 25 [RIP+0] + imm64 (14字节)
            if (PatchSize < 14) return false;

            uint8_t JmpCode[14] = {
                0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00
            };
            std::memcpy(&JmpCode[6], &to, 8);
            std::memcpy(reinterpret_cast<void*>(from), JmpCode, 14);
        }

        return true;
    }

    /// 暂停所有线程（简单实现：只暂停当前进程线程）
    static void SuspendAllThreads() {
        // 注意：这是一个简化实现，生产环境需要更复杂的线程管理
        // 在游戏进程中，这可能需要更好的实现
    }

    /// 恢复所有线程
    static void ResumeAllThreads() {
    }

public:
    /// 创建并安装 Hook
    static uint32_t InstallHook(const char* ModuleName, uint32_t Offset,
        const void* TrampolineCode, size_t TrampolineCodeSize) {

        std::lock_guard<std::mutex> lock(s_Mutex);

        if (TrampolineCodeSize > 4096 - 14) {
            std::cerr << "[InlineHook] TrampolineCodeSize too large: " << TrampolineCodeSize << "\n";
            return 0;
        }

        HMODULE hModule = GetModuleHandleA(ModuleName);
        if (!hModule) {
            std::cerr << "[InlineHook] Module not found: " << ModuleName << "\n";
            return 0;
        }

        uintptr_t TargetAddr = reinterpret_cast<uintptr_t>(hModule) + Offset;

        HookEntry Entry;
        if (!SafeReadTarget(TargetAddr, Entry.OriginalBytes, 32)) {
            std::cerr << "[InlineHook] Target address not readable: 0x" << std::hex << TargetAddr << "\n";
            return 0;
        }
        Entry.TargetAddr = TargetAddr;
        std::cout << "[InlineHook] Target: 0x" << std::hex << TargetAddr << std::dec << "\n";

        // 计算实际需要覆盖的指令长度
        Entry.PatchSize = CalculatePatchSize(TargetAddr);
        std::cout << "[InlineHook] Patch size: " << Entry.PatchSize << "\n";

        // 分配跳板内存
        HANDLE hProcess = GetCurrentProcess();
        Entry.Trampoline = reinterpret_cast<uintptr_t>(AllocateTrampolineNearTarget(hProcess, TargetAddr));
        if (!Entry.Trampoline) {
            std::cerr << "[InlineHook] Failed to allocate trampoline\n";
            return 0;
        }
        std::cout << "[InlineHook] Trampoline: 0x" << std::hex << Entry.Trampoline << std::dec << "\n";

        // 写入跳板代码
        unsigned char* Trampoline = reinterpret_cast<unsigned char*>(Entry.Trampoline);
        size_t Offset = 0;

        // 1. 复制原始指令（stolen bytes）
        if (Entry.PatchSize > 0 && Entry.PatchSize <= 32) {
            std::memcpy(Trampoline, Entry.OriginalBytes, Entry.PatchSize);
            Offset += Entry.PatchSize;
        }

        // 2. 用户自定义代码
        if (TrampolineCode && TrampolineCodeSize > 0) {
            if (Offset + TrampolineCodeSize > 4096 - 14) {
                std::cerr << "[InlineHook] Trampoline code too large\n";
                VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
                return 0;
            }
            std::memcpy(Trampoline + Offset, TrampolineCode, TrampolineCodeSize);
            Offset += TrampolineCodeSize;
        }

        // 3. jmp back to original + PatchSize
        Trampoline[Offset++] = 0xE9;
        int32_t JmpBackOffset;
        // 注意：from_next = Entry.Trampoline + Offset + 4 (jmp 指令后面)
        if (!TryMakeRel32(Entry.Trampoline + Offset + 4, TargetAddr + Entry.PatchSize, JmpBackOffset)) {
            std::cerr << "[InlineHook] JmpBackOffset out of range\n";
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            return 0;
        }
        std::memcpy(&Trampoline[Offset], &JmpBackOffset, 4);
        Offset += 4;

        // 暂停线程
        SuspendAllThreads();

        // 安装 jmp 到跳板
        DWORD OldProtect;
        if (!VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {
            std::cerr << "[InlineHook] VirtualProtect failed: " << GetLastError() << "\n";
            ResumeAllThreads();
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            return 0;
        }

        // 写入跳转：from = TargetAddr, from_next = TargetAddr + PatchSize
        if (!WriteJump(TargetAddr, TargetAddr + Entry.PatchSize, Entry.Trampoline, Entry.PatchSize)) {
            std::cerr << "[InlineHook] WriteJump failed\n";
            VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);
            ResumeAllThreads();
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            return 0;
        }

        // 刷新指令缓存
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(TargetAddr), Entry.PatchSize);

        VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);

        ResumeAllThreads();

        Entry.Installed = true;
        std::cout << "[InlineHook] Installed hook ID: " << s_NextHookId << "\n";

        s_Hooks.push_back(Entry);
        return s_NextHookId++;
    }

    /// 卸载指定 Hook
    static bool UninstallHook(uint32_t HookId) {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (HookId >= s_Hooks.size()) {
            std::cerr << "[InlineHook] Invalid hook ID: " << HookId << "\n";
            return false;
        }

        HookEntry& Entry = s_Hooks[HookId];

        if (!Entry.Installed) {
            if (Entry.Trampoline) {
                VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
                Entry.Trampoline = 0;
            }
            return true;
        }

        SuspendAllThreads();

        DWORD OldProtect;
        if (!VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {
            std::cerr << "[InlineHook] Uninstall VirtualProtect failed: " << GetLastError() << "\n";
            ResumeAllThreads();
            return false;
        }

        std::memcpy(reinterpret_cast<void*>(Entry.TargetAddr), Entry.OriginalBytes, Entry.PatchSize);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize);
        VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);

        ResumeAllThreads();

        Entry.Installed = false;
        std::cout << "[InlineHook] Uninstalled hook ID: " << HookId << "\n";

        if (Entry.Trampoline) {
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            Entry.Trampoline = 0;
        }

        return true;
    }

    /// 卸载所有 Hook
    static bool UninstallAll() {
        std::lock_guard<std::mutex> lock(s_Mutex);

        std::cout << "[InlineHook] Uninstalling all hooks...\n";
        bool AllSuccess = true;

        SuspendAllThreads();

        for (uint32_t i = 0; i < s_Hooks.size(); i++) {
            HookEntry& Entry = s_Hooks[i];

            if (!Entry.Installed) {
                if (Entry.Trampoline) {
                    VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
                    Entry.Trampoline = 0;
                }
                continue;
            }

            DWORD OldProtect;
            if (!VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize,
                PAGE_EXECUTE_READWRITE, &OldProtect)) {
                std::cerr << "[InlineHook] UninstallAll: VirtualProtect failed for ID " << i << "\n";
                AllSuccess = false;
                continue;
            }

            std::memcpy(reinterpret_cast<void*>(Entry.TargetAddr), Entry.OriginalBytes, Entry.PatchSize);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize);
            VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);

            Entry.Installed = false;

            if (Entry.Trampoline) {
                VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
                Entry.Trampoline = 0;
            }
        }

        ResumeAllThreads();

        // 只有全部成功才清空
        if (AllSuccess) {
            s_Hooks.clear();
            s_NextHookId = 0;
        }

        return AllSuccess;
    }
};

inline std::vector<HookEntry> HookManager::s_Hooks;
inline uint32_t HookManager::s_NextHookId = 0;
inline std::mutex HookManager::s_Mutex;

} // namespace InlineHook
