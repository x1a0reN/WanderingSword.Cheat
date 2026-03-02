#include <Windows.h>
#include <iostream>
#include <cstdio>

#include "CheatState.hpp"
#include "GCManager.hpp"
#include "Logging.hpp"
#include "FrameHook.hpp"
#include "PanelManager.hpp"
#include "InlineHook.hpp"

// 物品不减 Hook ID
static uint32_t GItemNoDecreaseHookId = UINT32_MAX;

// 物品不减 Hook 跳板代码
// 功能：如果 Num < 0，则设为 0（防止负数扣除）
static const unsigned char kItemNoDecreaseTrampolineCode[] = {
    0x48, 0x89, 0x5C, 0x24, 0x08,  // mov [rsp+08], rbx  (保存寄存器)
    0x41, 0x83, 0xF8, 0x00,         // cmp r8d, 0       (比较 Num 参数)
    0x0F, 0x8D, 0x03, 0x00, 0x00,  // jge +3           (如果 >= 0 跳过 xor)
    0x45, 0x31, 0xC0                 // xor r8d, r8d    (如果 < 0 设为 0)
};

static void RemoveAllHooks()
{
	if (GHooksRemoved.exchange(true, std::memory_order_acq_rel))
		return;

	GVCPostRenderHook.Remove();
	OriginalGVCPostRender = nullptr;
	std::cout << "[SDK] GVC PostRender unhooked\n";

	// 使用 InlineHook 库卸载所有 Hook
	if (GItemNoDecreaseHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GItemNoDecreaseHookId);
		GItemNoDecreaseHookId = UINT32_MAX;
	}

	// 卸载所有 Inline Hook
	InlineHook::HookManager::UninstallAll();
}

static void CloseConsoleSafely()
{
	if (GConsoleClosed.exchange(true, std::memory_order_acq_rel))
		return;

	ShutdownLocalLogging();
	if (GetConsoleWindow())
		FreeConsole();
}

// 启用物品不减 Hook
void EnableItemNoDecreaseHook()
{
	if (GItemNoDecreaseHookId != UINT32_MAX)
		return; // 已经安装

	// 使用 InlineHook 库安装 Hook
	uint32_t hookId = UINT32_MAX;
	bool success = InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		0x1206A70,
		kItemNoDecreaseTrampolineCode,
		sizeof(kItemNoDecreaseTrampolineCode),
		hookId
	);

	if (success && hookId != UINT32_MAX)
	{
		GItemNoDecreaseHookId = hookId;
		std::cout << "[SDK] ItemNoDecrease hook enabled, ID: " << hookId << "\n";
	}
	else
	{
		std::cout << "[SDK] ItemNoDecrease hook failed\n";
	}
}

