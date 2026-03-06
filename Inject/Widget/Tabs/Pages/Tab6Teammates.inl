void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTab.Content6) return;
	GDynTab.Content6->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GTeammate.FollowToggle = nullptr;
	GTeammate.FollowCount = nullptr;
	GTeammate.AddDD = nullptr;
	GTeammate.ReplaceToggle = nullptr;
	GTeammate.ReplaceDD = nullptr;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTab.Content6->AddChild(Panel);
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

	auto* TeamPanel = CreateCollapsiblePanel(PC, L"队伍设置");
	auto* TeamBox = TeamPanel ? TeamPanel->CT_Contents : nullptr;
	GTeammate.FollowToggle = CreateToggleItem(PC, L"设置队友跟随数量");
	if (GTeammate.FollowToggle)
	{
		if (TeamBox) TeamBox->AddChild(GTeammate.FollowToggle);
		else GDynTab.Content6->AddChild(GTeammate.FollowToggle);
		Count++;
	}
	GTeammate.FollowCount = CreateVolumeNumericEditBoxItem(PC, Outer, TeamBox ? TeamBox : GDynTab.Content6, L"跟随数量", L"输入数字", L"3");
	if (GTeammate.FollowCount)
	{
		if (TeamBox) TeamBox->AddChild(GTeammate.FollowCount);
		else GDynTab.Content6->AddChild(GTeammate.FollowCount);
		Count++;
	}
	AddPanelWithFixedGap(TeamPanel, 0.0f, 10.0f);

	auto* OperatePanel = CreateCollapsiblePanel(PC, L"队友操作");
	auto* OperateBox = OperatePanel ? OperatePanel->CT_Contents : nullptr;
	GTeammate.AddDD = CreateVideoItemWithOptions(PC, L"添加队友",
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammate.AddDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.AddDD);
		else GDynTab.Content6->AddChild(GTeammate.AddDD);
		Count++;
	}
	GTeammate.ReplaceToggle = CreateToggleItem(PC, L"替换指定队友");
	if (GTeammate.ReplaceToggle)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.ReplaceToggle);
		else GDynTab.Content6->AddChild(GTeammate.ReplaceToggle);
		Count++;
	}
	GTeammate.ReplaceDD = CreateVideoItemWithOptions(PC, L"指定队友",
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammate.ReplaceDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.ReplaceDD);
		else GDynTab.Content6->AddChild(GTeammate.ReplaceDD);
		Count++;
	}
	AddPanelWithFixedGap(OperatePanel, 0.0f, 8.0f);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  Tab6 Enable/Disable implementations
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{

// ── 设置队友跟随数量 ──
uint32_t GFollowerCountHookId = UINT32_MAX;
uintptr_t GFollowerCountOffset = 0;
volatile LONG GFollowerCountValue = 99;
const char* kFollowerCountPattern = "44 3B BE ?? 05 00 00 0F 8C";

const unsigned char kFollowerCountTrampolineTemplate[] = {
	0x50,                                             // push rax
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&GFollowerCountValue)
	0x41, 0x8B, 0x03,                                 // mov eax, dword [r11]
	0xFF, 0xC0,                                       // inc eax
	0x44, 0x3B, 0xF8,                                 // cmp r15d, eax (flags = r15d - eax)
	0x41, 0x5B,                                       // pop r11
	0x58,                                             // pop rax
};
constexpr size_t kFollowerCountImm64Offset = 5;

// ── 替换指定队友 ──
uint32_t GReplaceTeammateHookId = UINT32_MAX;
uintptr_t GReplaceTeammateOffset = 0;
volatile LONG GReplaceTeammateId = 30;
const char* kReplaceTeammatePattern = "48 33 C4 48 89 45 ?? 44 88 44 24 ??";

const unsigned char kReplaceTeammateTrampolineTemplate[] = {
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&GReplaceTeammateId)
	0x41, 0x8B, 0x13,                                 // mov edx, dword [r11]
	0x41, 0x5B,                                       // pop r11
};
constexpr size_t kReplaceTeammateImm64Offset = 4;

constexpr int32 kRoleIds[] = { 0, 30, 31, 32, 33, 34, 35, 36 };

} // end anonymous namespace

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  设置队友跟随数量
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void SetFollowerCountValue(int32 Value)
{
	if (Value < 1) Value = 1;
	if (Value > 99) Value = 99;
	InterlockedExchange(&GFollowerCountValue, static_cast<LONG>(Value));
}

void EnableFollowerCountHook()
{
	if (GFollowerCountHookId != UINT32_MAX) return;

	if (GFollowerCountOffset == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kFollowerCountPattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab6Teammates") << "[SDK] FollowerCount AobScan failed\n";
			return;
		}
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (!hModule) return;
		GFollowerCountOffset = found - reinterpret_cast<uintptr_t>(hModule);
	}

	unsigned char code[sizeof(kFollowerCountTrampolineTemplate)];
	std::memcpy(code, kFollowerCountTrampolineTemplate, sizeof(code));
	const uintptr_t valAddr = reinterpret_cast<uintptr_t>(&GFollowerCountValue);
	std::memcpy(code + kFollowerCountImm64Offset, &valAddr, sizeof(valAddr));

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GFollowerCountOffset),
		code, sizeof(code), hookId,
		false, true, false))
	{
		LOGE_STREAM("Tab6Teammates") << "[SDK] FollowerCount hook install failed\n";
		return;
	}
	GFollowerCountHookId = hookId;
	LOGI_STREAM("Tab6Teammates") << "[SDK] FollowerCount hook enabled, ID: " << hookId << "\n";
}

void DisableFollowerCountHook()
{
	if (GFollowerCountHookId == UINT32_MAX) return;
	InlineHook::HookManager::UninstallHook(GFollowerCountHookId);
	GFollowerCountHookId = UINT32_MAX;
	LOGI_STREAM("Tab6Teammates") << "[SDK] FollowerCount hook disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  替换指定队友
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void SetReplaceTeammateId(int32 Value)
{
	InterlockedExchange(&GReplaceTeammateId, static_cast<LONG>(Value));
}

void EnableReplaceTeammateHook()
{
	if (GReplaceTeammateHookId != UINT32_MAX) return;

	if (GReplaceTeammateOffset == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kReplaceTeammatePattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate AobScan failed\n";
			return;
		}
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (!hModule) return;
		GReplaceTeammateOffset = found - reinterpret_cast<uintptr_t>(hModule);
	}

	unsigned char code[sizeof(kReplaceTeammateTrampolineTemplate)];
	std::memcpy(code, kReplaceTeammateTrampolineTemplate, sizeof(code));
	const uintptr_t idAddr = reinterpret_cast<uintptr_t>(&GReplaceTeammateId);
	std::memcpy(code + kReplaceTeammateImm64Offset, &idAddr, sizeof(idAddr));

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GReplaceTeammateOffset),
		code, sizeof(code), hookId))
	{
		LOGE_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate hook install failed\n";
		return;
	}
	GReplaceTeammateHookId = hookId;
	LOGI_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate hook enabled, ID: " << hookId << "\n";
}

void DisableReplaceTeammateHook()
{
	if (GReplaceTeammateHookId == UINT32_MAX) return;
	InlineHook::HookManager::UninstallHook(GReplaceTeammateHookId);
	GReplaceTeammateHookId = UINT32_MAX;
	LOGI_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate hook disabled\n";
}
