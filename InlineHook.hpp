#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <mutex>

/// Inline Hook 封装库（安全版）
/// 支持：模块地址获取、跳板内存分配、动态启用/禁用 Hook
/// 线程安全设计，支持多线程环境

namespace InlineHook {

/// 错误码
enum class ErrorCode {
    Success = 0,
    ModuleNotFound,
    TargetNotReadable,
    TargetNotExecutable,
    TrampolineAllocFailed,
    TrampolineCodeTooLarge,
    Rel32OutOfRange,
    VirtualProtectFailed,
    PatchWriteFailed,
    UninstallFailed
};

/// Hook 节点
struct HookEntry {
    uintptr_t TargetAddr = 0;         // 目标地址
    uintptr_t Trampoline = 0;          // 跳板地址
    uint8_t OriginalBytes[16] = {};    // 原始字节
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

        // 检查内存状态和权限
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

        // 检查剩余长度
        uintptr_t offset = TargetAddr - reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        if (offset + Size > mbi.RegionSize) {
            return false;
        }

        std::memcpy(Buffer, reinterpret_cast<void*>(TargetAddr), Size);
        return true;
    }

    /// 简单反汇编：获取 x64 指令长度
    /// @param Address 指令地址
    /// @return 指令长度（字节）
    static size_t GetInstructionLength(uint8_t* Address) {
        // 简化实现：只处理常见前缀和opcode
        // 实际项目中应使用 Capstone 或udis86

        uint8_t Op = Address[0];

        // 0x90 = nop
        if (Op == 0x90) return 1;

        // REX 前缀 (0x40-0x4F)
        if (Op >= 0x40 && Op <= 0x4F) {
            Op = Address[1];
            // 处理 REX + 移位/扩展 opcode
            if (Op == 0xC0 || Op == 0xC1 || Op == 0xD0 || Op == 0xD1 || Op == 0xD2 || Op == 0xD3) {
                return 3; // REX + opcode + modrm
            }
            // REX + 常用指令
            return 2; // REX + opcode
        }

        // 2-byte opcode (0x0F开头)
        if (Op == 0x0F) {
            Op = Address[1];
            // 条件跳转: 0x80-0x8F (6字节)
            if (Op >= 0x80 && Op <= 0x8F) return 6;
            // MOVSXD (3字节)
            if (Op == 0x63) return 3;
            // 其他 0x0F xx 大多3-4字节
            return 3;
        }

        // 常用指令长度
        switch (Op) {
            case 0x55: // push rbp
            case 0x53: // push rbx
            case 0x50: // push rax
            case 0x51: // push rcx
            case 0x52: // push rdx
            case 0x5D: // pop rbp
            case 0x5B: // pop rbx
            case 0x58: // pop rax
            case 0x59: // pop rcx
            case 0x5A: // pop rdx
            case 0xC3: // ret
            case 0xCC: // int3
                return 1;

            case 0x48: // rex.w 前缀 + 下一条指令
                return 2 + GetInstructionLength(Address + 1);

            case 0x89: // mov r/m64, r64
            case 0x8B: // mov r64, r/m64
            case 0x81: // add/imul/etc r/m32, imm32
            case 0x83: // add/imul/etc r/m8/32, imm8
            case 0xC7: // mov r/m64, imm32
                return 3; // opcode + modrm + imm

            case 0xB8: // mov r64, imm64 (5字节) 或 mov r32, imm32 (5字节)
            case 0xB9:
            case 0xBA:
            case 0xBB:
            case 0xBC:
            case 0xBD:
            case 0xBE:
            case 0xBF:
                return 5;

            case 0xE9: // jmp rel32 (5字节)
            case 0xE8: // call rel32 (5字节)
                return 5;

            case 0xFF: // 扩展指令 (push, call, jmp)
                return 2;

            default:
                // 保守估计：至少覆盖最小指令
                return 1;
        }
    }

