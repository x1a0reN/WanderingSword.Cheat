// ── 坐骑下拉动态选项 ──
std::vector<int32> GTab5MountOptionIds;
std::vector<std::wstring> GTab5MountOptionLabels;

void ResolveMountOptionsFromTable()
{
	GTab5MountOptionIds.clear();
	GTab5MountOptionLabels.clear();

	UMountResManager* MountResMgr = UManagerFuncLib::GetMountResManager();
	UDataTable* MountTable = MountResMgr ? MountResMgr->MountResourceTable : nullptr;
	LOGI_STREAM("Tab5System") << "[SDK][Tab5] ResolveMountOptions mgr=" << (void*)MountResMgr
	          << " table=" << (void*)MountTable << "\n";

	if (!MountTable || !IsSafeLiveObject(static_cast<UObject*>(MountTable)))
		return;

	// FMountInfoSetting layout: ID at 0x08 (int32), Description at 0x80 (FText)
	std::vector<std::pair<int32, std::wstring>> Entries;

	auto& RowMap = MountTable->RowMap;
	const int32 AllocatedSlots = RowMap.IsValid() ? RowMap.NumAllocated() : 0;
	const int32 RowCount = RowMap.IsValid() ? RowMap.Num() : 0;

	if (RowMap.IsValid() && AllocatedSlots > 0 && RowCount > 0)
	{
		for (int32 i = 0; i < AllocatedSlots; ++i)
		{
			if (!RowMap.IsValidIndex(i))
				continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData)
				continue;

			const int32 MountId = *reinterpret_cast<int32*>(RowData + 0x08);
			std::wstring Label;

			// Read Description (FText at offset 0x80)
			auto* TextData = *reinterpret_cast<FTextImpl::FTextData**>(RowData + 0x80);
			if (TextData)
			{
				const wchar_t* WStr = TextData->TextSource.CStr();
				if (WStr && WStr[0])
					Label.assign(WStr);
			}
			if (Label.empty())
			{
				wchar_t Buf[32] = {};
				swprintf_s(Buf, 32, L"Mount_%d", MountId);
				Label.assign(Buf);
			}
			Entries.emplace_back(MountId, Label);
		}
	}

	// Fallback: use GetDataTableRowNames + GetDataTableRowFromName
	if (Entries.empty())
	{
		TArray<FName> RowNames;
		UDataTableFunctionLibrary::GetDataTableRowNames(MountTable, &RowNames);
		for (const FName& RowName : RowNames)
		{
			FMountInfoSetting Row{};
			if (!UDataTableFunctionLibrary::GetDataTableRowFromName(
				MountTable, RowName, reinterpret_cast<FTableRowBase*>(&Row)))
				continue;

			FString DescStr = UKismetTextLibrary::Conv_TextToString(Row.Description);
			const wchar_t* DescWs = DescStr.CStr();
			std::wstring Label = (DescWs && DescWs[0]) ? std::wstring(DescWs) : std::wstring();
			if (Label.empty())
			{
				wchar_t Buf[32] = {};
				swprintf_s(Buf, 32, L"Mount_%d", Row.ID);
				Label.assign(Buf);
			}
			Entries.emplace_back(Row.ID, Label);
		}
	}

	std::sort(Entries.begin(), Entries.end(),
		[](const std::pair<int32, std::wstring>& A, const std::pair<int32, std::wstring>& B)
		{ return A.first < B.first; });

	GTab5MountOptionIds.reserve(Entries.size());
	GTab5MountOptionLabels.reserve(Entries.size());
	for (const auto& It : Entries)
	{
		GTab5MountOptionIds.push_back(It.first);
		GTab5MountOptionLabels.push_back(It.second);
	}

	LOGI_STREAM("Tab5System") << "[SDK][Tab5] ResolveMountOptions count=" << GTab5MountOptionIds.size();
	for (size_t i = 0; i < GTab5MountOptionIds.size(); ++i)
	{
		LOGI_STREAM("Tab5System") << L" [" << i << L"]=" << GTab5MountOptionIds[i]
		           << L":" << GTab5MountOptionLabels[i].c_str();
	}
	LOGI_STREAM("Tab5System") << "\n";
}

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
	AddToggleStored(MoveBox, L"空格跳跃", GTab5.SpaceJumpToggle);
	AddSliderStored(MoveBox, L"跳跃速度", GTab5.JumpSpeedSlider);
	if (GTab5.JumpSpeedSlider && GTab5.JumpSpeedSlider->VolumeSlider)
	{
		GTab5.JumpSpeedSlider->VolumeSlider->SetValue(6.0f);
		if (GTab5.JumpSpeedSlider->TXT_CurrentValue)
			GTab5.JumpSpeedSlider->TXT_CurrentValue->SetText(MakeText(L"6"));
	}
	AddToggleStored(MoveBox, L"无限跳跃", GTab5.InfiniteJumpToggle);
	AddToggleStored(MoveBox, L"奔跑/骑马加速", GTab5.RunMountSpeedToggle);
	AddSliderStored(MoveBox, L"加速倍率", GTab5.RunMountSpeedSlider);
	if (GTab5.RunMountSpeedSlider && GTab5.RunMountSpeedSlider->VolumeSlider)
	{
		GTab5.RunMountSpeedSlider->VolumeSlider->SetValue(2.0f);
		if (GTab5.RunMountSpeedSlider->TXT_CurrentValue)
			GTab5.RunMountSpeedSlider->TXT_CurrentValue->SetText(MakeText(L"2"));
	}
	AddPanelWithFixedGap(MovePanel, 0.0f, 10.0f);

	auto* MountPanel = CreateCollapsiblePanel(PC, L"坐骑设置");
	auto* MountBox = MountPanel ? MountPanel->CT_Contents : nullptr;
	AddToggleStored(MountBox, L"坐骑替换", GTab5.MountReplaceToggle);
	AddDropdownStored(MountBox, L"指定坐骑", { L"..." }, GTab5.MountSelectDD);
	// Dynamically populate mount dropdown from game DataTable
	ResolveMountOptionsFromTable();
	if (GTab5.MountSelectDD && GTab5.MountSelectDD->CB_Main &&
		IsSafeLiveObject(static_cast<UObject*>(GTab5.MountSelectDD->CB_Main)))
	{
		UComboBoxString* Combo = GTab5.MountSelectDD->CB_Main;
		Combo->ClearOptions();
		if (!GTab5MountOptionLabels.empty())
		{
			for (const auto& Label : GTab5MountOptionLabels)
				Combo->AddOption(FString(Label.c_str()));
		}
		else
		{
			Combo->AddOption(FString(L"马"));
		}
		Combo->SetSelectedIndex(0);
	}
	AddPanelWithFixedGap(MountPanel, 0.0f, 10.0f);

	auto* StoryPanel = CreateCollapsiblePanel(PC, L"开档与解锁");
	auto* StoryBox = StoryPanel ? StoryPanel->CT_Contents : nullptr;
	AddToggleStored(StoryBox, L"一周目可选极难", GTab5.FirstPlayHardToggle);
	AddToggleStored(StoryBox, L"一周目可选传承", GTab5.FirstPlayInheritToggle);
	AddToggleStored(StoryBox, L"未交互驿站可用", GTab5.PostStationToggle);
	AddToggle(StoryBox, L"激活GM命令行");
	AddToggleStored(StoryBox, L"解锁全图鉴", GTab5.UnlockCodexToggle);
	AddToggleStored(StoryBox, L"解锁全成就", GTab5.UnlockAchievementToggle);
	AddPanelWithFixedGap(StoryPanel, 0.0f, 10.0f);

	auto AddNumericStored = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue, UBPVE_JHConfigVolumeItem2_C*& OutRef) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			OutRef = Item;
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto* ScreenPanel = CreateCollapsiblePanel(PC, L"屏幕设置");
	auto* ScreenBox = ScreenPanel ? ScreenPanel->CT_Contents : nullptr;
	AddNumericStored(ScreenBox, L"分辨率X", L"1920", GTab5.ResolutionXEdit);
	AddNumericStored(ScreenBox, L"分辨率Y", L"1080", GTab5.ResolutionYEdit);
	AddDropdownStored(ScreenBox, L"首选屏幕模式", { L"全屏", L"无边框窗口", L"窗口" }, GTab5.ScreenModeDD);
	AddDropdownStored(ScreenBox, L"使用垂直同步", { L"否", L"是" }, GTab5.VSyncDD);
	AddDropdownStored(ScreenBox, L"使用动态分辨率", { L"否", L"是" }, GTab5.DynResDD);
	AddPanelWithFixedGap(ScreenPanel, 0.0f, 10.0f);
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

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  解锁全图鉴
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{
	// 假需求: FRequirementSetting{Type=5, ID=150, Num=1.0} — "永远满足"
	struct FakeRequirement
	{
		int32 Type = 5;
		int32 ID = 150;
		float Num = 1.0f;
	};
	static FakeRequirement GFakeReq;

	// 备份结构: 保存每行原始 Requirements TArray 指针与大小
	struct ReqBackupEntry
	{
		uintptr_t OrigDataPtr;
		int32     OrigArrayNum;
		int32     OrigArrayMax;
	};

	// 成就额外备份: AchievementType
	struct AchiBackupEntry : ReqBackupEntry
	{
		int32 OrigAchievementType;
	};

	static std::vector<ReqBackupEntry>  GCodexBackup;
	static bool                         GUnlockCodexApplied = false;

	static std::vector<AchiBackupEntry> GAchiBackup;
	static bool                         GUnlockAchievementApplied = false;
}

