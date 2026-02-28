#include <Windows.h>
#include <iostream>

// For faster compilation, only Engine is included by default.
// Change to #include "SDK.hpp" if you need the full SDK.
#include "SDK/Engine_classes.hpp"
#include "VTHook.hpp"

using namespace SDK;

// ================================================================
// version.dll Proxy - forward all exports to the real System32 DLL
//
// Usage:
//   1. Build this project as version.dll (change Output File in project settings)
//   2. Place the built version.dll in the game's executable directory
//   3. The game will load your version.dll instead of the system one
//   4. All original version.dll functions are forwarded transparently
//
// NOTE: If the game uses a different DLL for hijacking (e.g. xinput1_3.dll,
//       d3d11.dll, winmm.dll), you'll need to adapt the exports accordingly.
// ================================================================

static HMODULE g_OriginalVersion = nullptr;

// Original function pointers
static FARPROC g_pGetFileVersionInfoA = nullptr;
static FARPROC g_pGetFileVersionInfoByHandle = nullptr;
static FARPROC g_pGetFileVersionInfoExA = nullptr;
static FARPROC g_pGetFileVersionInfoExW = nullptr;
static FARPROC g_pGetFileVersionInfoSizeA = nullptr;
static FARPROC g_pGetFileVersionInfoSizeExA = nullptr;
static FARPROC g_pGetFileVersionInfoSizeExW = nullptr;
static FARPROC g_pGetFileVersionInfoSizeW = nullptr;
static FARPROC g_pGetFileVersionInfoW = nullptr;
static FARPROC g_pVerFindFileA = nullptr;
static FARPROC g_pVerFindFileW = nullptr;
static FARPROC g_pVerInstallFileA = nullptr;
static FARPROC g_pVerInstallFileW = nullptr;
static FARPROC g_pVerLanguageNameA = nullptr;
static FARPROC g_pVerLanguageNameW = nullptr;
static FARPROC g_pVerQueryValueA = nullptr;
static FARPROC g_pVerQueryValueW = nullptr;

static bool LoadOriginalVersion()
{
	wchar_t sysDir[MAX_PATH];
	GetSystemDirectoryW(sysDir, MAX_PATH);
	wcscat_s(sysDir, L"\\version.dll");

	g_OriginalVersion = LoadLibraryW(sysDir);
	if (!g_OriginalVersion)
		return false;

	g_pGetFileVersionInfoA       = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoA");
	g_pGetFileVersionInfoByHandle= GetProcAddress(g_OriginalVersion, "GetFileVersionInfoByHandle");
	g_pGetFileVersionInfoExA     = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoExA");
	g_pGetFileVersionInfoExW     = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoExW");
	g_pGetFileVersionInfoSizeA   = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeA");
	g_pGetFileVersionInfoSizeExA = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeExA");
	g_pGetFileVersionInfoSizeExW = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeExW");
	g_pGetFileVersionInfoSizeW   = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoSizeW");
	g_pGetFileVersionInfoW       = GetProcAddress(g_OriginalVersion, "GetFileVersionInfoW");
	g_pVerFindFileA              = GetProcAddress(g_OriginalVersion, "VerFindFileA");
	g_pVerFindFileW              = GetProcAddress(g_OriginalVersion, "VerFindFileW");
	g_pVerInstallFileA           = GetProcAddress(g_OriginalVersion, "VerInstallFileA");
	g_pVerInstallFileW           = GetProcAddress(g_OriginalVersion, "VerInstallFileW");
	g_pVerLanguageNameA          = GetProcAddress(g_OriginalVersion, "VerLanguageNameA");
	g_pVerLanguageNameW          = GetProcAddress(g_OriginalVersion, "VerLanguageNameW");
	g_pVerQueryValueA            = GetProcAddress(g_OriginalVersion, "VerQueryValueA");
	g_pVerQueryValueW            = GetProcAddress(g_OriginalVersion, "VerQueryValueW");

	return true;
}

// Exported proxy functions - forward to the real version.dll
// Each function uses __declspec(naked) to avoid stack frame overhead

#define PROXY_EXPORT extern "C" __declspec(dllexport)

