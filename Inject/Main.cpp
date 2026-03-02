#include <Windows.h>
#include <iostream>
#include <cstdio>

#include "CheatState.hpp"
#include "GCManager.hpp"
#include "Logging.hpp"
#include "FrameHook.hpp"
#include "PanelManager.hpp"

// 启用物品不减 Hook
static void EnableItemNoDecreaseHook()
{
	if (!GChangeItemNumAddr || !GHookTrampoline || GInlineHookInstalled)
		return;

	DWORD OldProtect;
	if (VirtualProtect(reinterpret_cast<void*>(GChangeItemNumAddr), 14, PAGE_EXECUTE_READWRITE, &OldProtect))
	{
		// 写入 jmp 指令跳转到跳板
		unsigned char JmpToTrampoline[] = {
			0xE9, 0x00, 0x00, 0x00, 0x00  // jmp rel32
		};
		int32_t TrampolineOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(GHookTrampoline) - GChangeItemNumAddr - 5);
		std::memcpy(&JmpToTrampoline[1], &TrampolineOffset, 4);
		std::memcpy(reinterpret_cast<void*>(GChangeItemNumAddr), JmpToTrampoline, 5);

		VirtualProtect(reinterpret_cast<void*>(GChangeItemNumAddr), 14, OldProtect, &OldProtect);
		GInlineHookInstalled = true;
		std::cout << "[SDK] ItemNoDecrease hook enabled\n";
	}
}

// 禁用物品不减 Hook
static void DisableItemNoDecreaseHook()
{
	if (!GChangeItemNumAddr || !GInlineHookInstalled)
		return;

	DWORD OldProtect;
	if (VirtualProtect(reinterpret_cast<void*>(GChangeItemNumAddr), 14, PAGE_EXECUTE_READWRITE, &OldProtect))
	{
		// 恢复原始字节
		std::memcpy(reinterpret_cast<void*>(GChangeItemNumAddr), GOriginalChangeItemNumBytes, 5);
		VirtualProtect(reinterpret_cast<void*>(GChangeItemNumAddr), 14, OldProtect, &OldProtect);
		GInlineHookInstalled = false;
		std::cout << "[SDK] ItemNoDecrease hook disabled\n";
	}
}

static void RemoveAllHooks()
{
	if (GHooksRemoved.exchange(true, std::memory_order_acq_rel))
		return;

	GVCPostRenderHook.Remove();
	OriginalGVCPostRender = nullptr;
	std::cout << "[SDK] GVC PostRender unhooked\n";

	// 禁用物品不减 Hook
	DisableItemNoDecreaseHook();

	// 释放跳板内存
	if (GHookTrampoline)
	{
		VirtualFree(GHookTrampoline, 0, MEM_RELEASE);
		GHookTrampoline = nullptr;
	}
}

static void CloseConsoleSafely()
{
	if (GConsoleClosed.exchange(true, std::memory_order_acq_rel))
		return;

	ShutdownLocalLogging();
	if (GetConsoleWindow())
		FreeConsole();
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

	// 初始化物品不减 Hook（保存原始字节和分配跳板，但不安装hook）
	// 偏移地址: 0x1206A70
	HMODULE hGame = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (hGame)
	{
		GChangeItemNumAddr = reinterpret_cast<uintptr_t>(hGame) + 0x1206A70;
		std::cout << "[SDK] ChangeItemNum target address: " << std::hex << GChangeItemNumAddr << std::dec << "\n";

		// 保存原始代码 (前5字节)
		std::memcpy(GOriginalChangeItemNumBytes, reinterpret_cast<void*>(GChangeItemNumAddr), 5);

		// 分配跳板内存
		GHookTrampoline = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!GHookTrampoline)
		{
			std::cout << "[SDK] Failed to allocate trampoline memory\n";
		}
		else
		{
			// 写入跳板代码
			unsigned char* Trampoline = static_cast<unsigned char*>(GHookTrampoline);
			size_t Offset = 0;

			// 保存寄存器 (rbx)
			Trampoline[Offset++] = 0x48;  // rex.w
			Trampoline[Offset++] = 0x89;
			Trampoline[Offset++] = 0x5C;
			Trampoline[Offset++] = 0x24;
			Trampoline[Offset++] = 0x08;  // mov [rsp+08], rbx

			// cmp r8d, 0        ; 比较 Num 参数
			Trampoline[Offset++] = 0x41;
			Trampoline[Offset++] = 0x83;
			Trampoline[Offset++] = 0xF8;
			Trampoline[Offset++] = 0x00;

			// jge +13 (跳过xor)
			Trampoline[Offset++] = 0x0F;
			Trampoline[Offset++] = 0x8D;
			int32_t JgeOffset = 13;
			std::memcpy(&Trampoline[Offset], &JgeOffset, 4);
			Offset += 4;

			// xor r8d, r8d    ; Num < 0 时设为 0
			Trampoline[Offset++] = 0x45;
			Trampoline[Offset++] = 0x31;
			Trampoline[Offset++] = 0xC0;

			// jmp back to original +5
			Trampoline[Offset++] = 0xE9;
			int32_t JmpBackOffset = static_cast<int32_t>((GChangeItemNumAddr + 5) - (reinterpret_cast<uintptr_t>(GHookTrampoline) + Offset + 4));
			std::memcpy(&Trampoline[Offset], &JmpBackOffset, 4);
			Offset += 4;

			std::cout << "[SDK] ChangeItemNum trampoline prepared\n";
		}
	}
	else
	{
		std::cout << "[SDK] JH-Win64-Shipping.exe not found\n";
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
