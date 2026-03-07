void PopulateTab_Life(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->GamepadSlot, "Tab3(GamepadSlot)");
	if (!Container) return;
	Container->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = Container->AddChild(Panel);
		if (Slot && Slot->IsA(UVerticalBoxSlot::StaticClass()))
		{
			auto* VSlot = static_cast<UVerticalBoxSlot*>(Slot);
			FMargin Pad{};
			Pad.Top = TopGap;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"生活开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	GTab3.CraftIgnoreRequirementsToggle = CreateToggleItem(PC, L"锻造/制衣/炼丹/烹饪无视要求");
	if (GTab3.CraftIgnoreRequirementsToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.CraftIgnoreRequirementsToggle); else Container->AddChild(GTab3.CraftIgnoreRequirementsToggle); Count++; }
	GTab3.CraftOutputQuantityEdit = CreateVolumeNumericEditBoxItem(PC, Outer, SwitchBox ? SwitchBox : Container, L"产出数量", L"输入数字", L"1");
	if (GTab3.CraftOutputQuantityEdit) { if (SwitchBox) SwitchBox->AddChild(GTab3.CraftOutputQuantityEdit); else Container->AddChild(GTab3.CraftOutputQuantityEdit); Count++; }

	GTab3.GatherCooldownToggle = CreateToggleItem(PC, L"采集物每秒刷新");
	if (GTab3.GatherCooldownToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.GatherCooldownToggle); else Container->AddChild(GTab3.GatherCooldownToggle); Count++; }
	GTab3.FishRareOnlyToggle = CreateToggleItem(PC, L"钓鱼只钓稀有物");
	if (GTab3.FishRareOnlyToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.FishRareOnlyToggle); else Container->AddChild(GTab3.FishRareOnlyToggle); Count++; }
	GTab3.FishAlwaysCatchToggle = CreateToggleItem(PC, L"钓鱼收杆必有收获");
	if (GTab3.FishAlwaysCatchToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.FishAlwaysCatchToggle); else Container->AddChild(GTab3.FishAlwaysCatchToggle); Count++; }
	GTab3.HomelandHarvestToggle = CreateToggleItem(PC, L"家园随时收获");
	if (GTab3.HomelandHarvestToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.HomelandHarvestToggle); else Container->AddChild(GTab3.HomelandHarvestToggle); Count++; }
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);
}

namespace
{
uintptr_t GForgeLevelCheckAddr1 = 0;
std::vector<uintptr_t> GForgeLevelCheckAddrs;
std::vector<uintptr_t> GForgeMaterialCheckAddrs;
uintptr_t GAlchemyMoneyCheckAddr = 0;
uintptr_t GCookingMoneyCheckAddr = 0;
uintptr_t GAlchemyLvCheckAddr = 0;
uintptr_t GCookingLvCheckAddr = 0;

bool GCraftIgnoreRequirementsFeatureEnabled = false;
bool GCraftOutputQuantityFeatureEnabled = false;
bool GFusionAlertContHookEnabled = false;

// FusionAlertCont hook state
uintptr_t GFusionAlertContAddr = 0;
void*     GFusionAlertContTrampoline = nullptr;
unsigned char GFusionAlertContOrigBytes[6] = {};

volatile LONG GCraftOutputQtyFlag = 0;
volatile LONG GCraftOutputQtyVal = 1;

const char* kForgeLevelCheckPattern = "83 ?? FE 0F 85 ?? ?? ?? ?? 4C 8D 0D";
const char* kForgeMaterialCheckPattern = "?? 8B ?? 49 8B ?? E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? 00 00 4C 8D 0D";
const char* kAlchemyMoneyCheckPattern = "39 ?? 30 0F 8D ?? ?? 00 00 4C 8D";
const char* kCookingMoneyCheckPattern = "39 ?? 30 7D ?? 4C 8D";
const char* kAlchemyLvCheckPattern = "8B ?? 6C 49 8B ?? E8 ?? ?? ?? ?? 84 C0";
const char* kCookingLvCheckPattern = "8B ?? ?? 49 8B ?? E8 ?? ?? ?? ?? 84 C0 75 ?? 4C 8D";

// ── 第二层补丁 (CT: IgnoreForgeLevel / forgeMaterialEnough / fusionType) ──
uintptr_t GIgnoreForgeLevel2Addr = 0;
uintptr_t GForgeMaterialEnough2Addr = 0;
uintptr_t GFusionTypeAddr = 0;
void*     GFusionTypeTrampoline = nullptr;
unsigned char GFusionTypeOrigBytes[6] = {};

const char* kIgnoreForgeLevel2Pattern  = "?? 89 ?? ?? 48 8D 55 ?? 48 8B ?? E8 ?? ?? ?? ?? 84 C0 0F 85";
const char* kForgeMaterialEnough2Pattern = "48 8D 55 ?? 48 8B ?? E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 84 DB 0F 85";
const char* kFusionTypePattern = "48 8B 79 08 85 D2 0F 8E";

const char* kFusionAlertContPattern = "8B 81 04 03 00 00 89 44 24";

const unsigned char kFusionAlertContTrampolineCode[] = {
0x8B, 0x81, 0x04, 0x03, 0x00, 0x00, 
0x41, 0x53,
0x49, 0xBB,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00,
0x41, 0x83, 0x3B, 0x01,
0x75, 0x0D,
0x49, 0xBB,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00,
0x41, 0x8B, 0x03,
0x83, 0xF8, 0x00,
0x7F, 0x05,
0xB8, 0x01, 0x00, 0x00, 0x00,
0x89, 0x81, 0x04, 0x03, 0x00, 0x00, 
0x41, 0x5B
};
constexpr size_t kFusionAlertFlagImm64Offset = 10;
constexpr size_t kFusionAlertValImm64Offset = 26;
}

