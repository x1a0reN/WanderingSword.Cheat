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

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			RegisterTab0Binding(Title, Item, PC);
			Count++;
		}
	};

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"生活开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	GTab3.CraftIgnoreRequirementsToggle = CreateToggleItem(PC, L"锻造/制衣/炼丹/烹饪无视要求");
	if (GTab3.CraftIgnoreRequirementsToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.CraftIgnoreRequirementsToggle); else Container->AddChild(GTab3.CraftIgnoreRequirementsToggle); Count++; }
	GTab3.CraftOutputQuantityToggle = CreateToggleItem(PC, L"设置产出数量");
	if (GTab3.CraftOutputQuantityToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.CraftOutputQuantityToggle); else Container->AddChild(GTab3.CraftOutputQuantityToggle); Count++; }

	GTab3.GatherCooldownToggle = CreateToggleItem(PC, L"采集一秒冷却");
	if (GTab3.GatherCooldownToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.GatherCooldownToggle); else Container->AddChild(GTab3.GatherCooldownToggle); Count++; }
	GTab3.FishRareOnlyToggle = CreateToggleItem(PC, L"钓鱼只钓稀有物");
	if (GTab3.FishRareOnlyToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.FishRareOnlyToggle); else Container->AddChild(GTab3.FishRareOnlyToggle); Count++; }
	GTab3.FishAlwaysCatchToggle = CreateToggleItem(PC, L"钓鱼收杆必有收获");
	if (GTab3.FishAlwaysCatchToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.FishAlwaysCatchToggle); else Container->AddChild(GTab3.FishAlwaysCatchToggle); Count++; }
	GTab3.HomelandHarvestToggle = CreateToggleItem(PC, L"家园随时收获");
	if (GTab3.HomelandHarvestToggle) { if (SwitchBox) SwitchBox->AddChild(GTab3.HomelandHarvestToggle); else Container->AddChild(GTab3.HomelandHarvestToggle); Count++; }
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);

	auto* OutputPanel = CreateCollapsiblePanel(PC, L"产出与掉落");
	auto* OutputBox = OutputPanel ? OutputPanel->CT_Contents : nullptr;
	GTab3.CraftOutputQuantityEdit = CreateVolumeNumericEditBoxItem(PC, Outer, OutputBox ? OutputBox : Container, L"产出数量", L"输入数字", L"1");
	if (GTab3.CraftOutputQuantityEdit) { if (OutputBox) OutputBox->AddChild(GTab3.CraftOutputQuantityEdit); else Container->AddChild(GTab3.CraftOutputQuantityEdit); Count++; }
	AddPanelWithFixedGap(OutputPanel, 0.0f, 10.0f);

	auto* MasteryPanel = CreateCollapsiblePanel(PC, L"生活精通");
	auto* MasteryBox = MasteryPanel ? MasteryPanel->CT_Contents : nullptr;
	AddNumeric(MasteryBox, L"锻造精通", L"100");
	AddNumeric(MasteryBox, L"医术精通", L"100");
	AddNumeric(MasteryBox, L"制衣精通", L"100");
	AddNumeric(MasteryBox, L"炼丹精通", L"100");
	AddNumeric(MasteryBox, L"烹饪精通", L"100");
	AddNumeric(MasteryBox, L"采集精通", L"100");
	AddNumeric(MasteryBox, L"钓鱼精通", L"100");
	AddNumeric(MasteryBox, L"饮酒精通", L"100");
	AddNumeric(MasteryBox, L"茶道精通", L"100");
	AddNumeric(MasteryBox, L"口才精通", L"100");
	AddNumeric(MasteryBox, L"书法精通", L"100");
	AddPanelWithFixedGap(MasteryPanel, 0.0f, 10.0f);

	auto* ExpPanel = CreateCollapsiblePanel(PC, L"生活经验");
	auto* ExpBox = ExpPanel ? ExpPanel->CT_Contents : nullptr;
	AddNumeric(ExpBox, L"锻造经验", L"100");
	AddNumeric(ExpBox, L"医术经验", L"100");
	AddNumeric(ExpBox, L"制衣经验", L"100");
	AddNumeric(ExpBox, L"炼丹经验", L"100");
	AddNumeric(ExpBox, L"烹饪经验", L"100");
	AddNumeric(ExpBox, L"采集经验", L"100");
	AddNumeric(ExpBox, L"钓鱼经验", L"100");
	AddNumeric(ExpBox, L"饮酒经验", L"100");
	AddNumeric(ExpBox, L"茶道经验", L"100");
	AddNumeric(ExpBox, L"口才经验", L"100");
	AddNumeric(ExpBox, L"书法经验", L"100");
	AddPanelWithFixedGap(ExpPanel, 0.0f, 8.0f);
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

uint32_t GTab3CraftOutputQtyHookId = UINT32_MAX;
uintptr_t GTab3FusionAlertContOffset = 0;

