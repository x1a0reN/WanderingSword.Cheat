void PopulateTab_Social(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->LanSlot, "Tab4(LanSlot)");
	if (!Container) return;
	Container->ClearChildren();
	GTab4 = {};
	int Count = 0;

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

	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
		return Item;
	};

	auto* MainPanel = CreateCollapsiblePanel(PC, L"社交开关");
	auto* MainBox = MainPanel ? MainPanel->CT_Contents : nullptr;
	AddToggleStored(MainBox, L"送礼必定喜欢", GTab4.GiftAlwaysLikedToggle);
	AddToggleStored(MainBox, L"邀请无视条件", GTab4.InviteIgnoreToggle);
	AddToggleStored(MainBox, L"切磋无视好感", GTab4.SparIgnoreFavorToggle);
	AddToggleStored(MainBox, L"请教无视要求", GTab4.ConsultIgnoreToggle);
	AddToggleStored(MainBox, L"切磋获得对手背包", GTab4.SparGetLootToggle);
	AddToggleStored(MainBox, L"NPC装备可脱", GTab4.NpcEquipRemovableToggle);
	AddToggleStored(MainBox, L"NPC无视武器功法限制", GTab4.NpcIgnoreWeaponLimitToggle);
	AddToggleStored(MainBox, L"强制显示NPC互动", GTab4.ForceNpcInteractionToggle);
	AddPanelWithFixedGap(MainPanel, 0.0f, 10.0f);

	auto* GiftPanel = CreateCollapsiblePanel(PC, L"送礼设置");
	auto* GiftBox = GiftPanel ? GiftPanel->CT_Contents : nullptr;
	GTab4.GiftQualityDD = AddDropdown(GiftBox, L"物品质量(送礼)", { L"全部", L"白", L"绿", L"蓝", L"紫", L"橙", L"红" });
	AddPanelWithFixedGap(GiftPanel, 0.0f, 8.0f);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  Tab4 Enable/Disable implementations
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{

// ── 送礼必定喜欢 ──
uintptr_t GSongLi1Addr = 0;
uintptr_t GSongLi2Offset = 0;
uint32_t GSongLiQualityHookId = UINT32_MAX;
uint32_t GSongLiStrongEnoughHookId = UINT32_MAX;
uintptr_t GSongLiQualityOffset = 0;
volatile uint8_t GSongLiQualityValue = 5;
volatile float GSongLiStrongEnoughValue = 99999.0f;
volatile LONG GSongLiStrongEnoughFlag = 0;

const char* kSongLi1Pattern = "E8 ?? ?? ?? ?? C1 E8 1F 34 01";
const char* kSongLi2Pattern = "0F 10 ?? ?? 02 00 00 4C 8D ?? 24 ?? ?? 00 00 49 8B ?? ?? ?? 0F 28";
const char* kSongLiQualityPattern = "88 87 80 00 00 00 0F B6 83";

const unsigned char kSongLiStrongEnoughTrampolineTemplate[] = {
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&flag)
	0x41, 0x83, 0x3B, 0x01,                           // cmp dword ptr [r11], 1
	0x75, 0x1C,                                       // jne skip
	0x83, 0x7E, 0x38, 0x00,                           // cmp dword ptr [rsi+0x38], 0
	0x75, 0x16,                                       // jne skip
	0x41, 0xC7, 0x03, 0x00, 0x00, 0x00, 0x00,         // mov dword ptr [r11], 0
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&value)
	0xF3, 0x41, 0x0F, 0x10, 0x03,                     // movss xmm0, dword ptr [r11]
	0x41, 0x5B,                                       // pop r11
};
constexpr size_t kSongLiStrongEnoughFlagImm64Offset = 4;
constexpr size_t kSongLiStrongEnoughValueImm64Offset = 33;

const unsigned char kSongLiQualityTrampolineTemplate[] = {
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&flag)
	0x41, 0xC7, 0x03, 0x01, 0x00, 0x00, 0x00,         // mov dword [r11], 1
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&quality)
	0x41, 0x3A, 0x03,                                 // cmp al, byte [r11]
	0x7D, 0x03,                                       // jge +3
	0x41, 0x8A, 0x03,                                 // mov al, byte [r11]
	0x41, 0x5B,                                       // pop r11
	0x88, 0x87, 0x80, 0x00, 0x00, 0x00,               // mov [rdi+0x80], al (original)
};
constexpr size_t kSongLiFlagImm64Offset = 4;
constexpr size_t kSongLiQualImm64Offset = 21;