void EnableCraftIgnoreRequirements();
void DisableCraftIgnoreRequirements();
void SetCraftOutputQuantity(int32 Value);
void EnableCraftOutputQuantityHook();
void DisableCraftOutputQuantityHook();
bool EnsureFusionAlertContHookInstalled();
void DisableFusionAlertContHookIfUnused();
void SyncCraftOutputQuantityFlag();

void SyncCraftOutputQuantityFlag()
{
	const LONG enableOverride =
		(GCraftIgnoreRequirementsFeatureEnabled || GCraftOutputQuantityFeatureEnabled) ? 1L : 0L;
	InterlockedExchange(&GCraftOutputQtyFlag, enableOverride);
}

bool EnsureFusionAlertContHookInstalled()
{
	if (GFusionAlertContHookEnabled)
		return true;

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule)
	{
		LOGE_STREAM("Tab3Life") << "[SDK] FusionAlertCont failed to get module handle\n";
		return false;
	}
	const uintptr_t ModBase = reinterpret_cast<uintptr_t>(hModule);

	if (GFusionAlertContAddr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kFusionAlertContPattern);
		if (found)
		{
			GFusionAlertContAddr = found;
			LOGI_STREAM("Tab3Life") << "[SDK] FusionAlertCont found at: 0x" << std::hex << GFusionAlertContAddr
				<< ", offset: 0x" << (GFusionAlertContAddr - ModBase) << std::dec << "\n";
		}
		else
		{
			LOGE_STREAM("Tab3Life") << "[SDK] FusionAlertCont AobScan failed\n";
			return false;
		}
	}

	if (GFusionAlertContTrampoline == nullptr)
	{
		std::memcpy(GFusionAlertContOrigBytes, reinterpret_cast<void*>(GFusionAlertContAddr), sizeof(GFusionAlertContOrigBytes));

		// Allocate trampoline near target so we can keep the 5-byte entry JMP layout from CT.
		const uintptr_t allocGranularity = 0x10000;
		for (uintptr_t delta = allocGranularity; delta < 0x7FFF0000ULL && !GFusionAlertContTrampoline; delta += allocGranularity)
		{
			for (int dir = 0; dir < 2; ++dir)
			{
				uintptr_t tryAddr = dir == 0
					? (GFusionAlertContAddr > delta ? GFusionAlertContAddr - delta : 0)
					: GFusionAlertContAddr + delta;
				if (!tryAddr)
					continue;

				void* p = VirtualAlloc(reinterpret_cast<void*>(tryAddr), 256,
					MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (!p)
					continue;

				intptr_t dist = reinterpret_cast<intptr_t>(p) - static_cast<intptr_t>(GFusionAlertContAddr + 5);
				if (dist >= INT32_MIN && dist <= INT32_MAX)
				{
					GFusionAlertContTrampoline = p;
					break;
				}

				VirtualFree(p, 0, MEM_RELEASE);
			}
		}

		if (!GFusionAlertContTrampoline)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] FusionAlertCont VirtualAlloc failed\n";
			return false;
		}

		unsigned char code[sizeof(kFusionAlertContTrampolineCode)];
		std::memcpy(code, kFusionAlertContTrampolineCode, sizeof(code));

		const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GCraftOutputQtyFlag);
		const uintptr_t valAddr = reinterpret_cast<uintptr_t>(&GCraftOutputQtyVal);
		std::memcpy(&code[kFusionAlertFlagImm64Offset], &flagAddr, sizeof(flagAddr));
		std::memcpy(&code[kFusionAlertValImm64Offset], &valAddr, sizeof(valAddr));
		std::memcpy(GFusionAlertContTrampoline, code, sizeof(code));

		uint8_t* trampEnd = reinterpret_cast<uint8_t*>(GFusionAlertContTrampoline) + sizeof(code);
		trampEnd[0] = 0xFF;
		trampEnd[1] = 0x25;
		*reinterpret_cast<uint32_t*>(trampEnd + 2) = 0;
		const uintptr_t retAddr = GFusionAlertContAddr + sizeof(GFusionAlertContOrigBytes);
		std::memcpy(trampEnd + 6, &retAddr, sizeof(retAddr));
	}

	unsigned char jmpPatch[sizeof(GFusionAlertContOrigBytes)] = {};
	jmpPatch[0] = 0xE9;
	const int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(GFusionAlertContTrampoline) - (GFusionAlertContAddr + 5));
	std::memcpy(&jmpPatch[1], &rel, sizeof(rel));
	jmpPatch[5] = 0x90;
	InlineHook::HookManager::WriteMemory(GFusionAlertContAddr, jmpPatch, sizeof(jmpPatch));

	GFusionAlertContHookEnabled = true;
	LOGI_STREAM("Tab3Life") << "[SDK] FusionAlertCont hook enabled, trampoline at: " << GFusionAlertContTrampoline << "\n";
	return true;
}