volatile LONG GCraftOutputQtyFlag = 0;
volatile LONG GCraftOutputQtyVal = 1;

const char* kForgeLevelCheckPattern = "83 ?? FE 0F 85 ?? ?? ?? ?? 4C 8D 0D";
const char* kForgeMaterialCheckPattern = "?? 8B ?? 49 8B ?? E8 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? 00 00 4C 8D 0D";
const char* kAlchemyMoneyCheckPattern = "39 ?? 30 0F 8D ?? ?? 00 00 4C 8D";
const char* kCookingMoneyCheckPattern = "39 ?? 30 7D ?? 4C 8D";
const char* kAlchemyLvCheckPattern = "?? 8B ?? 49 8B ?? E8 ?? ?? ?? ?? 84 C0";
const char* kCookingLvCheckPattern = "8B ?? ?? 49 8B ?? E8 ?? ?? ?? ?? 84 C0 75 ?? 4C 8D";

const char* kFusionAlertContPattern = "8B 81 04 03 00 00 89 44 24";

const unsigned char kFusionAlertContTrampolineCode[] = {
0x8B, 0x81, 0x04, 0x03, 0x00, 0x00, 
0x41, 0x53,
0x49, 0xBB,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00,
0x41, 0x83, 0x3B, 0x01,
0x75, 0x14,
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
constexpr size_t kFusionAlertValImm64Offset = 25;
}

void EnableCraftIgnoreRequirements();
void DisableCraftIgnoreRequirements();
void SetCraftOutputQuantity(int32 Value);
void EnableCraftOutputQuantityHook();
void DisableCraftOutputQuantityHook();

void EnableCraftIgnoreRequirements()
{
	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule)
	{
		LOGE_STREAM("Tab3Life") << "[SDK] CraftIgnoreRequirements failed to get module handle\n";
		return;
	}
	const uintptr_t ModBase = reinterpret_cast<uintptr_t>(hModule);

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

	LOGI_STREAM("Tab3Life") << "[SDK] CraftIgnoreRequirements enabled\n";
}

void DisableCraftIgnoreRequirements()
{
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

	LOGI_STREAM("Tab3Life") << "[SDK] CraftIgnoreRequirements disabled\n";
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
	if (GTab3CraftOutputQtyHookId != UINT32_MAX)
		return;

	if (GTab3FusionAlertContOffset == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kFusionAlertContPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity AobScan failed\n";
			return;
		}

		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (!hModule)
		{
			LOGE_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity failed to get module handle\n";
			return;
		}

		GTab3FusionAlertContOffset = foundAddr - reinterpret_cast<uintptr_t>(hModule);
		LOGI_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity found at: 0x" << std::hex << foundAddr
			<< ", offset: 0x" << GTab3FusionAlertContOffset << std::dec << "\n";
	}

	unsigned char code[sizeof(kFusionAlertContTrampolineCode)] = {};
	std::memcpy(code, kFusionAlertContTrampolineCode, sizeof(code));

	const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GCraftOutputQtyFlag);
	const uintptr_t valAddr = reinterpret_cast<uintptr_t>(&GCraftOutputQtyVal);

	std::memcpy(code + kFusionAlertFlagImm64Offset, &flagAddr, sizeof(flagAddr));
	std::memcpy(code + kFusionAlertValImm64Offset, &valAddr, sizeof(valAddr));

	uint32_t hookId = UINT32_MAX;
	const bool success = InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GTab3FusionAlertContOffset),
		code,
		sizeof(code),
		hookId,
		false,
		false
	);

	if (success && hookId != UINT32_MAX)
	{
		GTab3CraftOutputQtyHookId = hookId;
		InterlockedExchange(&GCraftOutputQtyFlag, 1);
		LOGI_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity hook enabled, ID: " << hookId << "\n";
	}
	else
	{
		LOGE_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity hook failed\n";
	}
}

void DisableCraftOutputQuantityHook()
{
	if (GTab3CraftOutputQtyHookId == UINT32_MAX)
		return;

	const bool success = InlineHook::HookManager::UninstallHook(GTab3CraftOutputQtyHookId);
	if (success)
	{
		LOGI_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity hook disabled\n";
	}
	else
	{
		LOGE_STREAM("Tab3Life") << "[SDK] CraftOutputQuantity hook disable failed\n";
	}

	GTab3CraftOutputQtyHookId = UINT32_MAX;
	InterlockedExchange(&GCraftOutputQtyFlag, 0);
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
	0x76, 0x1B,                                       // jna +27 (skip)
	0x48, 0x39, 0x65, 0x28,                           // cmp [rbp+28], rsp
	0x76, 0x15,                                       // jna +21 (skip)
	0x48, 0x83, 0x7D, 0x30, 0x00,                     // cmp qword [rbp+30], 0
	0x76, 0x0E,                                       // jna +14 (skip)
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
		hookId))
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
			code2, sizeof(code2), hookId))
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