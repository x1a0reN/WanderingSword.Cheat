void PopulateTab_System(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->OthersSlot, "Tab5(OthersSlot)");
	if (!Container) return;
	Container->ClearChildren();
	GTab5 = {};
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

	auto AddToggleStored = [&](UPanelWidget* Box, const wchar_t* Title, UBPVE_JHConfigVideoItem2_C*& OutRef) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			OutRef = Item;
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddSliderStored = [&](UPanelWidget* Box, const wchar_t* Title, UBPVE_JHConfigVolumeItem2_C*& OutRef) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			OutRef = Item;
			if (Item->VolumeSlider)
			{
				Item->VolumeSlider->MinValue = 0.0f;
				Item->VolumeSlider->MaxValue = 10.0f;
				Item->VolumeSlider->StepSize = 0.1f;
			}
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddDropdownStored = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options, UBPVE_JHConfigVideoItem2_C*& OutRef) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			OutRef = Item;
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
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
			Count++;
		}
	};

	auto* MovePanel = CreateCollapsiblePanel(PC, L"移动与跳跃");
	auto* MoveBox = MovePanel ? MovePanel->CT_Contents : nullptr;
	AddToggle(MoveBox, L"空格跳跃");
	AddSlider(MoveBox, L"跳跃速度");
	AddToggleStored(MoveBox, L"无限跳跃", GTab5.InfiniteJumpToggle);
	AddToggleStored(MoveBox, L"奔跑/骑马加速", GTab5.RunMountSpeedToggle);
	AddSliderStored(MoveBox, L"加速倍率", GTab5.RunMountSpeedSlider);
	AddSlider(MoveBox, L"世界移动速度");
	AddSlider(MoveBox, L"场景移动速度");
	AddPanelWithFixedGap(MovePanel, 0.0f, 10.0f);

	auto* MountPanel = CreateCollapsiblePanel(PC, L"坐骑设置");
	auto* MountBox = MountPanel ? MountPanel->CT_Contents : nullptr;
	AddToggleStored(MountBox, L"坐骑替换", GTab5.MountReplaceToggle);
	AddDropdownStored(MountBox, L"指定坐骑", { L"黑马", L"白马", L"棕马", L"小毛驴" }, GTab5.MountSelectDD);
	AddPanelWithFixedGap(MountPanel, 0.0f, 10.0f);

	auto* StoryPanel = CreateCollapsiblePanel(PC, L"开档与解锁");
	auto* StoryBox = StoryPanel ? StoryPanel->CT_Contents : nullptr;
	AddToggleStored(StoryBox, L"一周目可选极难", GTab5.FirstPlayHardToggle);
	AddToggleStored(StoryBox, L"一周目可选传承", GTab5.FirstPlayInheritToggle);
	AddToggle(StoryBox, L"承君传承包括所有");
	AddToggleStored(StoryBox, L"未交互驿站可用", GTab5.PostStationToggle);
	AddToggle(StoryBox, L"激活GM命令行");
	AddToggle(StoryBox, L"解锁全图鉴");
	AddToggle(StoryBox, L"解锁全成就");
	AddPanelWithFixedGap(StoryPanel, 0.0f, 10.0f);

	auto* ScreenPanel = CreateCollapsiblePanel(PC, L"屏幕设置");
	auto* ScreenBox = ScreenPanel ? ScreenPanel->CT_Contents : nullptr;
	AddDropdown(ScreenBox, L"分辨率", { L"1920x1080", L"2560x1440", L"3840x2160" });
	AddDropdown(ScreenBox, L"窗口模式", { L"全屏", L"无边框", L"窗口" });
	AddToggle(ScreenBox, L"垂直同步");
	AddPanelWithFixedGap(ScreenPanel, 0.0f, 10.0f);

	auto* DiffPanel = CreateCollapsiblePanel(PC, L"开档难度系数");
	auto* DiffBox = DiffPanel ? DiffPanel->CT_Contents : nullptr;
	AddNumeric(DiffBox, L"简单系数", L"100");
	AddNumeric(DiffBox, L"普通系数", L"100");
	AddNumeric(DiffBox, L"困难系数", L"100");
	AddNumeric(DiffBox, L"极难系数", L"100");
	AddNumeric(DiffBox, L"敌人伤害系数", L"100");
	AddNumeric(DiffBox, L"敌人气血系数", L"100");
	AddNumeric(DiffBox, L"资源产出系数", L"100");
	AddNumeric(DiffBox, L"经验获取系数", L"100");
	AddPanelWithFixedGap(DiffPanel, 0.0f, 10.0f);

	auto* TitlePanel = CreateCollapsiblePanel(PC, L"称号战力门槛");
	auto* TitleBox = TitlePanel ? TitlePanel->CT_Contents : nullptr;
	AddNumeric(TitleBox, L"称号门槛1", L"100");
	AddNumeric(TitleBox, L"称号门槛2", L"200");
	AddNumeric(TitleBox, L"称号门槛3", L"300");
	AddNumeric(TitleBox, L"称号门槛4", L"400");
	AddNumeric(TitleBox, L"称号门槛5", L"500");
	AddNumeric(TitleBox, L"称号门槛6", L"600");
	AddNumeric(TitleBox, L"称号门槛7", L"700");
	AddNumeric(TitleBox, L"称号门槛8", L"800");
	AddNumeric(TitleBox, L"称号门槛9", L"900");
	AddNumeric(TitleBox, L"称号门槛10", L"1000");
	AddNumeric(TitleBox, L"称号门槛11", L"1100");
	AddPanelWithFixedGap(TitlePanel, 0.0f, 8.0f);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  Tab5 Enable/Disable implementations
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{

// ── 无限跳跃 ──
uintptr_t GInfiniteJumpAddr = 0;
const char* kInfiniteJumpPattern = "8B 87 ?? ?? 00 00 39 87 ?? ?? 00 00 0F 9C C0";

// ── 奔跑/骑马加速 ──
uint32_t GRunMountSpeedHookId1 = UINT32_MAX;
uint32_t GRunMountSpeedHookId2 = UINT32_MAX;
uintptr_t GRunMountSpeedOffset1 = 0;
uintptr_t GRunMountSpeedOffset2 = 0;
volatile float GRunMountSpeedMultiplier = 2.0f;
const char* kRunMountSpeedPattern = "F3 0F 58 ?? F3 0F 58 ?? ?? ?? 00 00 EB";

// CT pattern: hook replaces `addss xmm6, [rsi+offset]` with:
//   movss xmm0, [rsi+offset]  (load speed value)
//   mulss xmm0, [multiplier]   (multiply)
//   addss xmm6, xmm0           (add multiplied value)
// The offset in [rsi+xx] is read from the original instruction at runtime.
const unsigned char kRunMountSpeedTrampolineTemplate[] = {
	0xF3, 0x0F, 0x10, 0x86,                           // movss xmm0, [rsi+imm32] (4 bytes opcode, imm32 filled at runtime)
	0x00, 0x00, 0x00, 0x00,                            // imm32 placeholder for [rsi+offset]
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&multiplier)
	0xF3, 0x41, 0x0F, 0x59, 0x03,                     // mulss xmm0, dword [r11]
	0x41, 0x5B,                                       // pop r11
	0xF3, 0x0F, 0x58, 0xF0,                           // addss xmm6, xmm0
};
constexpr size_t kRunMountSpeedSrcOffImm32Offset = 4;
constexpr size_t kRunMountSpeedMulImm64Offset = 12;