void EnableUnlockAllCodex()
{
	if (GUnlockCodexApplied) return;

	UPictorialResManager* PicMgr = UManagerFuncLib::GetPictorialResManager();
	if (!PicMgr || !IsSafeLiveObject(static_cast<UObject*>(PicMgr)))
	{
		LOGE_STREAM("Tab5System") << "[SDK] UnlockCodex: PictorialResManager is null\n";
		return;
	}

	UDataTable* Table = PicMgr->PictorialResourceTable;
	if (!Table || !IsSafeLiveObject(static_cast<UObject*>(Table)))
	{
		LOGE_STREAM("Tab5System") << "[SDK] UnlockCodex: PictorialResourceTable is null\n";
		return;
	}

	auto& RowMap = Table->RowMap;
	const int32 AllocatedSlots = RowMap.IsValid() ? RowMap.NumAllocated() : 0;
	if (AllocatedSlots <= 0)
	{
		LOGE_STREAM("Tab5System") << "[SDK] UnlockCodex: RowMap empty\n";
		return;
	}

	GCodexBackup.clear();
	GCodexBackup.reserve(AllocatedSlots);

	const uintptr_t FakePtr = reinterpret_cast<uintptr_t>(&GFakeReq);

	int32 Modified = 0;
	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		if (!RowMap.IsValidIndex(i)) continue;
		uint8* RowData = RowMap[i].Value();
		if (!RowData) continue;

		// FPictorialSetting::DefaultRequirements TArray at offset 0x98
		ReqBackupEntry Entry{};
		Entry.OrigDataPtr = *reinterpret_cast<uintptr_t*>(RowData + 0x98);
		Entry.OrigArrayNum = *reinterpret_cast<int32*>(RowData + 0xA0);
		Entry.OrigArrayMax = *reinterpret_cast<int32*>(RowData + 0xA4);
		GCodexBackup.push_back(Entry);

		// Overwrite with fake requirement
		*reinterpret_cast<uintptr_t*>(RowData + 0x98) = FakePtr;
		*reinterpret_cast<int32*>(RowData + 0xA0) = 1;
		*reinterpret_cast<int32*>(RowData + 0xA4) = 1;
		++Modified;
	}

	GUnlockCodexApplied = true;
	LOGI_STREAM("Tab5System") << "[SDK] UnlockCodex enabled, modified " << Modified << " rows\n";
}