void DisableFusionAlertContHookIfUnused()
{
	if (GCraftIgnoreRequirementsFeatureEnabled || GCraftOutputQuantityFeatureEnabled)
		return;
	if (!GFusionAlertContHookEnabled || !GFusionAlertContAddr || !GFusionAlertContTrampoline)
		return;

	InlineHook::HookManager::WriteMemory(GFusionAlertContAddr, GFusionAlertContOrigBytes, sizeof(GFusionAlertContOrigBytes));
	GFusionAlertContHookEnabled = false;
	LOGI_STREAM("Tab3Life") << "[SDK] FusionAlertCont hook disabled\n";
}

void EnableCraftIgnoreRequirements()
{
	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule)
	{
		LOGE_STREAM("Tab3Life") << "[SDK] CraftIgnoreRequirements failed to get module handle\n";
		return;
	}
	const uintptr_t ModBase = reinterpret_cast<uintptr_t>(hModule);
	GCraftIgnoreRequirementsFeatureEnabled = true;

	// 1. ForgeLevelCheck (Multi-match)
	if (GForgeLevelCheckAddrs.empty())
	{
		GForgeLevelCheckAddrs = InlineHook::HookManager::AobScanModule(
			"JH-Win64-Shipping.exe", kForgeLevelCheckPattern, 256, true);
		if (GForgeLevelCheckAddrs.empty())
			LOGE_STREAM("Tab3Life") << "[SDK] ForgeLevelCheck AobScan failed\n";
		else
		{
			for (auto& a : GForgeLevelCheckAddrs)
			{
				a += 0x3;
				LOGI_STREAM("Tab3Life") << "[SDK] ForgeLevelCheck found at: 0x" << std::hex << a
					<< ", offset: 0x" << (a - ModBase) << std::dec << "\n";
			}
		}
	}
	const unsigned char p1[] = { 0x90, 0xE9 };
	for (auto addr : GForgeLevelCheckAddrs)
		InlineHook::HookManager::WriteMemory(addr, p1, sizeof(p1));

	// 2. ForgeMaterialCheck (Multi-match)
	if (GForgeMaterialCheckAddrs.empty())
	{
		GForgeMaterialCheckAddrs = InlineHook::HookManager::AobScanModule(
			"JH-Win64-Shipping.exe", kForgeMaterialCheckPattern, 256, true);
		if (GForgeMaterialCheckAddrs.empty())
			LOGE_STREAM("Tab3Life") << "[SDK] ForgeMaterialCheck AobScan failed\n";
		else
		{
			for (auto& a : GForgeMaterialCheckAddrs)
			{
				a += 0xB;
				LOGI_STREAM("Tab3Life") << "[SDK] ForgeMaterialCheck found at: 0x" << std::hex << a
					<< ", offset: 0x" << (a - ModBase) << std::dec << "\n";
			}
		}
	}
	const unsigned char p2[] = { 0x0C, 0x01 };
	for (auto addr : GForgeMaterialCheckAddrs)
		InlineHook::HookManager::WriteMemory(addr, p2, sizeof(p2));

	// 3. AlchemyMoneyCheck
	if (GAlchemyMoneyCheckAddr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kAlchemyMoneyCheckPattern);
		if (found)
		{
			GAlchemyMoneyCheckAddr = found + 0x3;
			LOGI_STREAM("Tab3Life") << "[SDK] AlchemyMoneyCheck found at: 0x" << std::hex << GAlchemyMoneyCheckAddr
				<< ", offset: 0x" << (GAlchemyMoneyCheckAddr - ModBase) << std::dec << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] AlchemyMoneyCheck AobScan failed\n";
	}
	const unsigned char p3[] = { 0x90, 0xE9 };
	if (GAlchemyMoneyCheckAddr) InlineHook::HookManager::WriteMemory(GAlchemyMoneyCheckAddr, p3, sizeof(p3));

	// 4. CookingMoneyCheck
	if (GCookingMoneyCheckAddr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kCookingMoneyCheckPattern);
		if (found)
		{
			GCookingMoneyCheckAddr = found + 0x3;
			LOGI_STREAM("Tab3Life") << "[SDK] CookingMoneyCheck found at: 0x" << std::hex << GCookingMoneyCheckAddr
				<< ", offset: 0x" << (GCookingMoneyCheckAddr - ModBase) << std::dec << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] CookingMoneyCheck AobScan failed\n";
	}
	const unsigned char p4[] = { 0xEB };
	if (GCookingMoneyCheckAddr) InlineHook::HookManager::WriteMemory(GCookingMoneyCheckAddr, p4, sizeof(p4));

	// 5. AlchemyLvCheck
	if (GAlchemyLvCheckAddr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kAlchemyLvCheckPattern);
		if (found)
		{
			GAlchemyLvCheckAddr = found + 0xB;
			LOGI_STREAM("Tab3Life") << "[SDK] AlchemyLvCheck found at: 0x" << std::hex << GAlchemyLvCheckAddr
				<< ", offset: 0x" << (GAlchemyLvCheckAddr - ModBase) << std::dec << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] AlchemyLvCheck AobScan failed\n";
	}
	const unsigned char p5[] = { 0x0C, 0x01 };
	if (GAlchemyLvCheckAddr) InlineHook::HookManager::WriteMemory(GAlchemyLvCheckAddr, p5, sizeof(p5));

	// 6. CookingLvCheck
	if (GCookingLvCheckAddr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kCookingLvCheckPattern);
		if (found)
		{
			GCookingLvCheckAddr = found + 0xB;
			LOGI_STREAM("Tab3Life") << "[SDK] CookingLvCheck found at: 0x" << std::hex << GCookingLvCheckAddr
				<< ", offset: 0x" << (GCookingLvCheckAddr - ModBase) << std::dec << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] CookingLvCheck AobScan failed\n";
	}
	const unsigned char p6[] = { 0x0C, 0x01 };
	if (GCookingLvCheckAddr) InlineHook::HookManager::WriteMemory(GCookingLvCheckAddr, p6, sizeof(p6));

	// ── 第二层补丁 ──

	// 7. IgnoreForgeLevel2 (执行层锻造等级验证)
	if (GIgnoreForgeLevel2Addr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kIgnoreForgeLevel2Pattern);
		if (found)
		{
			GIgnoreForgeLevel2Addr = found + 0x10;
			LOGI_STREAM("Tab3Life") << "[SDK] IgnoreForgeLevel2 found at: 0x" << std::hex << GIgnoreForgeLevel2Addr
				<< ", offset: 0x" << (GIgnoreForgeLevel2Addr - ModBase) << std::dec << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] IgnoreForgeLevel2 AobScan failed\n";
	}
	const unsigned char p7[] = { 0x0C, 0x01 };
	if (GIgnoreForgeLevel2Addr) InlineHook::HookManager::WriteMemory(GIgnoreForgeLevel2Addr, p7, sizeof(p7));

	// 8. ForgeMaterialEnough2 (执行层锻造材料验证)
	if (GForgeMaterialEnough2Addr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kForgeMaterialEnough2Pattern);
		if (found)
		{
			GForgeMaterialEnough2Addr = found + 0xC;
			LOGI_STREAM("Tab3Life") << "[SDK] ForgeMaterialEnough2 found at: 0x" << std::hex << GForgeMaterialEnough2Addr
				<< ", offset: 0x" << (GForgeMaterialEnough2Addr - ModBase) << std::dec << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] ForgeMaterialEnough2 AobScan failed\n";
	}
	const unsigned char p8[] = { 0x0C, 0x01 };
	if (GForgeMaterialEnough2Addr) InlineHook::HookManager::WriteMemory(GForgeMaterialEnough2Addr, p8, sizeof(p8));

	// 9. FusionType (融合类型绕过: 强制 edx=2)
	if (GFusionTypeAddr == 0)
	{
		uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kFusionTypePattern);
		if (found)
		{
			GFusionTypeAddr = found;
			LOGI_STREAM("Tab3Life") << "[SDK] FusionType found at: 0x" << std::hex << GFusionTypeAddr
				<< ", offset: 0x" << (GFusionTypeAddr - ModBase) << std::dec << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] FusionType AobScan failed\n";
	}
	if (GFusionTypeAddr && !GFusionTypeTrampoline)
	{
		// CT trampoline: mov edx,2; mov rdi,[rcx+08]; test edx,edx; jmp return
		// Original 6 bytes: 48 8B 79 08 85 D2
		memcpy(GFusionTypeOrigBytes, reinterpret_cast<void*>(GFusionTypeAddr), 6);

		// Allocate trampoline NEAR target (within ±2GB for JMP rel32)
		GFusionTypeTrampoline = nullptr;
		const uintptr_t allocGranularity = 0x10000; // 64KB
		for (uintptr_t delta = allocGranularity; delta < 0x7FFF0000ULL; delta += allocGranularity)
		{
			// Try below then above
			for (int dir = 0; dir < 2; ++dir)
			{
				uintptr_t tryAddr = dir == 0
					? (GFusionTypeAddr > delta ? GFusionTypeAddr - delta : 0)
					: GFusionTypeAddr + delta;
				if (!tryAddr) continue;
				void* p = VirtualAlloc(reinterpret_cast<void*>(tryAddr), 64,
					MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (p)
				{
					intptr_t dist = reinterpret_cast<intptr_t>(p) - static_cast<intptr_t>(GFusionTypeAddr + 5);
					if (dist >= INT32_MIN && dist <= INT32_MAX)
					{
						GFusionTypeTrampoline = p;
						break;
					}
					// Too far, release it (not a delete, just releasing unused alloc)
					VirtualFree(p, 0, MEM_RELEASE);
				}
			}
			if (GFusionTypeTrampoline) break;
		}
		if (GFusionTypeTrampoline)
		{
			unsigned char tramp[] = {
				0xBA, 0x02, 0x00, 0x00, 0x00,       // mov edx, 2
				0x48, 0x8B, 0x79, 0x08,              // mov rdi, [rcx+08]
				0x85, 0xD2,                           // test edx, edx
				0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,  // jmp [rip+0]
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // return address (8 bytes)
			};
			uintptr_t retAddr = GFusionTypeAddr + 6;
			memcpy(&tramp[17], &retAddr, 8);
			memcpy(GFusionTypeTrampoline, tramp, sizeof(tramp));

			// Write JMP from original to trampoline (5-byte JMP + 1 NOP)
			unsigned char jmpPatch[6];
			jmpPatch[0] = 0xE9;
			int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(GFusionTypeTrampoline) - (GFusionTypeAddr + 5));
			memcpy(&jmpPatch[1], &rel, 4);
			jmpPatch[5] = 0x90;
			InlineHook::HookManager::WriteMemory(GFusionTypeAddr, jmpPatch, 6);

			LOGI_STREAM("Tab3Life") << "[SDK] FusionType hook installed, trampoline at: " << GFusionTypeTrampoline << "\n";
		}
		else LOGE_STREAM("Tab3Life") << "[SDK] FusionType VirtualAlloc failed\n";
	}
	else if (GFusionTypeAddr && GFusionTypeTrampoline)
	{
		// Re-enable: reuse existing trampoline, just re-patch the JMP
		unsigned char jmpPatch[6];
		jmpPatch[0] = 0xE9;
		int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(GFusionTypeTrampoline) - (GFusionTypeAddr + 5));
		memcpy(&jmpPatch[1], &rel, 4);
		jmpPatch[5] = 0x90;
		InlineHook::HookManager::WriteMemory(GFusionTypeAddr, jmpPatch, 6);
		LOGI_STREAM("Tab3Life") << "[SDK] FusionType re-enabled, trampoline at: " << GFusionTypeTrampoline << "\n";
	}

	// 10. FusionAlertCont (产出数量替换: [rcx+304] → 自定义值)
	EnsureFusionAlertContHookInstalled();
	SyncCraftOutputQuantityFlag();

	// ── 补丁地址汇总日志 ──
	LOGI_STREAM("Tab3Life") << "[SDK] === Craft Patch Summary ===\n";
	for (size_t i = 0; i < GForgeLevelCheckAddrs.size(); ++i)
		LOGI_STREAM("Tab3Life") << "[SDK]   ForgeLvCheck[" << i << "]: 0x" << std::hex << GForgeLevelCheckAddrs[i] << std::dec << "\n";
	for (size_t i = 0; i < GForgeMaterialCheckAddrs.size(); ++i)
		LOGI_STREAM("Tab3Life") << "[SDK]   ForgeMaterialCheck[" << i << "]: 0x" << std::hex << GForgeMaterialCheckAddrs[i] << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   AlchemyMoney: 0x" << std::hex << GAlchemyMoneyCheckAddr << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   CookingMoney: 0x" << std::hex << GCookingMoneyCheckAddr << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   AlchemyLv: 0x" << std::hex << GAlchemyLvCheckAddr << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   CookingLv: 0x" << std::hex << GCookingLvCheckAddr << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   IgnoreForgeLevel2: 0x" << std::hex << GIgnoreForgeLevel2Addr << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   ForgeMaterialEnough2: 0x" << std::hex << GForgeMaterialEnough2Addr << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   FusionType: 0x" << std::hex << GFusionTypeAddr << " tramp=" << GFusionTypeTrampoline << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK]   FusionAlertCont: 0x" << std::hex << GFusionAlertContAddr
		<< " tramp=" << GFusionAlertContTrampoline
		<< " installed=" << (GFusionAlertContHookEnabled ? 1 : 0) << std::dec << "\n";
	LOGI_STREAM("Tab3Life") << "[SDK] CraftIgnoreRequirements enabled (with layer-2 patches)\n";
}

