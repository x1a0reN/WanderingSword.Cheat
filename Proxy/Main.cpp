#include <Windows.h>
#include <cstdio>

#include "CheatState.hpp"
#include "Logging.hpp"
#include "FrameHook.hpp"
#include "PanelManager.hpp"
#include "InlineHook.hpp"

using namespace SDK;

// ================================================================
// version.dll Proxy - forward exports to the real System32 DLL
// and host the full internal trainer runtime inside the proxy DLL.
// ================================================================

static HMODULE g_OriginalVersion = nullptr;
static INIT_ONCE g_OriginalVersionInitOnce = INIT_ONCE_STATIC_INIT;

static decltype(&::GetFileVersionInfoA) g_pGetFileVersionInfoA = nullptr;
using GetFileVersionInfoByHandleFn = int (WINAPI*)(int, LPCWSTR, DWORD, DWORD);
static GetFileVersionInfoByHandleFn g_pGetFileVersionInfoByHandle = nullptr;
static decltype(&::GetFileVersionInfoExA) g_pGetFileVersionInfoExA = nullptr;
static decltype(&::GetFileVersionInfoExW) g_pGetFileVersionInfoExW = nullptr;
static decltype(&::GetFileVersionInfoSizeA) g_pGetFileVersionInfoSizeA = nullptr;
static decltype(&::GetFileVersionInfoSizeExA) g_pGetFileVersionInfoSizeExA = nullptr;
static decltype(&::GetFileVersionInfoSizeExW) g_pGetFileVersionInfoSizeExW = nullptr;
static decltype(&::GetFileVersionInfoSizeW) g_pGetFileVersionInfoSizeW = nullptr;
static decltype(&::GetFileVersionInfoW) g_pGetFileVersionInfoW = nullptr;
static decltype(&::VerFindFileA) g_pVerFindFileA = nullptr;
static decltype(&::VerFindFileW) g_pVerFindFileW = nullptr;
static decltype(&::VerInstallFileA) g_pVerInstallFileA = nullptr;
static decltype(&::VerInstallFileW) g_pVerInstallFileW = nullptr;
static decltype(&::VerLanguageNameA) g_pVerLanguageNameA = nullptr;
static decltype(&::VerLanguageNameW) g_pVerLanguageNameW = nullptr;
static decltype(&::VerQueryValueA) g_pVerQueryValueA = nullptr;
static decltype(&::VerQueryValueW) g_pVerQueryValueW = nullptr;

static BOOL CALLBACK InitOriginalVersion(PINIT_ONCE, PVOID, PVOID*)
{
	wchar_t sysDir[MAX_PATH] = {};
	GetSystemDirectoryW(sysDir, MAX_PATH);
	wcscat_s(sysDir, L"\\version.dll");

	g_OriginalVersion = LoadLibraryW(sysDir);
	if (!g_OriginalVersion)
		return FALSE;

	g_pGetFileVersionInfoA = reinterpret_cast<decltype(g_pGetFileVersionInfoA)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoA"));
	g_pGetFileVersionInfoByHandle = reinterpret_cast<decltype(g_pGetFileVersionInfoByHandle)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoByHandle"));
	g_pGetFileVersionInfoExA = reinterpret_cast<decltype(g_pGetFileVersionInfoExA)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoExA"));
	g_pGetFileVersionInfoExW = reinterpret_cast<decltype(g_pGetFileVersionInfoExW)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoExW"));
	g_pGetFileVersionInfoSizeA = reinterpret_cast<decltype(g_pGetFileVersionInfoSizeA)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeA"));
	g_pGetFileVersionInfoSizeExA = reinterpret_cast<decltype(g_pGetFileVersionInfoSizeExA)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeExA"));
	g_pGetFileVersionInfoSizeExW = reinterpret_cast<decltype(g_pGetFileVersionInfoSizeExW)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeExW"));
	g_pGetFileVersionInfoSizeW = reinterpret_cast<decltype(g_pGetFileVersionInfoSizeW)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeW"));
	g_pGetFileVersionInfoW = reinterpret_cast<decltype(g_pGetFileVersionInfoW)>(GetProcAddress(g_OriginalVersion, "GetFileVersionInfoW"));
	g_pVerFindFileA = reinterpret_cast<decltype(g_pVerFindFileA)>(GetProcAddress(g_OriginalVersion, "VerFindFileA"));
	g_pVerFindFileW = reinterpret_cast<decltype(g_pVerFindFileW)>(GetProcAddress(g_OriginalVersion, "VerFindFileW"));
	g_pVerInstallFileA = reinterpret_cast<decltype(g_pVerInstallFileA)>(GetProcAddress(g_OriginalVersion, "VerInstallFileA"));
	g_pVerInstallFileW = reinterpret_cast<decltype(g_pVerInstallFileW)>(GetProcAddress(g_OriginalVersion, "VerInstallFileW"));
	g_pVerLanguageNameA = reinterpret_cast<decltype(g_pVerLanguageNameA)>(GetProcAddress(g_OriginalVersion, "VerLanguageNameA"));
	g_pVerLanguageNameW = reinterpret_cast<decltype(g_pVerLanguageNameW)>(GetProcAddress(g_OriginalVersion, "VerLanguageNameW"));
	g_pVerQueryValueA = reinterpret_cast<decltype(g_pVerQueryValueA)>(GetProcAddress(g_OriginalVersion, "VerQueryValueA"));
	g_pVerQueryValueW = reinterpret_cast<decltype(g_pVerQueryValueW)>(GetProcAddress(g_OriginalVersion, "VerQueryValueW"));

	return TRUE;
}