void DisableUnlockAllCodex()
{
	if (!GUnlockCodexApplied) return;

	UPictorialResManager* PicMgr = UManagerFuncLib::GetPictorialResManager();
	UDataTable* Table = (PicMgr && IsSafeLiveObject(static_cast<UObject*>(PicMgr)))
		? PicMgr->PictorialResourceTable : nullptr;

	if (Table && IsSafeLiveObject(static_cast<UObject*>(Table)))
	{
		auto& RowMap = Table->RowMap;
		const int32 AllocatedSlots = RowMap.IsValid() ? RowMap.NumAllocated() : 0;

		size_t BackupIdx = 0;
		for (int32 i = 0; i < AllocatedSlots && BackupIdx < GCodexBackup.size(); ++i)
		{
			if (!RowMap.IsValidIndex(i)) continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData) continue;

			const auto& Entry = GCodexBackup[BackupIdx++];
			*reinterpret_cast<uintptr_t*>(RowData + 0x98) = Entry.OrigDataPtr;
			*reinterpret_cast<int32*>(RowData + 0xA0) = Entry.OrigArrayNum;
			*reinterpret_cast<int32*>(RowData + 0xA4) = Entry.OrigArrayMax;
		}
	}

	GCodexBackup.clear();
	GUnlockCodexApplied = false;
	LOGI_STREAM("Tab5System") << "[SDK] UnlockCodex disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  解锁全成就
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableUnlockAllAchievements()
{
	if (GUnlockAchievementApplied) return;

	UAchievementResManager* AchiMgr = UManagerFuncLib::GetAchievementResManager();
	if (!AchiMgr || !IsSafeLiveObject(static_cast<UObject*>(AchiMgr)))
	{
		LOGE_STREAM("Tab5System") << "[SDK] UnlockAchievement: AchievementResManager is null\n";
		return;
	}

	UDataTable* Table = AchiMgr->AchievementResourceTable;
	if (!Table || !IsSafeLiveObject(static_cast<UObject*>(Table)))
	{
		LOGE_STREAM("Tab5System") << "[SDK] UnlockAchievement: AchievementResourceTable is null\n";
		return;
	}

	auto& RowMap = Table->RowMap;
	const int32 AllocatedSlots = RowMap.IsValid() ? RowMap.NumAllocated() : 0;
	if (AllocatedSlots <= 0)
	{
		LOGE_STREAM("Tab5System") << "[SDK] UnlockAchievement: RowMap empty\n";
		return;
	}

	GAchiBackup.clear();
	GAchiBackup.reserve(AllocatedSlots);

	const uintptr_t FakePtr = reinterpret_cast<uintptr_t>(&GFakeReq);

	int32 Modified = 0;
	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		if (!RowMap.IsValidIndex(i)) continue;
		uint8* RowData = RowMap[i].Value();
		if (!RowData) continue;

		// FAchievementInfoSetting::Requirements TArray at offset 0x20
		AchiBackupEntry Entry{};
		Entry.OrigDataPtr = *reinterpret_cast<uintptr_t*>(RowData + 0x20);
		Entry.OrigArrayNum = *reinterpret_cast<int32*>(RowData + 0x28);
		Entry.OrigArrayMax = *reinterpret_cast<int32*>(RowData + 0x2C);
		Entry.OrigAchievementType = *reinterpret_cast<int32*>(RowData + 0x40);
		GAchiBackup.push_back(Entry);

		// Overwrite with fake requirement
		*reinterpret_cast<uintptr_t*>(RowData + 0x20) = FakePtr;
		*reinterpret_cast<int32*>(RowData + 0x28) = 1;
		*reinterpret_cast<int32*>(RowData + 0x2C) = 1;
		// Set AchievementType = 3 (completed)
		*reinterpret_cast<int32*>(RowData + 0x40) = 3;
		++Modified;
	}

	GUnlockAchievementApplied = true;
	LOGI_STREAM("Tab5System") << "[SDK] UnlockAchievement enabled, modified " << Modified << " rows\n";
}