void DisableCraftIgnoreRequirements()
{
	GCraftIgnoreRequirementsFeatureEnabled = false;

	const unsigned char o1[] = { 0x0F, 0x85 };
	for (auto addr : GForgeLevelCheckAddrs)
		InlineHook::HookManager::WriteMemory(addr, o1, sizeof(o1));

	const unsigned char o2[] = { 0x84, 0xC0 };
	for (auto addr : GForgeMaterialCheckAddrs)
		InlineHook::HookManager::WriteMemory(addr, o2, sizeof(o2));

	const unsigned char o3[] = { 0x0F, 0x8D };
	if (GAlchemyMoneyCheckAddr) InlineHook::HookManager::WriteMemory(GAlchemyMoneyCheckAddr, o3, sizeof(o3));

	const unsigned char o4[] = { 0x7D };
	if (GCookingMoneyCheckAddr) InlineHook::HookManager::WriteMemory(GCookingMoneyCheckAddr, o4, sizeof(o4));

	const unsigned char o5[] = { 0x84, 0xC0 };
	if (GAlchemyLvCheckAddr) InlineHook::HookManager::WriteMemory(GAlchemyLvCheckAddr, o5, sizeof(o5));

	const unsigned char o6[] = { 0x84, 0xC0 };
	if (GCookingLvCheckAddr) InlineHook::HookManager::WriteMemory(GCookingLvCheckAddr, o6, sizeof(o6));

	// ── 第二层恢复 ──
	const unsigned char o7[] = { 0x84, 0xC0 };
	if (GIgnoreForgeLevel2Addr) InlineHook::HookManager::WriteMemory(GIgnoreForgeLevel2Addr, o7, sizeof(o7));

	const unsigned char o8[] = { 0x84, 0xC0 };
	if (GForgeMaterialEnough2Addr) InlineHook::HookManager::WriteMemory(GForgeMaterialEnough2Addr, o8, sizeof(o8));

	if (GFusionTypeAddr && GFusionTypeTrampoline)
	{
		InlineHook::HookManager::WriteMemory(GFusionTypeAddr, GFusionTypeOrigBytes, 6);
		// Keep GFusionTypeTrampoline for re-enable reuse
	}

	SyncCraftOutputQuantityFlag();
	DisableFusionAlertContHookIfUnused();

	LOGI_STREAM("Tab3Life") << "[SDK] CraftIgnoreRequirements disabled (with layer-2 patches)\n";
}
void SetCraftOutputQuantity(int32 Value)
{
	if (Value < 1) Value = 1;
	const LONG oldValue = InterlockedExchange(&GCraftOutputQtyVal, static_cast<LONG>(Value));
	if (oldValue != Value)
	{
		LOGI_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity set to: " << Value << "\n";
	}
}

