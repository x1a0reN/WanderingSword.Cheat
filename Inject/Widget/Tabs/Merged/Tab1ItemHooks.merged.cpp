#include <cstdint>
#include <iostream>

#include "CheatState.hpp"
#include "InlineHook.hpp"

namespace
{
    uint32_t GTab1ItemNoDecreaseHookId = UINT32_MAX;

    // 功能：如果 Num < 0，则设为 0（防止负数扣除）
    const unsigned char kItemNoDecreaseTrampolineCode[] = {
        0x41, 0x83, 0xF8, 0x00,               // cmp r8d, 0
        0x0F, 0x8D, 0x03, 0x00, 0x00, 0x00,   // jge +3
        0x45, 0x31, 0xC0                      // xor r8d, r8d
    };
}

void EnableItemNoDecreaseHook()
{
    if (GTab1ItemNoDecreaseHookId != UINT32_MAX)
        return;

    uint32_t hookId = UINT32_MAX;
    const bool success = InlineHook::HookManager::InstallHook(
        "JH-Win64-Shipping.exe",
        0x1206A70,
        kItemNoDecreaseTrampolineCode,
        sizeof(kItemNoDecreaseTrampolineCode),
        hookId
    );

    if (success && hookId != UINT32_MAX)
    {
        GTab1ItemNoDecreaseHookId = hookId;
        std::cout << "[SDK] ItemNoDecrease hook enabled, ID: " << hookId << "\n";
    }
    else
    {
        std::cout << "[SDK] ItemNoDecrease hook failed\n";
    }
}

void DisableItemNoDecreaseHook()
{
    if (GTab1ItemNoDecreaseHookId == UINT32_MAX)
        return;

    const bool success = InlineHook::HookManager::UninstallHook(GTab1ItemNoDecreaseHookId);
    if (success)
    {
        std::cout << "[SDK] ItemNoDecrease hook disabled\n";
    }
    else
    {
        std::cout << "[SDK] ItemNoDecrease hook disable failed\n";
    }

    GTab1ItemNoDecreaseHookId = UINT32_MAX;
}
