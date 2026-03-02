#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <memory>

/// Inline Hook 封装库
/// 简化使用：单行创建、自动管理 jmp 指令

namespace InlineHook {

/// Hook 节点
struct HookEntry {
    uintptr_t TargetAddr = 0;      // 目标地址
    uintptr_t Trampoline = 0;       // 跳板地址
    uint8_t OriginalBytes[16] = {}; // 原始字节
    size_t PatchSize = 5;          // 补丁大小
    bool Installed = false;         // 是否已安装
};

/// 全局 Hook 管理器
class HookManager {
private:
    static std::vector<HookEntry> s_Hooks;
    static uint32_t s_NextHookId;

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

public:
    /// 创建并安装 Hook
    /// @param ModuleName 模块名
    /// @param Offset 偏移地址
    /// @param TrampolineCode 自定义跳板代码
    /// @param TrampolineCodeSize 跳板代码大小（不包含最后的 jmp back）
    /// @return Hook ID（0 表示失败）
    static uint32_t InstallHook(const char* ModuleName, uint32_t Offset,
        const void* TrampolineCode, size_t TrampolineCodeSize) {

        HMODULE hModule = GetModuleHandleA(ModuleName);
        if (!hModule) {
            std::cerr << "[InlineHook] Module not found: " << ModuleName << "\n";
            return 0;
        }

        uintptr_t TargetAddr = reinterpret_cast<uintptr_t>(hModule) + Offset;
        std::cout << "[InlineHook] Target: 0x" << std::hex << TargetAddr << std::dec << "\n";

        // 保存原始字节
        HookEntry Entry;
        Entry.TargetAddr = TargetAddr;
        std::memcpy(Entry.OriginalBytes, reinterpret_cast<void*>(TargetAddr), 16);

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

        // 用户自定义代码
        if (TrampolineCode && TrampolineCodeSize > 0) {
            std::memcpy(Trampoline, TrampolineCode, TrampolineCodeSize);
            Offset += TrampolineCodeSize;
        }

        // jmp back to original +5
        Trampoline[Offset++] = 0xE9;
        int32_t JmpBackOffset = static_cast<int32_t>((TargetAddr + 5) - (Entry.Trampoline + Offset + 4));
        std::memcpy(&Trampoline[Offset], &JmpBackOffset, 4);
        Offset += 4;

        // 安装 jmp 到跳板
        Entry.PatchSize = 5;
        DWORD OldProtect;
        if (VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {

            uint8_t JmpCode[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
            int32_t JmpToTrampoline = static_cast<int32_t>(Entry.Trampoline - TargetAddr - 5);
            std::memcpy(&JmpCode[1], &JmpToTrampoline, 4);
            std::memcpy(reinterpret_cast<void*>(TargetAddr), JmpCode, 5);

            VirtualProtect(reinterpret_cast<void*>(TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);
            Entry.Installed = true;
            std::cout << "[InlineHook] Installed hook ID: " << s_NextHookId << "\n";
        }

        s_Hooks.push_back(Entry);
        return s_NextHookId++;
    }

    /// 卸载指定 Hook
    /// @param HookId Hook ID
    /// @return 是否成功
    static bool UninstallHook(uint32_t HookId) {
        if (HookId >= s_Hooks.size()) {
            std::cerr << "[InlineHook] Invalid hook ID: " << HookId << "\n";
            return false;
        }

        HookEntry& Entry = s_Hooks[HookId];
        if (!Entry.Installed) {
            std::cout << "[InlineHook] Hook not installed: " << HookId << "\n";
            return true;
        }

        DWORD OldProtect;
        if (VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {

            // 恢复原始字节
            std::memcpy(reinterpret_cast<void*>(Entry.TargetAddr), Entry.OriginalBytes, Entry.PatchSize);

            VirtualProtect(reinterpret_cast<void*>(Entry.TargetAddr), Entry.PatchSize, OldProtect, &OldProtect);
            Entry.Installed = false;
            std::cout << "[InlineHook] Uninstalled hook ID: " << HookId << "\n";
        }

        // 释放跳板内存
        if (Entry.Trampoline) {
            VirtualFree(reinterpret_cast<void*>(Entry.Trampoline), 0, MEM_RELEASE);
            Entry.Trampoline = 0;
        }

        return true;
    }

    /// 卸载所有 Hook
    static void UninstallAll() {
        std::cout << "[InlineHook] Uninstalling all hooks...\n";
        for (uint32_t i = 0; i < s_Hooks.size(); i++) {
            if (s_Hooks[i].Installed) {
                UninstallHook(i);
            }
        }
        s_Hooks.clear();
        s_NextHookId = 0;
    }
};

// 静态成员初始化
inline std::vector<HookEntry> HookManager::s_Hooks;
inline uint32_t HookManager::s_NextHookId = 0;

} // namespace InlineHook