void EnableCraftOutputQuantityHook()
{
	if (GCraftOutputQuantityFeatureEnabled)
	{
		SyncCraftOutputQuantityFlag();
		return;
	}

	if (!EnsureFusionAlertContHookInstalled())
	{
		LOGE_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity hook failed\n";
		return;
	}

	GCraftOutputQuantityFeatureEnabled = true;
	SyncCraftOutputQuantityFlag();
	LOGI_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity hook enabled\n";
}

void DisableCraftOutputQuantityHook()
{
	GCraftOutputQuantityFeatureEnabled = false;
	SyncCraftOutputQuantityFlag();
	DisableFusionAlertContHookIfUnused();
	LOGI_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity hook disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  采集一秒冷却
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{
uintptr_t GGatherCooldownAddr = 0;
const char* kGatherCooldownPattern = "0F 2F ?? ?? 72 ?? FF 15";
}

void EnableGatherCooldownPatch()
{
	if (GGatherCooldownAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kGatherCooldownPattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] GatherCooldown AobScan failed\n";
			return;
		}
		GGatherCooldownAddr = found;
		LOGI_STREAM("Tab3Life") << "[SDK] GatherCooldown found at: 0x" << std::hex << found << std::dec << "\n";
	}
	const unsigned char enable[] = { 0xEB, 0x04 };
	InlineHook::HookManager::WriteMemory(GGatherCooldownAddr, enable, sizeof(enable));
	LOGI_STREAM("Tab3Life") << "[SDK] GatherCooldown enabled\n";
}