// ── 坐骑替换 ──
uint32_t GMountReplaceHookId = UINT32_MAX;
uintptr_t GMountReplaceOffset = 0;
volatile LONG GMountReplaceId = 9;
const char* kMountReplacePattern = "83 B9 ?? ?? 00 00 00 7E ?? 48 89 ?? 24 ?? E8";

// CT pattern: write new mount, then re-execute original cmp for correct flags.
// Original instruction: 83 B9 [disp32] 00 = cmp dword [rcx+disp32], 0 (7 bytes)
// Trampoline: push/load mount id → mov [rcx+disp32],eax → pop → cmp [rcx+disp32],0
const unsigned char kMountReplaceTrampolineTemplate[] = {
	0x50,                                             // push rax          offset 0
	0x41, 0x53,                                       // push r11          offset 1
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&GMountReplaceId)  offset 3
	0x41, 0x8B, 0x03,                                 // mov eax, dword [r11]               offset 13
	0x89, 0x81,
	0x00, 0x00, 0x00, 0x00,                           // mov [rcx+disp32], eax              offset 16
	0x41, 0x5B,                                       // pop r11                            offset 22
	0x58,                                             // pop rax                            offset 24
	0x83, 0xB9,
	0x00, 0x00, 0x00, 0x00,                           // cmp dword [rcx+disp32], 0          offset 25
	0x00,                                             //   imm8 = 0                         offset 31
};
constexpr size_t kMountReplaceIdImm64Offset = 5;
constexpr size_t kMountReplaceMovDisp32Offset = 18;
constexpr size_t kMountReplaceCmpDisp32Offset = 27;

// ── 一周目可选极难 ──
uintptr_t GFirstPlayHardAddr = 0;
const char* kFirstPlayHardPattern = "48 8B C8 E8 ?? ?? ?? ?? 83 F8 64 7F";

