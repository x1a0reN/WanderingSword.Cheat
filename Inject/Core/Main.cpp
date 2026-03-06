// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  Main.cpp  —  DLL 入口点 · 注入初始化与安全卸载
//
//  DllMain 在 DLL_PROCESS_ATTACH 时启动工作线程；
//  MainThread 负责：
//    1. 初始化控制台与日志系统
//    2. 定位 UGameViewportClient 并安装 PostRender VTable Hook
//    3. 监听 DELETE 键触发安全三阶段卸载流程
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#include <Windows.h>
#include <cstdio>

#include "CheatState.hpp"
#include "GCManager.hpp"
#include "Logging.hpp"
#include "FrameHook.hpp"
#include "PanelManager.hpp"
#include "InlineHook.hpp"

// ┌─────────────────────────────────────────────────────────────┐
// │                     内部辅助函数                             │
// └─────────────────────────────────────────────────────────────┘

/// 一次性移除所有已安装的 Hook (VTable + Inline)。
/// 使用 atomic exchange 保证仅执行一次。
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

/// 安全释放调试控制台：先关闭日志文件再释放控制台窗口。
static void CloseConsoleSafely()
{
	if (GConsoleClosed.exchange(true, std::memory_order_acq_rel))
		return;

	ShutdownLocalLogging();
	if (GetConsoleWindow())
		FreeConsole();
}
// ┌─────────────────────────────────────────────────────────────┐
// │                   工作线程 · 注入主流程                      │
// └─────────────────────────────────────────────────────────────┘

/// 注入后的独立工作线程入口。
/// 执行两大阶段：初始化阶段 + DELETE 键触发的安全卸载流程。
DWORD MainThread(HMODULE Module)
{
	// ── 初始化：控制台 + 日志 + 崩溃过滤器 ──
	AllocConsole();
	FILE* Dummy = nullptr;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);
	SetupLocalLogging(Module);
	SetUnhandledExceptionFilter(UnhandledExceptionLogger);

	LOGI_STREAM("Main") << "[SDK] DLL Loaded. Press HOME to toggle internal widget, DELETE to unload.\n" << std::endl;

	// ── 初始化：定位 ViewportClient 并安装 PostRender VTable Hook ──
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
	// ── 主循环：等待 DELETE 键触发安全卸载 ──
	//
	// 卸载采用三阶段 fail-closed 设计，任何一阶段超时都将
	// 中止本次卸载并保持 DLL 驻留，避免崩溃：
	//   Phase 1  等待游戏线程完成 UI 资源清理
	//   Phase 2  确认当前无 PostRender 回调正在执行，然后摘钩
	//   Phase 3  摘钩后再次确认无逃逸调用，最终释放线程

	constexpr DWORD kCleanupTimeoutMs  = 2500;
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

		// Phase 1: 等待游戏线程完成 UI / GC 资源清理
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
		// Phase 2: 确认无 PostRender 调用正在执行，然后摘除 Hook
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

		// Phase 3: 摘钩后确认无逃逸调用
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
		// 最终关卡：双重断言通过后才允许释放
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

// ┌─────────────────────────────────────────────────────────────┐
// │                     DLL 入口点                              │
// └─────────────────────────────────────────────────────────────┘

/// Windows DLL 入口。仅处理 DLL_PROCESS_ATTACH：
/// 禁用线程通知以减少开销，启动独立工作线程执行 MainThread。
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		HANDLE ThreadHandle = CreateThread(
			nullptr, 0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread),
			hModule, 0, nullptr);
		if (ThreadHandle)
			CloseHandle(ThreadHandle);
	}

	return TRUE;
}