void DisableGatherCooldownPatch()
{
	if (GGatherCooldownAddr == 0) return;
	const unsigned char disable[] = { 0x0F, 0x2F };
	InlineHook::HookManager::WriteMemory(GGatherCooldownAddr, disable, sizeof(disable));
	LOGI_STREAM("Tab3Life") << "[SDK] GatherCooldown disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  家园随时收获
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{
uintptr_t GHomelandHarvestAddr = 0;
const char* kHomelandHarvestPattern = "41 8B 4F ?? 41 3B 4F ?? 74";
}

void EnableHomelandHarvestPatch()
{
	if (GHomelandHarvestAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kHomelandHarvestPattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] HomelandHarvest AobScan failed\n";
			return;
		}
		GHomelandHarvestAddr = found + 0x8;
		LOGI_STREAM("Tab3Life") << "[SDK] HomelandHarvest found, patch at: 0x" << std::hex << GHomelandHarvestAddr << std::dec << "\n";
	}
	const unsigned char enable[] = { 0xEB };
	InlineHook::HookManager::WriteMemory(GHomelandHarvestAddr, enable, sizeof(enable));
	LOGI_STREAM("Tab3Life") << "[SDK] HomelandHarvest enabled\n";
}

void DisableHomelandHarvestPatch()
{
	if (GHomelandHarvestAddr == 0) return;
	const unsigned char disable[] = { 0x74 };
	InlineHook::HookManager::WriteMemory(GHomelandHarvestAddr, disable, sizeof(disable));
	LOGI_STREAM("Tab3Life") << "[SDK] HomelandHarvest disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  钓鱼只钓稀有物
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{
uint32_t GFishRareOnlyHookId = UINT32_MAX;
uintptr_t GFishRareOnlyOffset = 0;
const char* kFishRareOnlyPattern = "F3 0F 59 0D ?? ?? ?? ?? F3 0F 2C ?? 3B ?? 0F 4E ?? 33 DB";

// cmp rbp, rsp; jna skip; cmp [rbp+28], rsp; jna skip; cmp qword [rbp+30], 0; jna skip;
// mov eax, 0x7FFFFFFF; mov ecx, 0x7FFFFFFF; skip:
const unsigned char kFishRareOnlyTrampolineCode[] = {
	0x48, 0x39, 0xE5,                               // cmp rbp, rsp
	0x76, 0x17,                                       // jna +23 -> end of trampoline (offset 28)
	0x48, 0x39, 0x65, 0x28,                           // cmp [rbp+28], rsp
	0x76, 0x11,                                       // jna +17 -> end of trampoline (offset 28)
	0x48, 0x83, 0x7D, 0x30, 0x00,                     // cmp qword [rbp+30], 0
	0x76, 0x0A,                                       // jna +10 -> end of trampoline (offset 28)
	0xB8, 0xFF, 0xFF, 0xFF, 0x7F,                     // mov eax, 0x7FFFFFFF
	0xB9, 0xFF, 0xFF, 0xFF, 0x7F,                     // mov ecx, 0x7FFFFFFF
};
}

void EnableFishRareOnlyHook()
{
	if (GFishRareOnlyHookId != UINT32_MAX) return;

	if (GFishRareOnlyOffset == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kFishRareOnlyPattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] FishRareOnly AobScan failed\n";
			return;
		}
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (!hModule) return;
		GFishRareOnlyOffset = (found + 0xC) - reinterpret_cast<uintptr_t>(hModule);
		LOGI_STREAM("Tab3Life") << "[SDK] FishRareOnly found, hook offset: 0x" << std::hex << GFishRareOnlyOffset << std::dec << "\n";
	}

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GFishRareOnlyOffset),
		kFishRareOnlyTrampolineCode,
		sizeof(kFishRareOnlyTrampolineCode),
		hookId,
		false))
	{
		LOGE_STREAM("Tab3Life") << "[SDK] FishRareOnly hook install failed\n";
		return;
	}
	GFishRareOnlyHookId = hookId;
	LOGI_STREAM("Tab3Life") << "[SDK] FishRareOnly hook enabled, ID: " << hookId << "\n";
}