static bool EnsureOriginalVersionLoaded()
{
	BOOL Pending = FALSE;
	return InitOnceExecuteOnce(&g_OriginalVersionInitOnce, InitOriginalVersion, nullptr, nullptr) != FALSE
		&& g_OriginalVersion != nullptr;
}

#pragma comment(linker, "/EXPORT:GetFileVersionInfoA=Proxy_GetFileVersionInfoA,@1")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoByHandle=Proxy_GetFileVersionInfoByHandle,@2")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoExA=Proxy_GetFileVersionInfoExA,@3")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoExW=Proxy_GetFileVersionInfoExW,@4")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeA=Proxy_GetFileVersionInfoSizeA,@5")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeExA=Proxy_GetFileVersionInfoSizeExA,@6")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeExW=Proxy_GetFileVersionInfoSizeExW,@7")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeW=Proxy_GetFileVersionInfoSizeW,@8")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoW=Proxy_GetFileVersionInfoW,@9")
#pragma comment(linker, "/EXPORT:VerFindFileA=Proxy_VerFindFileA,@10")
#pragma comment(linker, "/EXPORT:VerFindFileW=Proxy_VerFindFileW,@11")
#pragma comment(linker, "/EXPORT:VerInstallFileA=Proxy_VerInstallFileA,@12")
#pragma comment(linker, "/EXPORT:VerInstallFileW=Proxy_VerInstallFileW,@13")
#pragma comment(linker, "/EXPORT:VerLanguageNameA=Proxy_VerLanguageNameA,@14")
#pragma comment(linker, "/EXPORT:VerLanguageNameW=Proxy_VerLanguageNameW,@15")
#pragma comment(linker, "/EXPORT:VerQueryValueA=Proxy_VerQueryValueA,@16")
#pragma comment(linker, "/EXPORT:VerQueryValueW=Proxy_VerQueryValueW,@17")

extern "C" BOOL WINAPI Proxy_GetFileVersionInfoA(LPCSTR a, DWORD b, DWORD c, LPVOID d)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoA)
		return FALSE;
	return g_pGetFileVersionInfoA(a, b, c, d);
}

extern "C" BOOL WINAPI Proxy_GetFileVersionInfoW(LPCWSTR a, DWORD b, DWORD c, LPVOID d)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoW)
		return FALSE;
	return g_pGetFileVersionInfoW(a, b, c, d);
}

extern "C" DWORD WINAPI Proxy_GetFileVersionInfoSizeA(LPCSTR a, LPDWORD b)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoSizeA)
		return 0;
	return g_pGetFileVersionInfoSizeA(a, b);
}

