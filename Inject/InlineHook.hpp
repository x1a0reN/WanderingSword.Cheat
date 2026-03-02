#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <iostream>

/// Inline Hook 封装库
/// 支持：模块地址获取、跳板内存分配、动态启用/禁用 Hook

namespace InlineHook {

/// Hook 配置
struct HookConfig {
    const char* ModuleName;      // 模块名，如 "JH-Win64-Shipping.exe"
    uint32_t Offset;             // 偏移地址
    const void* HookFunc;        // Hook 函数地址
    size_t JmpBackOffset;       // 跳回原函数的偏移（相对于被 Hook 地址+5）
};

/// Inline Hook 实例
classInlineHook {
private:
    uintptr_t m_TargetAddr = 0;      // 目标地址
    uintptr_t m_Trampoline = 0;       // 跳板地址
    uint8_t m_OriginalBytes[16] = {}; // 原始字节
    bool m_Installed = false;         // 是否已安装
    size_t m_PatchSize = 5;           // Hook 补丁大小

    /// 分配跳板内存（在目标模块附近）
    void* AllocateTrampolineNearTarget(HANDLE hProcess, uintptr_t TargetAddr) {
        // 尝试在目标地址附近分配 (目标地址 + 0x10000000)
        uintptr_t PreferredAddr = (TargetAddr & 0xFFFFFFFF00000000ULL) | 0x10000000;
        void* Trampoline = VirtualAllocEx(hProcess, reinterpret_cast<void*>(PreferredAddr), 4096,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        // 如果失败，使用任意地址
        if (!Trampoline) {
            Trampoline = VirtualAllocEx(hProcess, nullptr, 4096,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        }

        return Trampoline;
    }

public:
    InlineHook() = default;
    ~InlineHook() { Remove(); }

    // 禁止拷贝
    InlineHook(const InlineHook&) = delete;
    InlineHook& operator=(const InlineHook&) = delete;

    /// 初始化 Hook
    /// @param ModuleName 模块名
    /// @param Offset 偏移地址
    /// @return 是否成功
    bool Init(const char* ModuleName, uint32_t Offset) {
        HMODULE hModule = GetModuleHandleA(ModuleName);
        if (!hModule) {
            std::cerr << "[InlineHook] Module not found: " << ModuleName << "\n";
            return false;
        }

        m_TargetAddr = reinterpret_cast<uintptr_t>(hModule) + Offset;
        std::cout << "[InlineHook] Target address: 0x" << std::hex << m_TargetAddr << std::dec << "\n";

        // 保存原始字节
        std::memcpy(m_OriginalBytes, reinterpret_cast<void*>(m_TargetAddr), 16);

        // 分配跳板内存
        HANDLE hProcess = GetCurrentProcess();
        m_Trampoline = reinterpret_cast<uintptr_t>(AllocateTrampolineNearTarget(hProcess, m_TargetAddr));
        if (!m_Trampoline) {
            std::cerr << "[InlineHook] Failed to allocate trampoline\n";
            return false;
        }

        std::cout << "[InlineHook] Trampoline address: 0x" << std::hex << m_Trampoline << std::dec << "\n";
        return true;
    }

    /// 设置跳板代码（用户自定义）
    /// @param TrampolineCode 跳板代码
    /// @param Size 代码大小
    void SetTrampolineCode(const void* TrampolineCode, size_t Size) {
        if (m_Trampoline && TrampolineCode && Size > 0) {
            std::memcpy(reinterpret_cast<void*>(m_Trampoline), TrampolineCode, Size);
            std::cout << "[InlineHook] Trampoline code set, size: " << Size << "\n";
        }
    }

    /// 安装 Hook
    /// @param PatchSize 补丁大小（默认5字节）
    void Install(size_t PatchSize = 5) {
        if (m_Installed || !m_TargetAddr || !m_Trampoline)
            return;

        m_PatchSize = PatchSize;

        DWORD OldProtect;
        if (VirtualProtect(reinterpret_cast<void*>(m_TargetAddr), m_PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {

            // 写入 jmp 指令跳转到跳板
            uint8_t JmpCode[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
            int32_t Offset = static_cast<int32_t>(m_Trampoline - m_TargetAddr - 5);
            std::memcpy(&JmpCode[1], &Offset, 4);
            std::memcpy(reinterpret_cast<void*>(m_TargetAddr), JmpCode, 5);

            VirtualProtect(reinterpret_cast<void*>(m_TargetAddr), m_PatchSize, OldProtect, &OldProtect);
            m_Installed = true;
            std::cout << "[InlineHook] Installed at 0x" << std::hex << m_TargetAddr << std::dec << "\n";
        }
    }

    /// 卸载 Hook
    void Remove() {
        if (!m_Installed || !m_TargetAddr)
            return;

        DWORD OldProtect;
        if (VirtualProtect(reinterpret_cast<void*>(m_TargetAddr), m_PatchSize,
            PAGE_EXECUTE_READWRITE, &OldProtect)) {

            // 恢复原始字节
            std::memcpy(reinterpret_cast<void*>(m_TargetAddr), m_OriginalBytes, m_PatchSize);

            VirtualProtect(reinterpret_cast<void*>(m_TargetAddr), m_PatchSize, OldProtect, &OldProtect);
            m_Installed = false;
            std::cout << "[InlineHook] Removed from 0x" << std::hex << m_TargetAddr << std::dec << "\n";
        }
    }

    /// 释放跳板内存
    void FreeTrampoline() {
        if (m_Trampoline) {
            VirtualFree(reinterpret_cast<void*>(m_Trampoline), 0, MEM_RELEASE);
            m_Trampoline = 0;
        }
    }

    /// 获取目标地址
    uintptr_t GetTargetAddr() const { return m_TargetAddr; }

    /// 获取跳板地址
    uintptr_t GetTrampolineAddr() const { return m_Trampoline; }

    /// 是否已安装
    bool IsInstalled() const { return m_Installed; }

    /// 计算跳转到指定地址的偏移（用于跳板中）
    static int32_t CalcJmpOffset(uintptr_t From, uintptr_t To) {
        return static_cast<int32_t>(To - (From + 5));
    }

    /// 计算从跳板跳回原函数的偏移
    static int32_t CalcJmpBackOffset(uintptr_t TrampolineAddr, size_t TrampolineSize, uintptr_t TargetAddr) {
        return static_cast<int32_t>((TargetAddr + 5) - (TrampolineAddr + TrampolineSize + 4));
    }
};

} // namespace InlineHook