void DisableFishRareOnlyHook()
{
	if (GFishRareOnlyHookId == UINT32_MAX) return;
	InlineHook::HookManager::UninstallHook(GFishRareOnlyHookId);
	GFishRareOnlyHookId = UINT32_MAX;
	LOGI_STREAM("Tab3Life") << "[SDK] FishRareOnly hook disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  钓鱼收杆必有收获
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{
uint32_t GFishAlwaysCatchHookId1 = UINT32_MAX;
uint32_t GFishAlwaysCatchHookId2 = UINT32_MAX;
uintptr_t GFishAlwaysCatchOffset1 = 0;
uintptr_t GFishAlwaysCatchOffset2 = 0;
volatile LONG GFishAlwaysCatchFlag = 0;

const char* kFishAlwaysCatchPattern1 = "48 8B 89 ?? ?? 00 00 48 8B 01 B2 03";
const char* kFishAlwaysCatchPattern2 = "48 8B FA ?? 63 30";

// Hook1: set flag to 1 when reel is triggered
// mov [r11], 1 (r11 = &GFishAlwaysCatchFlag)
const unsigned char kFishAlwaysCatchHook1Template[] = {
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&flag)
	0x41, 0xC7, 0x03, 0x01, 0x00, 0x00, 0x00,         // mov dword [r11], 1
	0x41, 0x5B,                                       // pop r11
};
constexpr size_t kFishCatch1FlagImm64Offset = 4;

// Hook2: if flag==1, set [rcx+0x279]=1 (catch success), then reset flag
const unsigned char kFishAlwaysCatchHook2Template[] = {
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&flag)
	0x41, 0x83, 0x3B, 0x01,                           // cmp dword [r11], 1
	0x75, 0x13,                                       // jne +19 (skip)
	0x41, 0xC7, 0x03, 0x00, 0x00, 0x00, 0x00,         // mov dword [r11], 0
	0x48, 0x39, 0xE1,                                 // cmp rcx, rsp
	0x76, 0x07,                                       // jna +7 (skip)
	0xC6, 0x81, 0x79, 0x02, 0x00, 0x00, 0x01,         // mov byte [rcx+0x279], 1
	0x41, 0x5B,                                       // pop r11
};
constexpr size_t kFishCatch2FlagImm64Offset = 4;
}