void DisableUnlockAllAchievements()
{
	if (!GUnlockAchievementApplied) return;

	UAchievementResManager* AchiMgr = UManagerFuncLib::GetAchievementResManager();
	UDataTable* Table = (AchiMgr && IsSafeLiveObject(static_cast<UObject*>(AchiMgr)))
		? AchiMgr->AchievementResourceTable : nullptr;

	if (Table && IsSafeLiveObject(static_cast<UObject*>(Table)))
	{
		auto& RowMap = Table->RowMap;
		const int32 AllocatedSlots = RowMap.IsValid() ? RowMap.NumAllocated() : 0;

		size_t BackupIdx = 0;
		for (int32 i = 0; i < AllocatedSlots && BackupIdx < GAchiBackup.size(); ++i)
		{
			if (!RowMap.IsValidIndex(i)) continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData) continue;

			const auto& Entry = GAchiBackup[BackupIdx++];
			*reinterpret_cast<uintptr_t*>(RowData + 0x20) = Entry.OrigDataPtr;
			*reinterpret_cast<int32*>(RowData + 0x28) = Entry.OrigArrayNum;
			*reinterpret_cast<int32*>(RowData + 0x2C) = Entry.OrigArrayMax;
			*reinterpret_cast<int32*>(RowData + 0x40) = Entry.OrigAchievementType;
		}
	}

	GAchiBackup.clear();
	GUnlockAchievementApplied = false;
	LOGI_STREAM("Tab5System") << "[SDK] UnlockAchievement disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  屏幕设置 (via UGameUserSettings SDK)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{
	// 安全读取下拉框选中索引
	int32 SafeReadDropdownIndex(UBPVE_JHConfigVideoItem2_C* DD)
	{
		if (!DD || !IsSafeLiveObject(static_cast<UObject*>(DD))) return -1;
		if (!DD->CB_Main || !IsSafeLiveObject(static_cast<UObject*>(DD->CB_Main))) return -1;
		return DD->CB_Main->GetSelectedIndex();
	}

	// 安全读取数字编辑框中的整数值
	int32 SafeReadNumericEdit(UBPVE_JHConfigVolumeItem2_C* Edit, int32 DefaultValue)
	{
		if (!Edit || !IsSafeLiveObject(static_cast<UObject*>(Edit))) return DefaultValue;
		if (!Edit->TXT_CurrentValue || !IsSafeLiveObject(static_cast<UObject*>(Edit->TXT_CurrentValue)))
			return DefaultValue;

		const std::string Raw = Edit->TXT_CurrentValue->GetText().ToString();
		if (Raw.empty()) return DefaultValue;

		try { int32 Val = std::stoi(Raw); return (Val > 0) ? Val : DefaultValue; }
		catch (...) { return DefaultValue; }
	}
}