// ── 邀请无视条件 ──
uintptr_t GCanInvite1Addr = 0;
uintptr_t GCanInvite2Addr = 0;
const char* kCanInvite1Pattern = "48 8D ?? ?? ?? 00 00 48 8D 54 24 ?? E8 ?? ?? ?? ?? 84 C0";
const char* kCanInvite2Pattern = "48 8B ?? E8 ?? ?? ?? ?? 83 F8 3C 7D ?? 48";

// ── 切磋无视好感 ──
uintptr_t GQieCuoFriendlinessAddr = 0;
uint32_t GQieCuoFriendlinessHookId = UINT32_MAX;
uintptr_t GQieCuoFriendlinessOffset = 0;
const char* kQieCuoFriendlinessPattern = "89 06 48 8B 74 24 ?? 48";

const unsigned char kQieCuoTrampolineCode[] = {
	0x83, 0xF8, 0x14,                                 // cmp eax, 20
	0x7D, 0x05,                                       // jge +5
	0xB8, 0x14, 0x00, 0x00, 0x00,                     // mov eax, 20
};

// ── 请教无视要求 ──
uintptr_t GVigorIsMetAddr = 0;
uintptr_t GPlayerFriendlinessIsMetAddr = 0;
uintptr_t GConsultRequirementsIsMetAddr = 0;
uintptr_t GConsultCanSelect1Offset = 0;
uintptr_t GConsultCanSelect2Addr = 0;
uintptr_t GConsultCanSelect3Addr = 0;
uint32_t GConsultCanSelect1HookId = UINT32_MAX;

const char* kVigorIsMetPattern = "84 C0 74 08 F3 0F 10 ?? ?? ?? ?? ?? 0F 2F ?? 73";
const char* kPlayerFriendlinessIsMetPattern = "7E ?? 0F 2F ?? 73 ?? 48 8D";
const char* kConsultRequirementsIsMetPattern = "41 8B F8 8B F2 48 8B D9 E8 ?? ?? ?? ?? 84 C0 74";
const char* kConsultCanSelect1Pattern = "?? 8B ?? ?? 8B ?? E8 ?? ?? ?? ?? 44 0F B6 ?? ?? 8D ?? 24 ?? 48 8D";
const char* kConsultCanSelect2Pattern = "FF 90 ?? ?? 00 00 83 F8 01 74 05";
const char* kConsultCanSelect3Pattern = "FF 90 ?? ?? 00 00 83 F8 01 74 ?? 48 8D";
const unsigned char kConsultCanSelect1TrampolineCode[] = { 0x0C, 0x01 };

// ── 切磋获得对手背包 ──
uintptr_t GAllLoot1Addr = 0;
uintptr_t GAllLoot2Addr = 0;
const char* kAllLoot1Pattern = "0F 4E D9 FF C3";
const char* kAllLoot2Pattern = "03 00 00 00 EB ?? 85 DB";

// ── NPC装备可脱 ──
uintptr_t GNpcEquipCanRemove1Addr = 0;
uintptr_t GNpcEquipCanRemove2Addr = 0;
const char* kNpcEquipCanRemove1Pattern = "80 B9 ?? ?? 00 00 00 0F 85 ?? ?? 00 00 48 8D 45";
const char* kNpcEquipCanRemove2Pattern = "0F B6 83 85 00 00 00 88 87 85 00 00 00";