void EnableFishAlwaysCatchHook()
{
	if (GFishAlwaysCatchHookId1 != UINT32_MAX && GFishAlwaysCatchHookId2 != UINT32_MAX)
		return;

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule) return;
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);

	if (GFishAlwaysCatchOffset1 == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kFishAlwaysCatchPattern1);
		if (found == 0)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch pattern1 AobScan failed\n";
			return;
		}
		GFishAlwaysCatchOffset1 = found - moduleBase;
		LOGI_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch hook1 offset: 0x" << std::hex << GFishAlwaysCatchOffset1 << std::dec << "\n";
	}

	if (GFishAlwaysCatchOffset2 == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kFishAlwaysCatchPattern2);
		if (found == 0)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch pattern2 AobScan failed\n";
			return;
		}
		GFishAlwaysCatchOffset2 = found - moduleBase;
		LOGI_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch hook2 offset: 0x" << std::hex << GFishAlwaysCatchOffset2 << std::dec << "\n";
	}

	InterlockedExchange(&GFishAlwaysCatchFlag, 0);
	const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GFishAlwaysCatchFlag);

	if (GFishAlwaysCatchHookId1 == UINT32_MAX)
	{
		unsigned char code1[sizeof(kFishAlwaysCatchHook1Template)];
		std::memcpy(code1, kFishAlwaysCatchHook1Template, sizeof(code1));
		std::memcpy(code1 + kFishCatch1FlagImm64Offset, &flagAddr, sizeof(flagAddr));

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GFishAlwaysCatchOffset1),
			code1, sizeof(code1), hookId))
		{
			LOGE_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch hook1 install failed\n";
			return;
		}
		GFishAlwaysCatchHookId1 = hookId;
	}

	if (GFishAlwaysCatchHookId2 == UINT32_MAX)
	{
		unsigned char code2[sizeof(kFishAlwaysCatchHook2Template)];
		std::memcpy(code2, kFishAlwaysCatchHook2Template, sizeof(code2));
		std::memcpy(code2 + kFishCatch2FlagImm64Offset, &flagAddr, sizeof(flagAddr));

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GFishAlwaysCatchOffset2),
			code2, sizeof(code2), hookId,
			true))
		{
			InlineHook::HookManager::UninstallHook(GFishAlwaysCatchHookId1);
			GFishAlwaysCatchHookId1 = UINT32_MAX;
			LOGE_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch hook2 install failed\n";
			return;
		}
		GFishAlwaysCatchHookId2 = hookId;
	}

	LOGI_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch hooks enabled, IDs: "
		<< GFishAlwaysCatchHookId1 << ", " << GFishAlwaysCatchHookId2 << "\n";
}

void DisableFishAlwaysCatchHook()
{
	if (GFishAlwaysCatchHookId2 != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GFishAlwaysCatchHookId2);
		GFishAlwaysCatchHookId2 = UINT32_MAX;
	}
	if (GFishAlwaysCatchHookId1 != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GFishAlwaysCatchHookId1);
		GFishAlwaysCatchHookId1 = UINT32_MAX;
	}
	InterlockedExchange(&GFishAlwaysCatchFlag, 0);
	LOGI_STREAM("Tab3Life") << "[SDK] FishAlwaysCatch hooks disabled\n";
}