PROXY_EXPORT BOOL WINAPI GetFileVersionInfoA(LPCSTR a, DWORD b, DWORD c, LPVOID d)
{ return reinterpret_cast<decltype(&GetFileVersionInfoA)>(g_pGetFileVersionInfoA)(a, b, c, d); }

PROXY_EXPORT BOOL WINAPI GetFileVersionInfoW(LPCWSTR a, DWORD b, DWORD c, LPVOID d)
{ return reinterpret_cast<decltype(&GetFileVersionInfoW)>(g_pGetFileVersionInfoW)(a, b, c, d); }

PROXY_EXPORT DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR a, LPDWORD b)
{ return reinterpret_cast<decltype(&GetFileVersionInfoSizeA)>(g_pGetFileVersionInfoSizeA)(a, b); }

PROXY_EXPORT DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR a, LPDWORD b)
{ return reinterpret_cast<decltype(&GetFileVersionInfoSizeW)>(g_pGetFileVersionInfoSizeW)(a, b); }

PROXY_EXPORT BOOL WINAPI GetFileVersionInfoExA(DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e)
{ return reinterpret_cast<decltype(&GetFileVersionInfoExA)>(g_pGetFileVersionInfoExA)(a, b, c, d, e); }

PROXY_EXPORT BOOL WINAPI GetFileVersionInfoExW(DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e)
{ return reinterpret_cast<decltype(&GetFileVersionInfoExW)>(g_pGetFileVersionInfoExW)(a, b, c, d, e); }

PROXY_EXPORT DWORD WINAPI GetFileVersionInfoSizeExA(DWORD a, LPCSTR b, LPDWORD c)
{ return reinterpret_cast<decltype(&GetFileVersionInfoSizeExA)>(g_pGetFileVersionInfoSizeExA)(a, b, c); }

PROXY_EXPORT DWORD WINAPI GetFileVersionInfoSizeExW(DWORD a, LPCWSTR b, LPDWORD c)
{ return reinterpret_cast<decltype(&GetFileVersionInfoSizeExW)>(g_pGetFileVersionInfoSizeExW)(a, b, c); }

PROXY_EXPORT BOOL WINAPI VerQueryValueA(LPCVOID a, LPCSTR b, LPVOID* c, PUINT d)
{ return reinterpret_cast<decltype(&VerQueryValueA)>(g_pVerQueryValueA)(a, b, c, d); }

PROXY_EXPORT BOOL WINAPI VerQueryValueW(LPCVOID a, LPCWSTR b, LPVOID* c, PUINT d)
{ return reinterpret_cast<decltype(&VerQueryValueW)>(g_pVerQueryValueW)(a, b, c, d); }

PROXY_EXPORT DWORD WINAPI VerFindFileA(DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPSTR e, PUINT f, LPSTR g, PUINT h)
{ return reinterpret_cast<decltype(&VerFindFileA)>(g_pVerFindFileA)(a, b, c, d, e, f, g, h); }

PROXY_EXPORT DWORD WINAPI VerFindFileW(DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPWSTR e, PUINT f, LPWSTR g, PUINT h)
{ return reinterpret_cast<decltype(&VerFindFileW)>(g_pVerFindFileW)(a, b, c, d, e, f, g, h); }

PROXY_EXPORT DWORD WINAPI VerInstallFileA(DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPCSTR e, LPCSTR f, LPSTR g, PUINT h)
{ return reinterpret_cast<decltype(&VerInstallFileA)>(g_pVerInstallFileA)(a, b, c, d, e, f, g, h); }

PROXY_EXPORT DWORD WINAPI VerInstallFileW(DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPCWSTR e, LPCWSTR f, LPWSTR g, PUINT h)
{ return reinterpret_cast<decltype(&VerInstallFileW)>(g_pVerInstallFileW)(a, b, c, d, e, f, g, h); }

PROXY_EXPORT DWORD WINAPI VerLanguageNameA(DWORD a, LPSTR b, DWORD c)
{ return reinterpret_cast<decltype(&VerLanguageNameA)>(g_pVerLanguageNameA)(a, b, c); }

PROXY_EXPORT DWORD WINAPI VerLanguageNameW(DWORD a, LPWSTR b, DWORD c)
{ return reinterpret_cast<decltype(&VerLanguageNameW)>(g_pVerLanguageNameW)(a, b, c); }

