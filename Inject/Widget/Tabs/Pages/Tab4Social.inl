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
uintptr_t GSongLi2Addr = 0;
uint32_t GSongLiQualityHookId = UINT32_MAX;
uintptr_t GSongLiQualityOffset = 0;
volatile uint8_t GSongLiQualityValue = 5;
volatile LONG GSongLiStrongEnoughFlag = 0;

const char* kSongLi1Pattern = "E8 ?? ?? ?? ?? C1 E8 1F 34 01";
const char* kSongLiQualityPattern = "88 87 80 00 00 00 0F B6 83";

const unsigned char kSongLiQualityTrampolineTemplate[] = {
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&flag)
	0x41, 0xC7, 0x03, 0x01, 0x00, 0x00, 0x00,         // mov dword [r11], 1
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&quality)
	0x41, 0x0F, 0xB6, 0x1B,                           // movzx ebx, byte [r11]
	0x38, 0xD8,                                       // cmp al, bl
	0x7D, 0x02,                                       // jge +2
	0x88, 0xD8,                                       // mov al, bl
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
uintptr_t GConsultCanSelect2Addr = 0;
uintptr_t GConsultCanSelect3Addr = 0;

const char* kVigorIsMetPattern = "84 C0 74 08 F3 0F 10 ?? ?? ?? ?? ?? 0F 2F ?? 73";
const char* kPlayerFriendlinessIsMetPattern = "7E ?? 0F 2F ?? 73 ?? 48 8D";
const char* kConsultRequirementsIsMetPattern = "41 8B F8 8B F2 48 8B D9 E8 ?? ?? ?? ?? 84 C0 74";
const char* kConsultCanSelect2Pattern = "FF 90 ?? ?? 00 00 83 F8 01 74 05";
const char* kConsultCanSelect3Pattern = "FF 90 ?? ?? 00 00 83 F8 01 74 ?? 48 8D";

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
const char* kIgnoreWeaponLimitsPattern = "0F 44 ?? ?? FF ?? 48 ?? ?? 75 ?? 40 84 ?? 75 ??";
const char* kIgnoreSkillLimitsPattern = "49 8B 4F 70 E8 ?? ?? ?? ?? 3C 04 75 75";

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
		hookId))
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
	LOGI_STREAM("Tab4Social") << "[SDK] NpcIgnoreWeaponLimit enabled\n";
}

void DisableNpcIgnoreWeaponLimit()
{
	const unsigned char jne[] = { 0x75 };
	if (GIgnoreWeaponLimitsAddr) InlineHook::HookManager::WriteMemory(GIgnoreWeaponLimitsAddr, jne, 1);
	if (GIgnoreSkillLimitsAddr) InlineHook::HookManager::WriteMemory(GIgnoreSkillLimitsAddr, jne, 1);
	LOGI_STREAM("Tab4Social") << "[SDK] NpcIgnoreWeaponLimit disabled\n";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  强制显示NPC互动 (placeholder - complex)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void EnableForceNpcInteraction()
{
	LOGI_STREAM("Tab4Social") << "[SDK] ForceNpcInteraction: not yet implemented (complex feature)\n";
}

void DisableForceNpcInteraction()
{
	LOGI_STREAM("Tab4Social") << "[SDK] ForceNpcInteraction: not yet implemented (complex feature)\n";
}