// ── 一周目可选传承 ──
uintptr_t GFirstPlayInheritAddr = 0;
const char* kFirstPlayInheritPattern = "48 8D 55 D0 48 8B ?? FF 90 70 03 00 00 84 C0 0F 85 ?? ?? ?? ?? ?? ?? ?? 48 8D 45";

// ── 未交互驿站可用 ──
uintptr_t GPostStationAddr = 0;
const char* kPostStationPattern = "?? 8B ?? 48 8D 54 24 ?? ?? 8B ?? FF 90 ?? ?? 00 00 85 C0 0F 85";

} // end anonymous namespace

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  无限跳跃
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableInfiniteJumpPatch()
{
	if (GInfiniteJumpAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kInfiniteJumpPattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab5System") << "[SDK] InfiniteJump AobScan failed\n";
			return;
		}
		GInfiniteJumpAddr = found + 0xC;
	}
	const unsigned char enable[] = { 0x83, 0xC8, 0x01 };
	InlineHook::HookManager::WriteMemory(GInfiniteJumpAddr, enable, sizeof(enable));
	LOGI_STREAM("Tab5System") << "[SDK] InfiniteJump enabled\n";
}

void DisableInfiniteJumpPatch()
{
	if (GInfiniteJumpAddr == 0) return;
	const unsigned char disable[] = { 0x0F, 0x9C, 0xC0 };
	InlineHook::HookManager::WriteMemory(GInfiniteJumpAddr, disable, sizeof(disable));
	LOGI_STREAM("Tab5System") << "[SDK] InfiniteJump disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  奔跑/骑马加速
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void SetRunMountSpeedMultiplier(float Value)
{
	if (Value < 0.1f) Value = 0.1f;
	if (Value > 10.0f) Value = 10.0f;
	GRunMountSpeedMultiplier = Value;
}

void EnableRunMountSpeedHook()
{
	if (GRunMountSpeedHookId1 != UINT32_MAX) return;

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule) return;
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);

	auto results = InlineHook::HookManager::AobScanModule(
		"JH-Win64-Shipping.exe", kRunMountSpeedPattern, 2, true);
	if (results.empty())
	{
		LOGE_STREAM("Tab5System") << "[SDK] RunMountSpeed AobScan failed\n";
		return;
	}

	GRunMountSpeedOffset1 = (results[0] + 4) - moduleBase;
	if (results.size() >= 2)
		GRunMountSpeedOffset2 = (results[1] + 4) - moduleBase;

	const uintptr_t mulAddr = reinterpret_cast<uintptr_t>(&GRunMountSpeedMultiplier);

	auto InstallOne = [&](uintptr_t offset, uintptr_t patternAddr, uint32_t& outId) -> bool
	{
		unsigned char code[sizeof(kRunMountSpeedTrampolineTemplate)];
		std::memcpy(code, kRunMountSpeedTrampolineTemplate, sizeof(code));

		int32 srcOffset = 0;
		InlineHook::HookManager::ReadValue(patternAddr + 4 + 4, srcOffset);
		std::memcpy(code + kRunMountSpeedSrcOffImm32Offset, &srcOffset, sizeof(srcOffset));
		std::memcpy(code + kRunMountSpeedMulImm64Offset, &mulAddr, sizeof(mulAddr));

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(offset),
			code, sizeof(code), hookId,
			false, true, false))
			return false;
		outId = hookId;
		return true;
	};

	if (!InstallOne(GRunMountSpeedOffset1, results[0], GRunMountSpeedHookId1))
	{
		LOGE_STREAM("Tab5System") << "[SDK] RunMountSpeed hook1 install failed\n";
		return;
	}

	if (GRunMountSpeedOffset2 != 0 && results.size() >= 2)
	{
		if (!InstallOne(GRunMountSpeedOffset2, results[1], GRunMountSpeedHookId2))
			LOGE_STREAM("Tab5System") << "[SDK] RunMountSpeed hook2 install failed (non-fatal)\n";
	}

	LOGI_STREAM("Tab5System") << "[SDK] RunMountSpeed hooks enabled\n";
}

void DisableRunMountSpeedHook()
{
	if (GRunMountSpeedHookId2 != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GRunMountSpeedHookId2);
		GRunMountSpeedHookId2 = UINT32_MAX;
	}
	if (GRunMountSpeedHookId1 != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GRunMountSpeedHookId1);
		GRunMountSpeedHookId1 = UINT32_MAX;
	}
	LOGI_STREAM("Tab5System") << "[SDK] RunMountSpeed hooks disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  坐骑替换
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void SetMountReplaceId(int32 Value)
{
	InterlockedExchange(&GMountReplaceId, static_cast<LONG>(Value));
}