PROXY_EXPORT int WINAPI GetFileVersionInfoByHandle(int a, LPCWSTR b, DWORD c, DWORD d)
{ return reinterpret_cast<decltype(&GetFileVersionInfoByHandle)>(g_pGetFileVersionInfoByHandle)(a, b, c, d); }

// -- GVC PostRender Hook (delayed - waits for UE engine initialization) --

using GVCPostRenderFn = void(__fastcall*)(void* /* this */, void* /* Canvas */);
static GVCPostRenderFn OriginalGVCPostRender = nullptr;
static VTableHook GVCPostRenderHook;

void __fastcall HookedGVCPostRender(void* This, void* Canvas)
{
	if (OriginalGVCPostRender)
		OriginalGVCPostRender(This, Canvas);

	static DWORD LastPrint = 0;
	DWORD Now = GetTickCount();
	if (Now - LastPrint > 1000)
	{
		std::cout << "[SDK-Proxy] GVC PostRender called\n";
		LastPrint = Now;
	}
}

// -- Main Thread - polls until UE engine is ready, then hooks --

DWORD ProxyMainThread(HMODULE Module)
{
	AllocConsole();
	FILE* Dummy;
	freopen_s(&Dummy, "CONOUT$", "w", stdout);
	freopen_s(&Dummy, "CONIN$", "r", stdin);

	std::cout << "[SDK-Proxy] version.dll proxy loaded. Waiting for engine init...\n";

	// -- Phase 1: Wait for UE engine to initialize --
	// Proxy DLLs load BEFORE the game engine starts.
	// We must poll until GObjects and UWorld are available.
	// Typical wait time: 5-30 seconds depending on the game.

	if constexpr (Offsets::GVCPostRenderIdx >= 0)
	{
		UGameViewportClient* GVC = nullptr;

		for (int Attempt = 0; Attempt < 600; Attempt++) // Up to 60 seconds
		{
			if (GetAsyncKeyState(VK_DELETE) & 1)
			{
				std::cout << "[SDK-Proxy] Aborted by user during init wait.\n";
				goto cleanup;
			}

			Sleep(100);

			// Try to get ViewportClient via LocalPlayer - will be null until engine is fully initialized
			__try
			{
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
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				GVC = nullptr;
			}

			if (GVC)
				break;
		}

		if (!GVC)
		{
			std::cout << "[SDK-Proxy] Timed out waiting for ViewportClient.\n";
			std::cout << "[SDK-Proxy] Press DELETE to unload.\n";
			goto hotkey_loop;
		}

		// -- Phase 2: Engine is ready - install hook --
		std::cout << "[SDK-Proxy] Engine initialized. Installing PostRender hook...\n";

		GVCPostRenderHook = VTableHook(GVC, Offsets::GVCPostRenderIdx);
		OriginalGVCPostRender = GVCPostRenderHook.Install<GVCPostRenderFn>(HookedGVCPostRender);

		if (OriginalGVCPostRender)
			std::cout << "[SDK-Proxy] GVC PostRender hooked at index " << Offsets::GVCPostRenderIdx << "\n";
		else
			std::cout << "[SDK-Proxy] Failed to hook GVC PostRender.\n";
	}
	else
	{
		std::cout << "[SDK-Proxy] GVCPostRenderIdx not detected (-1), hook skipped.\n";
	}

	std::cout << "[SDK-Proxy] Press DELETE to unload.\n";

hotkey_loop:
	while (true)
	{
		if (GetAsyncKeyState(VK_DELETE) & 1)
			break;
		Sleep(100);
	}

cleanup:
	std::cout << "[SDK-Proxy] Unloading...\n";
	GVCPostRenderHook.Remove();
	Sleep(200);

	// Keep CRT stdio handles untouched to avoid SECURE CRT invalid-parameter on some builds.`r`n	FreeLibraryAndExitThread(Module, 0);

	return 0;
}

// -- DllMain --

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		if (!LoadOriginalVersion())
			return FALSE; // Critical: can't forward exports without the real DLL
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ProxyMainThread, hModule, 0, 0);
		break;

	case DLL_PROCESS_DETACH:
		if (g_OriginalVersion)
			FreeLibrary(g_OriginalVersion);
		break;
	}

	return TRUE;
}