// 禁用物品不减 Hook
void DisableItemNoDecreaseHook()
{
	if (GItemNoDecreaseHookId == UINT32_MAX)
		return; // 未安装

	bool success = InlineHook::HookManager::UninstallHook(GItemNoDecreaseHookId);
	if (success)
	{
		std::cout << "[SDK] ItemNoDecrease hook disabled\n";
	}
	else
	{
		std::cout << "[SDK] ItemNoDecrease hook disable failed\n";
	}
	GItemNoDecreaseHookId = UINT32_MAX;
}

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy = nullptr;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);
	SetupLocalLogging(Module);
	SetUnhandledExceptionFilter(UnhandledExceptionLogger);

	std::cout << "[SDK] DLL Loaded. Press HOME to toggle internal widget, DELETE to unload.\n" << std::endl;

	if constexpr (Offsets::GVCPostRenderIdx >= 0)
	{
		UGameViewportClient* GVC = nullptr;
		UWorld* World = UWorld::GetWorld();
		if (World && World->PersistentLevel)
		{
			UWorld* OW = World->PersistentLevel->OwningWorld;
			if (OW && OW->OwningGameInstance && OW->OwningGameInstance->LocalPlayers.Num() > 0)
			{
				ULocalPlayer* LP = OW->OwningGameInstance->LocalPlayers[0];
				if (LP) GVC = LP->ViewportClient;
			}
		}

		if (GVC)
		{
			GVCPostRenderHook = VTableHook(GVC, Offsets::GVCPostRenderIdx);
			OriginalGVCPostRender = GVCPostRenderHook.Install<GVCPostRenderFn>(HookedGVCPostRender);

			if (OriginalGVCPostRender)
				std::cout << "[SDK] GVC PostRender hooked at index " << Offsets::GVCPostRenderIdx << "\n";
			else
				std::cout << "[SDK] Failed to hook GVC PostRender (VirtualProtect failed)\n";
		}
		else
			std::cout << "[SDK] ViewportClient is null, hook skipped\n";
	}
	else
	{
		std::cout << "[SDK] GVCPostRenderIdx not detected (-1), hook skipped\n";
	}

	constexpr DWORD kCleanupTimeoutMs = 2500;
	constexpr DWORD kInFlightTimeoutMs = 1500;

	while (true)
	{
		if ((GetAsyncKeyState(VK_DELETE) & 1) == 0)
		{
			Sleep(100);
			continue;
		}

		std::cout << "[SDK] Unloading...\n";
		GUnloadCleanupDone.store(false, std::memory_order_release);
		GIsUnloading.store(true, std::memory_order_release);

		// Phase 1: cleanup must be completed on game thread.
		bool CleanupDone = false;
		DWORD WaitStart = GetTickCount();
		while (!(CleanupDone = GUnloadCleanupDone.load(std::memory_order_acquire)))
		{
			if (GetTickCount() - WaitStart > kCleanupTimeoutMs)
				break;
			Sleep(10);
		}
		if (!CleanupDone)
		{
			GIsUnloading.store(false, std::memory_order_release);
			GUnloadCleanupDone.store(false, std::memory_order_release);
			std::cout << "[SDK] Unload refused (fail-closed): cleanup timeout, DLL kept loaded\n";
			continue;
		}

		// Phase 2: ensure no PostRender call is currently in-flight before unhook.
		bool PreUnhookDrained = false;
		WaitStart = GetTickCount();
		while (!(PreUnhookDrained = (GPostRenderInFlight.load(std::memory_order_acquire) == 0)))
		{
			if (GetTickCount() - WaitStart > kInFlightTimeoutMs)
				break;
			Sleep(1);
		}
		if (!PreUnhookDrained)
		{
			GIsUnloading.store(false, std::memory_order_release);
			std::cout << "[SDK] Unload refused (fail-closed): in-flight timeout before unhook, DLL kept loaded\n";
			continue;
		}

		RemoveAllHooks();

		// Phase 3: after unhook, there still must be zero in-flight calls.
		bool PostUnhookDrained = false;
		WaitStart = GetTickCount();
		while (!(PostUnhookDrained = (GPostRenderInFlight.load(std::memory_order_acquire) == 0)))
		{
			if (GetTickCount() - WaitStart > kInFlightTimeoutMs)
				break;
			Sleep(1);
		}
		if (!PostUnhookDrained)
		{
			GIsUnloading.store(false, std::memory_order_release);
			std::cout << "[SDK] Unload refused (fail-closed): in-flight timeout after unhook, DLL kept loaded\n";
			continue;
		}

		// Hard gate: both conditions must be satisfied, otherwise never unload.
		if (!GUnloadCleanupDone.load(std::memory_order_acquire) ||
			GPostRenderInFlight.load(std::memory_order_acquire) != 0)
		{
			GIsUnloading.store(false, std::memory_order_release);
			std::cout << "[SDK] Unload refused (fail-closed): final gate check failed, DLL kept loaded\n";
			continue;
		}

		SetUnhandledExceptionFilter(nullptr);
		CloseConsoleSafely();
		FreeLibraryAndExitThread(Module, 0);
		return 0;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		HANDLE ThreadHandle = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread), hModule, 0, nullptr);
		if (ThreadHandle)
			CloseHandle(ThreadHandle);
	}

	return TRUE;
}