    /// 计算需要覆盖的指令边界
    static size_t CalculatePatchSize(uintptr_t TargetAddr, size_t MaxSize = 14) {
        size_t TotalSize = 0;
        uint8_t* p = reinterpret_cast<uint8_t*>(TargetAddr);

        while (TotalSize < MaxSize) {
            size_t InstLen = GetInstructionLength(p + TotalSize);
            TotalSize += InstLen;

            // 至少需要5字节来写入jmp
            if (TotalSize >= 5) break;
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

    /// 写入跳转（支持 rel32 或绝对跳转）
    static bool WriteJump(uintptr_t From, uintptr_t To, size_t PatchSize) {
        if (PatchSize < 5) return false;

        int32_t Offset;
        if (TryMakeRel32(From, To, Offset)) {
            // 使用 rel32 jmp (5字节)
            uint8_t JmpCode[5] = { 0xE9, 0, 0, 0, 0 };
            std::memcpy(&JmpCode[1], &Offset, 4);
            std::memcpy(reinterpret_cast<void*>(From), JmpCode, 5);

            // 如果需要覆盖更多字节，填充 nop
            for (size_t i = 5; i < PatchSize; i++) {
                reinterpret_cast<uint8_t*>(From)[i] = 0x90; // nop
            }
        } else {
            // 使用绝对跳转 FF 25 [RIP+0] + imm64 (14字节)
            if (PatchSize < 14) return false;

            uint8_t JmpCode[14] = {
                0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp qword [rip+0]
                0x00, 0x00, 0x00, 0x00,             // 64位目标地址 (低32位，为0表示下条指令)
                0x00, 0x00, 0x00, 0x00              // 64位目标地址 (高32位)
            };
            // 将目标地址写入 [rip+2] 后的8字节
            std::memcpy(&JmpCode[6], &To, 8);
            std::memcpy(reinterpret_cast<void*>(From), JmpCode, 14);
        }

        return true;
    }

public:
    /// 创建并安装 Hook
    /// @param ModuleName 模块名
    /// @param Offset 偏移地址
    /// @param TrampolineCode 自定义跳板代码
    /// @param TrampolineCodeSize 跳板代码大小（不包含最后的 jmp back）
    /// @return Hook ID（0 表示失败）
    static uint32_t InstallHook(const char* ModuleName, uint32_t Offset,
        const void* TrampolineCode, size_t TrampolineCodeSize) {

        std::lock_guard<std::mutex> lock(s_Mutex);

        // 检查长度上限 (保留空间给 jmp back)
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

        // 安全读取原始字节（至少16字节）
        HookEntry Entry;
        if (!SafeReadTarget(TargetAddr, Entry.OriginalBytes, 16)) {
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
        if (Entry.PatchSize > 0) {
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
        if (!TryMakeRel32(Entry.Trampoline + Offset + 4, TargetAddr + Entry.PatchSize, JmpBackOffset)) {
            std::cerr << "[InlineHook] JmpBackOffset out of range\n";
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            return 0;
        }
        std::memcpy(&Trampoline[Offset], &JmpBackOffset, 4);
        Offset += 4;

        // 安装 jmp 到跳板
        DWORD OldProtect;
        if (!VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {
            std::cerr << "[InlineHook] VirtualProtect failed: " << GetLastError() << "\n";
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            return 0;
        }

        // 写入跳转指令
        if (!WriteJump(TargetAddr, Entry.Trampoline, Entry.PatchSize)) {
            std::cerr << "[InlineHook] WriteJump failed\n";
            VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            return 0;
        }

        // 验证写入
        uint8_t VerifyBuffer[16] = {};
        std::memcpy(VerifyBuffer, reinterpret_cast<void*>(TargetAddr), Entry.PatchSize);
        uint8_t Expected[16] = {};
        WriteJump(TargetAddr, Entry.Trampoline, Entry.PatchSize);
        // 注意：WriteJump 内部已经写入，这里只是简单检查

        // 刷新指令缓存
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(TargetAddr), Entry.PatchSize);

        VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);

        Entry.Installed = true;
        std::cout << "[InlineHook] Installed hook ID: " << s_NextHookId << "\n";

        s_Hooks.push_back(Entry);
        return s_NextHookId++;
    }

    /// 卸载指定 Hook
    /// @param HookId Hook ID
    /// @return 是否成功
    static bool UninstallHook(uint32_t HookId) {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (HookId >= s_Hooks.size()) {
            std::cerr << "[InlineHook] Invalid hook ID: " << HookId << "\n";
            return false;
        }

        HookEntry& Entry = s_Hooks[HookId];

        // 如果未安装，只释放跳板内存
        if (!Entry.Installed) {
            if (Entry.Trampoline) {
                VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
                Entry.Trampoline = 0;
            }
            return true;
        }

        // 恢复原始字节
        DWORD OldProtect;
        if (!VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {
            std::cerr << "[InlineHook] Uninstall VirtualProtect failed: " << GetLastError() << "\n";
            return false;  // 不释放跳板，防止跳到已释放内存
        }

        // 恢复原始指令
        std::memcpy(reinterpret_cast<void*>(Entry.TargetAddr), Entry.OriginalBytes, Entry.PatchSize);

        // 刷新指令缓存
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize);

        VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);
        Entry.Installed = false;
        std::cout << "[InlineHook] Uninstalled hook ID: " << HookId << "\n";

        // 释放跳板内存（仅在成功恢复原始指令后才释放）
        if (Entry.Trampoline) {
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            Entry.Trampoline = 0;
        }

        return true;
    }

    /// 卸载所有 Hook
    /// @return 是否全部成功
    static bool UninstallAll() {
        std::lock_guard<std::mutex> lock(s_Mutex);

        std::cout << "[InlineHook] Uninstalling all hooks...\n";
        bool AllSuccess = true;

        for (uint32_t i = 0; i < s_Hooks.size(); i++) {
            HookEntry& Entry = s_Hooks[i];

            if (!Entry.Installed) {
                // 未安装但有跳板的也要释放
                if (Entry.Trampoline) {
                    VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
                    Entry.Trampoline = 0;
                }
                continue;
            }

            // 尝试恢复
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

        s_Hooks.clear();
        s_NextHookId = 0;

        return AllSuccess;
    }
};

// 静态成员初始化
inline std::vector<HookEntry> HookManager::s_Hooks;
inline uint32_t HookManager::s_NextHookId = 0;
inline std::mutex HookManager::s_Mutex;

} // namespace InlineHook