extern "C" DWORD WINAPI Proxy_GetFileVersionInfoSizeW(LPCWSTR a, LPDWORD b)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoSizeW)
		return 0;
	return g_pGetFileVersionInfoSizeW(a, b);
}

extern "C" BOOL WINAPI Proxy_GetFileVersionInfoExA(DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoExA)
		return FALSE;
	return g_pGetFileVersionInfoExA(a, b, c, d, e);
}

extern "C" BOOL WINAPI Proxy_GetFileVersionInfoExW(DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoExW)
		return FALSE;
	return g_pGetFileVersionInfoExW(a, b, c, d, e);
}

extern "C" DWORD WINAPI Proxy_GetFileVersionInfoSizeExA(DWORD a, LPCSTR b, LPDWORD c)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoSizeExA)
		return 0;
	return g_pGetFileVersionInfoSizeExA(a, b, c);
}

extern "C" DWORD WINAPI Proxy_GetFileVersionInfoSizeExW(DWORD a, LPCWSTR b, LPDWORD c)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoSizeExW)
		return 0;
	return g_pGetFileVersionInfoSizeExW(a, b, c);
}

extern "C" BOOL WINAPI Proxy_VerQueryValueA(LPCVOID a, LPCSTR b, LPVOID* c, PUINT d)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerQueryValueA)
		return FALSE;
	return g_pVerQueryValueA(a, b, c, d);
}

extern "C" BOOL WINAPI Proxy_VerQueryValueW(LPCVOID a, LPCWSTR b, LPVOID* c, PUINT d)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerQueryValueW)
		return FALSE;
	return g_pVerQueryValueW(a, b, c, d);
}

extern "C" DWORD WINAPI Proxy_VerFindFileA(DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPSTR e, PUINT f, LPSTR g, PUINT h)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerFindFileA)
		return 0;
	return g_pVerFindFileA(a, b, c, d, e, f, g, h);
}

extern "C" DWORD WINAPI Proxy_VerFindFileW(DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPWSTR e, PUINT f, LPWSTR g, PUINT h)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerFindFileW)
		return 0;
	return g_pVerFindFileW(a, b, c, d, e, f, g, h);
}

extern "C" DWORD WINAPI Proxy_VerInstallFileA(DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPCSTR e, LPCSTR f, LPSTR g, PUINT h)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerInstallFileA)
		return 0;
	return g_pVerInstallFileA(a, b, c, d, e, f, g, h);
}

extern "C" DWORD WINAPI Proxy_VerInstallFileW(DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPCWSTR e, LPCWSTR f, LPWSTR g, PUINT h)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerInstallFileW)
		return 0;
	return g_pVerInstallFileW(a, b, c, d, e, f, g, h);
}

extern "C" DWORD WINAPI Proxy_VerLanguageNameA(DWORD a, LPSTR b, DWORD c)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerLanguageNameA)
		return 0;
	return g_pVerLanguageNameA(a, b, c);
}

extern "C" DWORD WINAPI Proxy_VerLanguageNameW(DWORD a, LPWSTR b, DWORD c)
{
	if (!EnsureOriginalVersionLoaded() || !g_pVerLanguageNameW)
		return 0;
	return g_pVerLanguageNameW(a, b, c);
}

extern "C" int WINAPI Proxy_GetFileVersionInfoByHandle(int a, LPCWSTR b, DWORD c, DWORD d)
{
	if (!EnsureOriginalVersionLoaded() || !g_pGetFileVersionInfoByHandle)
		return 0;
	return g_pGetFileVersionInfoByHandle(a, b, c, d);
}

namespace
{
	UGameViewportClient* TryGetViewportClient()
	{
		UGameViewportClient* GVC = nullptr;

		__try
		{
			UWorld* World = UWorld::GetWorld();
			if (!World || !World->PersistentLevel)
				return nullptr;

			UWorld* OwningWorld = World->PersistentLevel->OwningWorld;
			if (!OwningWorld || !OwningWorld->OwningGameInstance)
				return nullptr;

			if (OwningWorld->OwningGameInstance->LocalPlayers.Num() <= 0)
				return nullptr;

			ULocalPlayer* LP = OwningWorld->OwningGameInstance->LocalPlayers[0];
			if (!LP)
				return nullptr;

			GVC = LP->ViewportClient;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			GVC = nullptr;
		}

		return GVC;
	}