void EnableMountReplacePatch()
{
	if (GMountReplaceHookId != UINT32_MAX) return;

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule) return;
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);

	if (GMountReplaceOffset == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kMountReplacePattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab5System") << "[SDK] MountReplace AobScan failed\n";
			return;
		}
		GMountReplaceOffset = found - moduleBase;
	}

	const uintptr_t foundAddr = moduleBase + GMountReplaceOffset;
	int32 fieldDisp32 = 0;
	InlineHook::HookManager::ReadValue(foundAddr + 2, fieldDisp32);
	LOGI_STREAM("Tab5System") << "[SDK] MountReplace found, field disp32=0x"
		<< std::hex << fieldDisp32 << std::dec << "\n";

	unsigned char code[sizeof(kMountReplaceTrampolineTemplate)];
	std::memcpy(code, kMountReplaceTrampolineTemplate, sizeof(code));

	const uintptr_t idAddr = reinterpret_cast<uintptr_t>(&GMountReplaceId);
	std::memcpy(code + kMountReplaceIdImm64Offset, &idAddr, sizeof(idAddr));
	std::memcpy(code + kMountReplaceMovDisp32Offset, &fieldDisp32, sizeof(fieldDisp32));
	std::memcpy(code + kMountReplaceCmpDisp32Offset, &fieldDisp32, sizeof(fieldDisp32));

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GMountReplaceOffset),
		code, sizeof(code), hookId,
		false, true, false))
	{
		LOGE_STREAM("Tab5System") << "[SDK] MountReplace hook install failed\n";
		return;
	}
	GMountReplaceHookId = hookId;
	LOGI_STREAM("Tab5System") << "[SDK] MountReplace hook enabled, ID: " << hookId << "\n";
}

void DisableMountReplacePatch()
{
	if (GMountReplaceHookId == UINT32_MAX) return;
	InlineHook::HookManager::UninstallHook(GMountReplaceHookId);
	GMountReplaceHookId = UINT32_MAX;
	LOGI_STREAM("Tab5System") << "[SDK] MountReplace hook disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  一周目可选极难
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableFirstPlayHardPatch()
{
	if (GFirstPlayHardAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kFirstPlayHardPattern);
		if (found == 0) { LOGE_STREAM("Tab5System") << "[SDK] FirstPlayHard AobScan failed\n"; return; }
		GFirstPlayHardAddr = found + 0xB;
	}
	const unsigned char enable[] = { 0xEB };
	InlineHook::HookManager::WriteMemory(GFirstPlayHardAddr, enable, 1);
	LOGI_STREAM("Tab5System") << "[SDK] FirstPlayHard enabled\n";
}

void DisableFirstPlayHardPatch()
{
	if (GFirstPlayHardAddr == 0) return;
	const unsigned char disable[] = { 0x7F };
	InlineHook::HookManager::WriteMemory(GFirstPlayHardAddr, disable, 1);
	LOGI_STREAM("Tab5System") << "[SDK] FirstPlayHard disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  一周目可选传承
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableFirstPlayInheritPatch()
{
	if (GFirstPlayInheritAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kFirstPlayInheritPattern);
		if (found == 0) { LOGE_STREAM("Tab5System") << "[SDK] FirstPlayInherit AobScan failed\n"; return; }
		GFirstPlayInheritAddr = found + 0xD;
	}
	const unsigned char enable[] = { 0x24, 0x00 };
	InlineHook::HookManager::WriteMemory(GFirstPlayInheritAddr, enable, 2);
	LOGI_STREAM("Tab5System") << "[SDK] FirstPlayInherit enabled\n";
}

void DisableFirstPlayInheritPatch()
{
	if (GFirstPlayInheritAddr == 0) return;
	const unsigned char disable[] = { 0x84, 0xC0 };
	InlineHook::HookManager::WriteMemory(GFirstPlayInheritAddr, disable, 2);
	LOGI_STREAM("Tab5System") << "[SDK] FirstPlayInherit disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  未交互驿站可用
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnablePostStationPatch()
{
	if (GPostStationAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kPostStationPattern);
		if (found == 0) { LOGE_STREAM("Tab5System") << "[SDK] PostStation AobScan failed\n"; return; }
		GPostStationAddr = found + 0x13;
	}
	const unsigned char enable[] = { 0xEB, 0x04 };
	InlineHook::HookManager::WriteMemory(GPostStationAddr, enable, 2);
	LOGI_STREAM("Tab5System") << "[SDK] PostStation enabled\n";
}

void DisablePostStationPatch()
{
	if (GPostStationAddr == 0) return;
	const unsigned char disable[] = { 0x0F, 0x85 };
	InlineHook::HookManager::WriteMemory(GPostStationAddr, disable, 2);
	LOGI_STREAM("Tab5System") << "[SDK] PostStation disabled\n";
}