// ── NPC无视武器功法限制 ──
uintptr_t GIgnoreWeaponLimitsAddr = 0;
uintptr_t GIgnoreSkillLimitsAddr = 0;
uintptr_t GProtagonistSkillUiOffset = 0;
uint32_t GProtagonistSkillUiHookId = UINT32_MAX;
const char* kIgnoreWeaponLimitsPattern = "0F 44 ?? ?? FF ?? 48 ?? ?? 75 ?? 40 84 ?? 75 ??";
const char* kIgnoreSkillLimitsPattern = "49 8B 4F 70 E8 ?? ?? ?? ?? 3C 04 75 75";
const char* kProtagonistSkillUiPattern = "B2 01 49 8B 86 ?? ?? 00 00";
const unsigned char kProtagonistSkillUiTrampolineTemplate[] = {
	0x41, 0x50,                                       // push r8
	0x49, 0xB8,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r8, imm64 (engine ptr)
	0x4C, 0x3B, 0xC4,                                 // cmp r8, rsp
	0x76, 0x2E,                                       // jna skip
	0x4D, 0x8B, 0x80, 0x60, 0x0C, 0x00, 0x00,         // mov r8, [r8+0xC60]
	0x4C, 0x3B, 0xC4,                                 // cmp r8, rsp
	0x76, 0x22,                                       // jna skip
	0x4D, 0x8B, 0x40, 0x38,                           // mov r8, [r8+0x38]
	0x4C, 0x3B, 0xC4,                                 // cmp r8, rsp
	0x76, 0x19,                                       // jna skip
	0x4D, 0x8B, 0x40, 0x38,                           // mov r8, [r8+0x38]
	0x4C, 0x3B, 0xC4,                                 // cmp r8, rsp
	0x76, 0x10,                                       // jna skip
	0x4D, 0x8B, 0x40, 0x30,                           // mov r8, [r8+0x30]
	0x4C, 0x3B, 0xC4,                                 // cmp r8, rsp
	0x76, 0x07,                                       // jna skip
	0x4D, 0x8B, 0x40, 0x08,                           // mov r8, [r8+0x08]
	0x4D, 0x89, 0xC6,                                 // mov r14, r8
	0x41, 0x58,                                       // pop r8
};
constexpr size_t kProtagonistSkillUiEngineImm64Offset = 4;

// ── 强制显示NPC互动 ──
// (placeholder - complex feature, implemented later)

} // end anonymous namespace

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  送礼必定喜欢
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableGiftAlwaysLiked()
{
	if (GSongLi1Addr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kSongLi1Pattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked pattern1 AobScan failed\n";
			return;
		}
		GSongLi1Addr = found + 0x8;
		LOGI_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked pattern1 at: 0x" << std::hex << GSongLi1Addr << std::dec << "\n";
	}
	const unsigned char p1[] = { 0x0C, 0x01 };
	InlineHook::HookManager::WriteMemory(GSongLi1Addr, p1, sizeof(p1));

	if (GSongLiStrongEnoughHookId == UINT32_MAX)
	{
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (!hModule) return;

		if (GSongLi2Offset == 0)
		{
			const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
				"JH-Win64-Shipping.exe", kSongLi2Pattern);
			if (found == 0)
			{
				LOGE_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked strong-enough AobScan failed\n";
				return;
			}
			GSongLi2Offset = (found + 0x7) - reinterpret_cast<uintptr_t>(hModule);
		}

		unsigned char code[sizeof(kSongLiStrongEnoughTrampolineTemplate)];
		std::memcpy(code, kSongLiStrongEnoughTrampolineTemplate, sizeof(code));

		const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GSongLiStrongEnoughFlag);
		const uintptr_t valueAddr = reinterpret_cast<uintptr_t>(&GSongLiStrongEnoughValue);
		std::memcpy(code + kSongLiStrongEnoughFlagImm64Offset, &flagAddr, sizeof(flagAddr));
		std::memcpy(code + kSongLiStrongEnoughValueImm64Offset, &valueAddr, sizeof(valueAddr));

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GSongLi2Offset),
			code, sizeof(code), hookId,
			false))
		{
			LOGE_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked strong-enough hook install failed\n";
			return;
		}
		GSongLiStrongEnoughHookId = hookId;
	}

	if (GSongLiQualityHookId == UINT32_MAX)
	{
		if (GSongLiQualityOffset == 0)
		{
			const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
				"JH-Win64-Shipping.exe", kSongLiQualityPattern);
			if (found == 0)
			{
				LOGE_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked quality AobScan failed\n";
				return;
			}
			HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
			if (!hModule) return;
			GSongLiQualityOffset = found - reinterpret_cast<uintptr_t>(hModule);
		}

		unsigned char code[sizeof(kSongLiQualityTrampolineTemplate)];
		std::memcpy(code, kSongLiQualityTrampolineTemplate, sizeof(code));
		const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GSongLiStrongEnoughFlag);
		const uintptr_t qualAddr = reinterpret_cast<uintptr_t>(&GSongLiQualityValue);
		std::memcpy(code + kSongLiFlagImm64Offset, &flagAddr, sizeof(flagAddr));
		std::memcpy(code + kSongLiQualImm64Offset, &qualAddr, sizeof(qualAddr));

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GSongLiQualityOffset),
			code, sizeof(code), hookId,
			false, true, false))
		{
			LOGE_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked quality hook install failed\n";
			return;
		}
		GSongLiQualityHookId = hookId;
	}

	LOGI_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked enabled\n";
}