void ApplyScreenSettings()
{
	// 读取当前 UI 值
	const int32 ResX = SafeReadNumericEdit(GTab5.ResolutionXEdit, 0);
	const int32 ResY = SafeReadNumericEdit(GTab5.ResolutionYEdit, 0);
	const int32 ModeIdx = SafeReadDropdownIndex(GTab5.ScreenModeDD);
	const int32 VSyncIdx = SafeReadDropdownIndex(GTab5.VSyncDD);
	const int32 DynResIdx = SafeReadDropdownIndex(GTab5.DynResDD);

	// 用静态变量追踪上一次 UI 值, 只在值变化时才应用
	static int32 LastResX = 0, LastResY = 0;
	static int32 LastModeIdx = -1, LastVSyncIdx = -1, LastDynResIdx = -1;
	static bool  bInitialized = false;

	// 首次调用: 记录初始 UI 值, 不做任何修改
	if (!bInitialized)
	{
		LastResX = ResX; LastResY = ResY;
		LastModeIdx = ModeIdx; LastVSyncIdx = VSyncIdx; LastDynResIdx = DynResIdx;
		bInitialized = true;
		return;
	}

	// 检测是否有 UI 值变化
	const bool bResChanged = (ResX > 0 && ResY > 0 && (ResX != LastResX || ResY != LastResY));
	const bool bModeChanged = (ModeIdx >= 0 && ModeIdx != LastModeIdx);
	const bool bVSyncChanged = (VSyncIdx >= 0 && VSyncIdx != LastVSyncIdx);
	const bool bDynResChanged = (DynResIdx >= 0 && DynResIdx != LastDynResIdx);

	if (!bResChanged && !bModeChanged && !bVSyncChanged && !bDynResChanged)
		return;

	// 更新追踪值
	if (bResChanged) { LastResX = ResX; LastResY = ResY; }
	if (bModeChanged) LastModeIdx = ModeIdx;
	if (bVSyncChanged) LastVSyncIdx = VSyncIdx;
	if (bDynResChanged) LastDynResIdx = DynResIdx;

	// 直接在当前线程应用 (UI-tracking 保证不会在首次打开菜单时触发)
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	if (bResChanged)
	{
		FIntPoint Res; Res.X = ResX; Res.Y = ResY;
		Settings->SetScreenResolution(Res);
		LOGI_STREAM("Tab5System") << "[SDK] ScreenSettings: Resolution " << ResX << "x" << ResY << "\n";
	}
	if (bModeChanged)
	{
		Settings->SetFullscreenMode(static_cast<EWindowMode>(ModeIdx));
		LOGI_STREAM("Tab5System") << "[SDK] ScreenSettings: FullscreenMode " << ModeIdx << "\n";
	}
	if (bVSyncChanged)
	{
		Settings->SetVSyncEnabled(VSyncIdx == 1);
		LOGI_STREAM("Tab5System") << "[SDK] ScreenSettings: VSync " << VSyncIdx << "\n";
	}
	if (bDynResChanged)
	{
		Settings->SetDynamicResolutionEnabled(DynResIdx == 1);
		LOGI_STREAM("Tab5System") << "[SDK] ScreenSettings: DynRes " << DynResIdx << "\n";
	}

	Settings->ApplySettings(false);
	LOGI_STREAM("Tab5System") << "[SDK] ScreenSettings: Applied\n";
}
