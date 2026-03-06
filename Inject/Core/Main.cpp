#include <Windows.h>
#include <cstdio>

#include "CheatState.hpp"
#include "GCManager.hpp"
#include "Logging.hpp"
#include "FrameHook.hpp"
#include "PanelManager.hpp"
#include "InlineHook.hpp"

static void RemoveAllHooks()
{
	if (GHooksRemoved.exchange(true, std::memory_order_acq_rel))
		return;

	GVCPostRenderHook.Remove();
	GOriginalPostRender = nullptr;
	LOGI_STREAM("Main") << "[SDK] GVC PostRender unhooked\n";

	DisableItemNoDecreaseHook();

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

DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy = nullptr;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);
	SetupLocalLogging(Module);
	SetUnhandledExceptionFilter(UnhandledExceptionLogger);

	LOGI_STREAM("Main") << "[SDK] DLL Loaded. Press HOME to toggle internal widget, DELETE to unload.\n" << std::endl;

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
			GOriginalPostRender = GVCPostRenderHook.Install<GVCPostRenderFn>(HookedGVCPostRender);

			if (GOriginalPostRender)
				LOGI_STREAM("Main") << "[SDK] GVC PostRender hooked at index " << Offsets::GVCPostRenderIdx << "\n";
			else
				LOGI_STREAM("Main") << "[SDK] Failed to hook GVC PostRender (VirtualProtect failed)\n";
		}
		else
			LOGI_STREAM("Main") << "[SDK] ViewportClient is null, hook skipped\n";
	}
	else
	{
		LOGI_STREAM("Main") << "[SDK] GVCPostRenderIdx not detected (-1), hook skipped\n";
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

		LOGI_STREAM("Main") << "[SDK] Unloading...\n";
		GUnloadCleanupDone.store(false, std::memory_order_release);
		GIsUnloading.store(true, std::memory_order_release);

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
			LOGI_STREAM("Main") << "[SDK] Unload refused (fail-closed): cleanup timeout, DLL kept loaded\n";
			continue;
		}

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
			LOGI_STREAM("Main") << "[SDK] Unload refused (fail-closed): in-flight timeout before unhook, DLL kept loaded\n";
			continue;
		}

		RemoveAllHooks();

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
			LOGI_STREAM("Main") << "[SDK] Unload refused (fail-closed): in-flight timeout after unhook, DLL kept loaded\n";
			continue;
		}

		if (!GUnloadCleanupDone.load(std::memory_order_acquire) ||
			GPostRenderInFlight.load(std::memory_order_acquire) != 0)
		{
			GIsUnloading.store(false, std::memory_order_release);
			LOGI_STREAM("Main") << "[SDK] Unload refused (fail-closed): final gate check failed, DLL kept loaded\n";
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