void DisableGiftAlwaysLiked()
{
	if (GSongLi1Addr != 0)
	{
		const unsigned char o1[] = { 0x34, 0x01 };
		InlineHook::HookManager::WriteMemory(GSongLi1Addr, o1, sizeof(o1));
	}
	if (GSongLiQualityHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GSongLiQualityHookId);
		GSongLiQualityHookId = UINT32_MAX;
	}
	if (GSongLiStrongEnoughHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GSongLiStrongEnoughHookId);
		GSongLiStrongEnoughHookId = UINT32_MAX;
	}
	InterlockedExchange(&GSongLiStrongEnoughFlag, 0);
	LOGI_STREAM("Tab4Social") << "[SDK] GiftAlwaysLiked disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  邀请无视条件
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableInviteIgnoreConditions()
{
	if (GCanInvite1Addr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kCanInvite1Pattern);
		if (found) GCanInvite1Addr = found + 0x11;
		else LOGE_STREAM("Tab4Social") << "[SDK] CanInvite1 AobScan failed\n";
	}
	if (GCanInvite2Addr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kCanInvite2Pattern);
		if (found) GCanInvite2Addr = found + 0x8;
		else LOGE_STREAM("Tab4Social") << "[SDK] CanInvite2 AobScan failed\n";
	}

	if (GCanInvite1Addr)
	{
		const unsigned char p[] = { 0x0C, 0x01 };
		InlineHook::HookManager::WriteMemory(GCanInvite1Addr, p, sizeof(p));
	}
	if (GCanInvite2Addr)
	{
		const unsigned char p[] = { 0x39, 0xC0, 0x90 };
		InlineHook::HookManager::WriteMemory(GCanInvite2Addr, p, sizeof(p));
	}
	LOGI_STREAM("Tab4Social") << "[SDK] InviteIgnoreConditions enabled\n";
}

void DisableInviteIgnoreConditions()
{
	if (GCanInvite1Addr)
	{
		const unsigned char o[] = { 0x84, 0xC0 };
		InlineHook::HookManager::WriteMemory(GCanInvite1Addr, o, sizeof(o));
	}
	if (GCanInvite2Addr)
	{
		const unsigned char o[] = { 0x83, 0xF8, 0x3C };
		InlineHook::HookManager::WriteMemory(GCanInvite2Addr, o, sizeof(o));
	}
	LOGI_STREAM("Tab4Social") << "[SDK] InviteIgnoreConditions disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  切磋无视好感
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableSparIgnoreFavor()
{
	if (GQieCuoFriendlinessHookId != UINT32_MAX) return;

	if (GQieCuoFriendlinessOffset == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kQieCuoFriendlinessPattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab4Social") << "[SDK] SparIgnoreFavor AobScan failed\n";
			return;
		}
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (!hModule) return;
		GQieCuoFriendlinessOffset = found - reinterpret_cast<uintptr_t>(hModule);
	}

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GQieCuoFriendlinessOffset),
		kQieCuoTrampolineCode, sizeof(kQieCuoTrampolineCode),
		hookId,
		true))
	{
		LOGE_STREAM("Tab4Social") << "[SDK] SparIgnoreFavor hook install failed\n";
		return;
	}
	GQieCuoFriendlinessHookId = hookId;
	LOGI_STREAM("Tab4Social") << "[SDK] SparIgnoreFavor enabled, ID: " << hookId << "\n";
}