	bool InstallPostRenderHookWhenReady()
	{
		if constexpr (Offsets::GVCPostRenderIdx < 0)
		{
			LOGI_STREAM("Main") << "[SDK-Proxy] GVCPostRenderIdx not detected (-1), hook skipped\n";
			return false;
		}

		DWORD LastLogTick = 0;
		int32 Attempts = 0;

		for (;;)
		{
			if (GetAsyncKeyState(VK_DELETE) & 1)
			{
				LOGI_STREAM("Main") << "[SDK-Proxy] Init wait aborted by DELETE before hook install\n";
				return false;
			}

			UGameViewportClient* GVC = TryGetViewportClient();
			if (GVC)
			{
				GVCPostRenderHook = VTableHook(GVC, Offsets::GVCPostRenderIdx);
				GOriginalPostRender = GVCPostRenderHook.Install<GVCPostRenderFn>(HookedGVCPostRender);
				if (GOriginalPostRender)
				{
					LOGI_STREAM("Main") << "[SDK-Proxy] GVC PostRender hooked at index " << Offsets::GVCPostRenderIdx << "\n";
					return true;
				}

				LOGI_STREAM("Main") << "[SDK-Proxy] Found ViewportClient but hook install failed, retrying...\n";
			}
			else
			{
				const DWORD Now = GetTickCount();
				if (Now - LastLogTick >= 5000)
				{
					LOGI_STREAM("Main") << "[SDK-Proxy] Waiting for engine init... attempts=" << Attempts << "\n";
					LastLogTick = Now;
				}
			}

			++Attempts;
			Sleep(100);
		}
	}

	void RequestProxySoftReset()
	{
		constexpr DWORD kCleanupTimeoutMs = 2500;

		LOGI_STREAM("Main") << "[SDK-Proxy] DELETE pressed: resetting trainer runtime (proxy stays loaded)\n";
		GUnloadCleanupDone.store(false, std::memory_order_release);
		GIsUnloading.store(true, std::memory_order_release);

		bool CleanupDone = false;
		const DWORD WaitStart = GetTickCount();
		while (!(CleanupDone = GUnloadCleanupDone.load(std::memory_order_acquire)))
		{
			if (GetTickCount() - WaitStart > kCleanupTimeoutMs)
				break;
			Sleep(10);
		}

		GIsUnloading.store(false, std::memory_order_release);
		GUnloadCleanupDone.store(false, std::memory_order_release);

		if (CleanupDone)
		{
			LOGI_STREAM("Main") << "[SDK-Proxy] Runtime reset complete. Press HOME to reopen the panel.\n";
		}
		else
		{
			LOGI_STREAM("Main") << "[SDK-Proxy] Runtime reset timed out. Keeping hook alive for safety.\n";
		}
	}
}

DWORD ProxyMainThread(HMODULE Module)
{
	SetupLocalLogging(Module);
	SetUnhandledExceptionFilter(UnhandledExceptionLogger);

	LOGI_STREAM("Main") << "[SDK-Proxy] version.dll proxy loaded. Press HOME to toggle internal widget, DELETE to reset features.\n";

	InstallPostRenderHookWhenReady();

	for (;;)
	{
		if (GetAsyncKeyState(VK_DELETE) & 1)
			RequestProxySoftReset();

		Sleep(100);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		{
			HANDLE ThreadHandle = CreateThread(
				nullptr,
				0,
				reinterpret_cast<LPTHREAD_START_ROUTINE>(ProxyMainThread),
				hModule,
				0,
				nullptr);
			if (ThreadHandle)
				CloseHandle(ThreadHandle);
		}
		break;

	case DLL_PROCESS_DETACH:
		if (g_OriginalVersion)
			FreeLibrary(g_OriginalVersion);
		break;
	}

	return TRUE;
}