void DisableSparIgnoreFavor()
{
	if (GQieCuoFriendlinessHookId == UINT32_MAX) return;
	InlineHook::HookManager::UninstallHook(GQieCuoFriendlinessHookId);
	GQieCuoFriendlinessHookId = UINT32_MAX;
	LOGI_STREAM("Tab4Social") << "[SDK] SparIgnoreFavor disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  请教无视要求
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableConsultIgnoreRequirements()
{
	auto Scan = [](const char* pattern, uintptr_t& addr, int offset, const char* tag)
	{
		if (addr != 0) return;
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", pattern);
		if (found) addr = found + offset;
		else LOGE_STREAM("Tab4Social") << "[SDK] " << tag << " AobScan failed\n";
	};

	Scan(kVigorIsMetPattern, GVigorIsMetAddr, 0xF, "VigorIsMet");
	Scan(kPlayerFriendlinessIsMetPattern, GPlayerFriendlinessIsMetAddr, 0x5, "PlayerFriendlinessIsMet");
	Scan(kConsultRequirementsIsMetPattern, GConsultRequirementsIsMetAddr, 0xD, "ConsultRequirementsIsMet");
	Scan(kConsultCanSelect2Pattern, GConsultCanSelect2Addr, 0x9, "ConsultCanSelect2");
	Scan(kConsultCanSelect3Pattern, GConsultCanSelect3Addr, 0x9, "ConsultCanSelect3");

	if (GConsultCanSelect1HookId == UINT32_MAX)
	{
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (hModule && GConsultCanSelect1Offset == 0)
		{
			const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
				"JH-Win64-Shipping.exe", kConsultCanSelect1Pattern);
			if (found)
				GConsultCanSelect1Offset = (found + 0xB) - reinterpret_cast<uintptr_t>(hModule);
			else
				LOGE_STREAM("Tab4Social") << "[SDK] ConsultCanSelect1 AobScan failed\n";
		}

		if (GConsultCanSelect1Offset != 0)
		{
			uint32_t hookId = UINT32_MAX;
			if (InlineHook::HookManager::InstallHook(
				"JH-Win64-Shipping.exe",
				static_cast<uint32_t>(GConsultCanSelect1Offset),
				kConsultCanSelect1TrampolineCode, sizeof(kConsultCanSelect1TrampolineCode),
				hookId,
				true))
			{
				GConsultCanSelect1HookId = hookId;
			}
			else
			{
				LOGE_STREAM("Tab4Social") << "[SDK] ConsultCanSelect1 hook install failed\n";
			}
		}
	}

	const unsigned char jmpByte[] = { 0xEB };
	const unsigned char forceTrue[] = { 0x0C, 0x01 };

	if (GVigorIsMetAddr) InlineHook::HookManager::WriteMemory(GVigorIsMetAddr, jmpByte, 1);
	if (GPlayerFriendlinessIsMetAddr) InlineHook::HookManager::WriteMemory(GPlayerFriendlinessIsMetAddr, jmpByte, 1);
	if (GConsultRequirementsIsMetAddr) InlineHook::HookManager::WriteMemory(GConsultRequirementsIsMetAddr, forceTrue, 2);
	if (GConsultCanSelect2Addr) InlineHook::HookManager::WriteMemory(GConsultCanSelect2Addr, jmpByte, 1);
	if (GConsultCanSelect3Addr) InlineHook::HookManager::WriteMemory(GConsultCanSelect3Addr, jmpByte, 1);

	LOGI_STREAM("Tab4Social") << "[SDK] ConsultIgnoreRequirements enabled\n";
}

void DisableConsultIgnoreRequirements()
{
	const unsigned char jae[] = { 0x73 };
	const unsigned char testAlAl[] = { 0x84, 0xC0 };
	const unsigned char je[] = { 0x74 };

	if (GVigorIsMetAddr) InlineHook::HookManager::WriteMemory(GVigorIsMetAddr, jae, 1);
	if (GPlayerFriendlinessIsMetAddr) InlineHook::HookManager::WriteMemory(GPlayerFriendlinessIsMetAddr, jae, 1);
	if (GConsultRequirementsIsMetAddr) InlineHook::HookManager::WriteMemory(GConsultRequirementsIsMetAddr, testAlAl, 2);
	if (GConsultCanSelect2Addr) InlineHook::HookManager::WriteMemory(GConsultCanSelect2Addr, je, 1);
	if (GConsultCanSelect3Addr) InlineHook::HookManager::WriteMemory(GConsultCanSelect3Addr, je, 1);
	if (GConsultCanSelect1HookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GConsultCanSelect1HookId);
		GConsultCanSelect1HookId = UINT32_MAX;
	}

	LOGI_STREAM("Tab4Social") << "[SDK] ConsultIgnoreRequirements disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  切磋获得对手背包
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableSparGetAllLoot()
{
	if (GAllLoot1Addr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kAllLoot1Pattern);
		if (found) GAllLoot1Addr = found;
		else LOGE_STREAM("Tab4Social") << "[SDK] AllLoot1 AobScan failed\n";
	}
	if (GAllLoot2Addr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kAllLoot2Pattern);
		if (found) GAllLoot2Addr = found;
		else LOGE_STREAM("Tab4Social") << "[SDK] AllLoot2 AobScan failed\n";
	}

	if (GAllLoot1Addr)
	{
		const unsigned char p[] = { 0xBB, 0x9F, 0x86, 0x01, 0x00 };
		InlineHook::HookManager::WriteMemory(GAllLoot1Addr, p, sizeof(p));
	}
	if (GAllLoot2Addr)
	{
		const unsigned char p[] = { 0x9F, 0x86, 0x01, 0x00 };
		InlineHook::HookManager::WriteMemory(GAllLoot2Addr, p, sizeof(p));
	}
	LOGI_STREAM("Tab4Social") << "[SDK] SparGetAllLoot enabled\n";
}

void DisableSparGetAllLoot()
{
	if (GAllLoot1Addr)
	{
		const unsigned char o[] = { 0x0F, 0x4E, 0xD9, 0xFF, 0xC3 };
		InlineHook::HookManager::WriteMemory(GAllLoot1Addr, o, sizeof(o));
	}
	if (GAllLoot2Addr)
	{
		const unsigned char o[] = { 0x03, 0x00, 0x00, 0x00 };
		InlineHook::HookManager::WriteMemory(GAllLoot2Addr, o, sizeof(o));
	}
	LOGI_STREAM("Tab4Social") << "[SDK] SparGetAllLoot disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  NPC装备可脱
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableNpcEquipRemovable()
{
	if (GNpcEquipCanRemove1Addr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kNpcEquipCanRemove1Pattern);
		if (found) GNpcEquipCanRemove1Addr = found;
		else LOGE_STREAM("Tab4Social") << "[SDK] NpcEquipCanRemove1 AobScan failed\n";
	}
	if (GNpcEquipCanRemove2Addr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kNpcEquipCanRemove2Pattern);
		if (found) GNpcEquipCanRemove2Addr = found;
		else LOGE_STREAM("Tab4Social") << "[SDK] NpcEquipCanRemove2 AobScan failed\n";
	}

	if (GNpcEquipCanRemove1Addr)
	{
		const unsigned char p1[] = { 0xC6, 0x81 };
		InlineHook::HookManager::WriteMemory(GNpcEquipCanRemove1Addr, p1, sizeof(p1));
		const unsigned char p2[] = { 0xEB, 0x04 };
		InlineHook::HookManager::WriteMemory(GNpcEquipCanRemove1Addr + 7, p2, sizeof(p2));
	}
	if (GNpcEquipCanRemove2Addr)
	{
		const unsigned char p[] = { 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x90 };
		InlineHook::HookManager::WriteMemory(GNpcEquipCanRemove2Addr, p, sizeof(p));
	}
	LOGI_STREAM("Tab4Social") << "[SDK] NpcEquipRemovable enabled\n";
}

void DisableNpcEquipRemovable()
{
	if (GNpcEquipCanRemove1Addr)
	{
		const unsigned char o1[] = { 0x80, 0xB9 };
		InlineHook::HookManager::WriteMemory(GNpcEquipCanRemove1Addr, o1, sizeof(o1));
		const unsigned char o2[] = { 0x0F, 0x85 };
		InlineHook::HookManager::WriteMemory(GNpcEquipCanRemove1Addr + 7, o2, sizeof(o2));
	}
	if (GNpcEquipCanRemove2Addr)
	{
		const unsigned char o[] = { 0x0F, 0xB6, 0x83, 0x85, 0x00, 0x00, 0x00 };
		InlineHook::HookManager::WriteMemory(GNpcEquipCanRemove2Addr, o, sizeof(o));
	}
	LOGI_STREAM("Tab4Social") << "[SDK] NpcEquipRemovable disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  NPC无视武器功法限制
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableNpcIgnoreWeaponLimit()
{
	if (GIgnoreWeaponLimitsAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kIgnoreWeaponLimitsPattern);
		if (found) GIgnoreWeaponLimitsAddr = found + 0xE;
		else LOGE_STREAM("Tab4Social") << "[SDK] IgnoreWeaponLimits AobScan failed\n";
	}
	if (GIgnoreSkillLimitsAddr == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kIgnoreSkillLimitsPattern);
		if (found) GIgnoreSkillLimitsAddr = found + 0xB;
		else LOGE_STREAM("Tab4Social") << "[SDK] IgnoreSkillLimits AobScan failed\n";
	}

	const unsigned char jmp[] = { 0xEB };
	if (GIgnoreWeaponLimitsAddr) InlineHook::HookManager::WriteMemory(GIgnoreWeaponLimitsAddr, jmp, 1);
	if (GIgnoreSkillLimitsAddr) InlineHook::HookManager::WriteMemory(GIgnoreSkillLimitsAddr, jmp, 1);

	if (GProtagonistSkillUiHookId == UINT32_MAX)
	{
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (hModule && GProtagonistSkillUiOffset == 0)
		{
			const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
				"JH-Win64-Shipping.exe", kProtagonistSkillUiPattern);
			if (found)
				GProtagonistSkillUiOffset = (found + 0x2) - reinterpret_cast<uintptr_t>(hModule);
			else
				LOGE_STREAM("Tab4Social") << "[SDK] ProtagonistSkillUI AobScan failed\n";
		}

		const uintptr_t enginePtr = reinterpret_cast<uintptr_t>(UGameEngine::GetEngine());
		if (GProtagonistSkillUiOffset != 0 && enginePtr != 0)
		{
			unsigned char code[sizeof(kProtagonistSkillUiTrampolineTemplate)];
			std::memcpy(code, kProtagonistSkillUiTrampolineTemplate, sizeof(code));
			std::memcpy(code + kProtagonistSkillUiEngineImm64Offset, &enginePtr, sizeof(enginePtr));

			uint32_t hookId = UINT32_MAX;
			if (InlineHook::HookManager::InstallHook(
				"JH-Win64-Shipping.exe",
				static_cast<uint32_t>(GProtagonistSkillUiOffset),
				code, sizeof(code), hookId,
				true))
			{
				GProtagonistSkillUiHookId = hookId;
			}
			else
			{
				LOGE_STREAM("Tab4Social") << "[SDK] ProtagonistSkillUI hook install failed\n";
			}
		}
		else if (enginePtr == 0)
		{
			LOGE_STREAM("Tab4Social") << "[SDK] ProtagonistSkillUI engine pointer unavailable\n";
		}
	}

	LOGI_STREAM("Tab4Social") << "[SDK] NpcIgnoreWeaponLimit enabled\n";
}

void DisableNpcIgnoreWeaponLimit()
{
	const unsigned char jne[] = { 0x75 };
	if (GIgnoreWeaponLimitsAddr) InlineHook::HookManager::WriteMemory(GIgnoreWeaponLimitsAddr, jne, 1);
	if (GIgnoreSkillLimitsAddr) InlineHook::HookManager::WriteMemory(GIgnoreSkillLimitsAddr, jne, 1);
	if (GProtagonistSkillUiHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GProtagonistSkillUiHookId);
		GProtagonistSkillUiHookId = UINT32_MAX;
	}
	LOGI_STREAM("Tab4Social") << "[SDK] NpcIgnoreWeaponLimit disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  强制显示NPC互动 — SDK DataTable approach
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// TArray<FFunctionSetting> in-memory layout (16 bytes):
//   uint8* Data;   // +0x00  pointer to element array
//   int32  Num;    // +0x08  element count
//   int32  Max;    // +0x0C  allocated capacity
// FFunctionSetting is 0x20 bytes: ENPCFunction(uint8) +7pad +FText(0x18)

// ENPCFunction values we want to force-add
static constexpr uint8 kForcedFunctions[] = {
	1,   // Talk      对话
	12,  // Observe   观察
	2,   // Deal      交易
	3,   // MakeEquip 锻造
	8,   // ZhiYi     制衣
	5,   // MakeDrug  炼丹
};
static constexpr int kForcedCount = sizeof(kForcedFunctions) / sizeof(kForcedFunctions[0]);

// Saved original TArray header per row pointer
struct FTArrayBackup {
	uint8* OrigData;
	int32  OrigNum;
	int32  OrigMax;
};
static std::unordered_map<uint8*, FTArrayBackup> GNpcFuncBackups;
// Allocated blocks to free on disable
static std::vector<uint8*> GNpcFuncAllocations;

void EnableForceNpcInteraction()
{
	// 1. Get NPCResManager (returns UNPCResManager* but we treat as UObject*)
	UObject* ResMgr = static_cast<UObject*>(UManagerFuncLib::GetNPCResManager());
	if (!ResMgr || !IsSafeLiveObject(ResMgr))
	{
		LOGE_STREAM("Tab4Social") << "[SDK] ForceNpcInteraction: GetNPCResManager failed\n";
		return;
	}

	// 2. Get NPCTable (UDataTable* at +0x38 of UNPCResManager)
	UDataTable* NPCTable = *reinterpret_cast<UDataTable**>(
		reinterpret_cast<uint8*>(ResMgr) + 0x38);
	if (!NPCTable || !IsSafeLiveObject(static_cast<UObject*>(NPCTable)))
	{
		LOGE_STREAM("Tab4Social") << "[SDK] ForceNpcInteraction: NPCTable null\n";
		return;
	}

	// 3. Iterate RowMap
	auto& RowMap = NPCTable->RowMap;
	if (!RowMap.IsValid())
	{
		LOGE_STREAM("Tab4Social") << "[SDK] ForceNpcInteraction: RowMap invalid\n";
		return;
	}
	const int32 AllocatedSlots = RowMap.NumAllocated();
	int32 ModifiedCount = 0;

	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		if (!RowMap.IsValidIndex(i))
			continue;
		uint8* RowData = RowMap[i].Value();
		if (!RowData)
			continue;

		// FNPCSetting.Functions is TArray<FFunctionSetting> at offset +0xE0
		uint8* FuncsPtr = RowData + 0xE0;
		uint8*& Data = *reinterpret_cast<uint8**>(FuncsPtr + 0x00);
		int32&  Num  = *reinterpret_cast<int32*>(FuncsPtr + 0x08);
		int32&  Max  = *reinterpret_cast<int32*>(FuncsPtr + 0x0C);

		// Save original if not already saved
		if (GNpcFuncBackups.find(RowData) == GNpcFuncBackups.end())
		{
			GNpcFuncBackups[RowData] = { Data, Num, Max };
		}

		// Collect existing function types (simple array scan, no std::set needed)
		uint8 ExistingTypes[64] = {};
		int32 ExistingCount = 0;
		if (Data && Num > 0 && Num < 200)
		{
			for (int32 j = 0; j < Num && ExistingCount < 64; ++j)
			{
				ExistingTypes[ExistingCount++] = *(Data + j * 0x20); // ENPCFunction at +0x00
			}
		}

		// Determine how many new types to add
		auto HasType = [&](uint8 t) {
			for (int32 x = 0; x < ExistingCount; ++x)
				if (ExistingTypes[x] == t) return true;
			return false;
		};
		uint8 ToAdd[kForcedCount] = {};
		int32 ToAddCount = 0;
		for (int k = 0; k < kForcedCount; ++k)
		{
			if (!HasType(kForcedFunctions[k]))
				ToAdd[ToAddCount++] = kForcedFunctions[k];
		}

		if (ToAddCount == 0)
			continue; // NPC already has all types

		// Allocate new array: original entries + new entries
		int32 NewNum = Num + ToAddCount;
		uint8* NewData = static_cast<uint8*>(VirtualAlloc(
			nullptr, NewNum * 0x20, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (!NewData)
			continue;
		GNpcFuncAllocations.push_back(NewData);

		// Copy original entries
		if (Data && Num > 0)
			memcpy(NewData, Data, Num * 0x20);

		// Append new entries (zero-init then set Function type)
		for (int32 a = 0; a < ToAddCount; ++a)
		{
			uint8* Entry = NewData + (Num + a) * 0x20;
			memset(Entry, 0, 0x20);
			*Entry = ToAdd[a]; // ENPCFunction at offset 0
		}

		// Repoint TArray
		Data = NewData;
		Num  = NewNum;
		Max  = NewNum;
		ModifiedCount++;
	}

	LOGI_STREAM("Tab4Social") << "[SDK] ForceNpcInteraction enabled: modified "
		<< ModifiedCount << " NPCs\n";
}

void DisableForceNpcInteraction()
{
	// Restore all saved TArray headers
	for (auto& Pair : GNpcFuncBackups)
	{
		uint8* RowData = Pair.first;
		const auto& Bak = Pair.second;
		uint8* FuncsPtr = RowData + 0xE0;
		*reinterpret_cast<uint8**>(FuncsPtr + 0x00) = Bak.OrigData;
		*reinterpret_cast<int32*>(FuncsPtr + 0x08)  = Bak.OrigNum;
		*reinterpret_cast<int32*>(FuncsPtr + 0x0C)  = Bak.OrigMax;
	}
	int32 Restored = static_cast<int32>(GNpcFuncBackups.size());
	GNpcFuncBackups.clear();

	// Free allocated blocks
	for (uint8* Block : GNpcFuncAllocations)
		VirtualFree(Block, 0, MEM_RELEASE);
	GNpcFuncAllocations.clear();

	LOGI_STREAM("Tab4Social") << "[SDK] ForceNpcInteraction disabled: restored "
		<< Restored << " NPCs\n";
}
