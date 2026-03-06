void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->InputSlot, "Tab2(InputSlot)");
	if (!Container) return;
	Container->ClearChildren();

	GTab2.DamageBoostToggle = nullptr;
	GTab2.DamageFriendlyOnlyToggle = nullptr;
	GTab2.SkillNoCooldownToggle = nullptr;
	GTab2.NoEncounterToggle = nullptr;
	GTab2.AllTeammatesInFightToggle = nullptr;
	GTab2.DefeatAsVictoryToggle = nullptr;
	GTab2.NeiGongFillLastSlotToggle = nullptr;
	GTab2.AutoRecoverHpMpToggle = nullptr;
	GTab2.TotalMoveSpeedToggle = nullptr;
	GTab2.DamageMultiplierSlider = nullptr;
	GTab2.MoveSpeedMultiplierSlider = nullptr;

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

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) -> UBPVE_JHConfigVideoItem2_C* {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
			return Item;
		}
		return nullptr;
	};
	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) -> UBPVE_JHConfigVolumeItem2_C* {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			bool hasRememberedValue = false;
			float rememberedValue = 2.0f;
			if (Title && Title[0] != L'\0')
			{
				std::wstring titleKey(Title);
				while (!titleKey.empty() && (titleKey.front() == L' ' || titleKey.front() == L'\t' || titleKey.front() == L'\r' || titleKey.front() == L'\n'))
					titleKey.erase(titleKey.begin());
				while (!titleKey.empty() && (titleKey.back() == L' ' || titleKey.back() == L'\t' || titleKey.back() == L'\r' || titleKey.back() == L'\n'))
					titleKey.pop_back();
				const auto it = GUIRememberState.SliderValueByTitle.find(titleKey);
				hasRememberedValue = (it != GUIRememberState.SliderValueByTitle.end());
				if (hasRememberedValue)
					rememberedValue = it->second;
			}

			if (Item->VolumeSlider)
			{
				Item->VolumeSlider->MinValue = 0.0f;
				Item->VolumeSlider->MaxValue = 10.0f;
				Item->VolumeSlider->StepSize = 0.1f;
				if (hasRememberedValue)
				{
					if (rememberedValue < 0.0f) rememberedValue = 0.0f;
					if (rememberedValue > 10.0f) rememberedValue = 10.0f;
					Item->VolumeSlider->SetValue(rememberedValue);
				}
			}
			if (Item->TXT_CurrentValue && Item->VolumeSlider)
			{
				float value = Item->VolumeSlider->GetValue();
				if (value < 0.0f) value = 0.0f;
				if (value > 10.0f) value = 10.0f;
				wchar_t buf[16] = {};
				swprintf_s(buf, 16, L"%.1f", static_cast<double>(value));
				Item->TXT_CurrentValue->SetText(MakeText(buf));
			}

			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
			return Item;
		}
		return nullptr;
	};

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"战斗开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	GTab2.SkillNoCooldownToggle = AddToggle(SwitchBox, L"招式无视冷却");
	GTab2.DamageBoostToggle = AddToggle(SwitchBox, L"战斗加速");
	GTab2.DamageMultiplierSlider = AddSlider(SwitchBox, L"战斗加速倍数");
	GTab2.NoEncounterToggle = AddToggle(SwitchBox, L"不遇敌");
	GTab2.AllTeammatesInFightToggle = AddToggle(SwitchBox, L"全队友参战");
	GTab2.DefeatAsVictoryToggle = AddToggle(SwitchBox, L"战败视为胜利");
	GTab2.NeiGongFillLastSlotToggle = AddToggle(SwitchBox, L"心法填装最后一格");
	GTab2.AutoRecoverHpMpToggle = AddToggle(SwitchBox, L"战斗前自动恢复气血和真气");
	GTab2.TotalMoveSpeedToggle = AddToggle(SwitchBox, L"总移动速度加倍");
	GTab2.MoveSpeedMultiplierSlider = AddSlider(SwitchBox, L"移动倍数");
	GTab2.DamageFriendlyOnlyToggle = AddToggle(SwitchBox, L"只对本方生效");
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);
}

namespace
{
	uint32_t GTab2UseSkillHookId = UINT32_MAX;
	uint32_t GTab2SkillNoCDHookId = UINT32_MAX;
	uint32_t GTab2BattleSpeedHook1Id = UINT32_MAX;
	uint32_t GTab2BattleSpeedHook2Id = UINT32_MAX;
	uint32_t GTab2AllInFightHook1Id = UINT32_MAX;
	uint32_t GTab2AllInFightHook2Id = UINT32_MAX;
	uint32_t GTab2AllInFightHook3Id = UINT32_MAX;
	uint32_t GTab2AllInFightHook4Id = UINT32_MAX;
	uint32_t GTab2AllInFightHook5Id = UINT32_MAX;
	uint32_t GTab2DefeatAsVictoryHookId = UINT32_MAX;
	uint32_t GTab2NeiGongFillLastSlotHookId = UINT32_MAX;
	uint32_t GTab2AutoRecoverHpMpHookId = UINT32_MAX;
	uint32_t GTab2TotalMoveSpeedHookId = UINT32_MAX;
	uintptr_t GTab2UseSkillOffset = 0;
	uintptr_t GTab2SkillNoCDOffset = 0;
	uintptr_t GTab2AllInFightOffset1 = 0;
	uintptr_t GTab2AllInFightOffset2 = 0;
	uintptr_t GTab2AllInFightOffset3 = 0;
	uintptr_t GTab2AllInFightOffset4 = 0;
	uintptr_t GTab2AllInFightOffset5 = 0;
	uintptr_t GTab2DefeatAsVictoryOffset = 0;
	uintptr_t GTab2NeiGongFillLastSlotOffset = 0;
	uintptr_t GTab2AutoRecoverHpMpOffset = 0;
	uintptr_t GTab2TotalMoveSpeedOffset = 0;
	volatile LONG GTab2SkillNoCooldownFlag = 0;
	volatile LONG GTab2BattleSpeedHookEnabled = 0;
	volatile LONG GTab2TotalMoveSpeedFriendlyOnly = 1;
	float GTab2BattleSpeedHookMultiplier = 2.0f;
	float GTab2TotalMoveSpeedHookMultiplier = 2.0f;
	bool GTab2BattleSpeedAutoAppliedInFight = false;
	float GTab2BattleSpeedLastAppliedMultiplier = 0.0f;
	DWORD GTab2BattleSpeedLastAutoApplyTick = 0;
	uintptr_t GTab2OwnerTeamInfoOffset = 0x1408;
	uintptr_t GTab2NoEncounterPatchAddr = 0;
	unsigned char GTab2NoEncounterOriginalBytes[2] = { 0x0F, 0x84 };
	bool GTab2NoEncounterOriginalCaptured = false;
	bool GTab2AutoRecoverHpMpEnabled = false;
	bool GTab2AutoRecoverHpMpAppliedInFight = false;
	uintptr_t GTab2AddNeiGongNoLimitPatchAddr = 0;
	uint8_t GTab2AddNeiGongNoLimitOriginalByte = 0x02;
	bool GTab2AddNeiGongNoLimitOriginalCaptured = false;
	uintptr_t GTab2NeiGongLimitAddr = 0;
	int32 GTab2NeiGongLimitOriginalValue = 6;
	bool GTab2NeiGongLimitOriginalCaptured = false;

	constexpr uint32_t kTab2BattleSpeedHookOffset1 = 0x100D67B;
	constexpr uint32_t kTab2BattleSpeedHookOffset2 = 0x100D73B;
	constexpr int32 kTab2BattleModuleAnchorIndex = 52869;
	constexpr int32 kTab2BattleModuleSearchRadius = 10;

	const char* kTab2UseSkillPattern = "48 8B ? 48 8B ? 8B 12 E8 ? ? ? ? 48 85 C0";
	const char* kTab2SkillNoCDPattern = "8B 80 DC 01 00 00 FF C8 83 F8 05 77 ? 48 8D 15";
	const char* kTab2JHASCFieldPattern = "48 8B B9 ? ? 00 00 0F B6 F2 48 8B";
	const char* kTab2NoEncounterPattern = "? 8B EA 4C 8B F1 48 85 D2 0F 84 ? ? 00 00 E8";
	const char* kTab2DefeatAsVictoryPattern = "41 55 41 56 41 57 48 83 EC 50 44 0F B6 ? 48 8B";
	const char* kTab2NeiGongFillLastSlotPattern = "49 FF C0 48 83 C0 18";
	const char* kTab2AddNeiGongNoLimitPattern = "41 83 F8 02 7E ? 41 B8 02 00 00 00";
	const char* kTab2NeiGongLimitRefPattern = "41 8B CD 44 39 2D";
	const char* kTab2AllInFightPattern1 = "49 63 85 08 01 00 00 48 8D";
	const char* kTab2AllInFightPattern2 = "49 8D B7 00 01 00 00 48 89 75";
	const char* kTab2AllInFightPattern3 = "83 E8 01 49 8B 3A";
	const char* kTab2AllInFightPattern4 = "49 8B 0C 04 83 79 38 00";
	const char* kTab2AllInFightPattern5 = "80 BD ? ? 00 00 05";
	const char* kTab2AutoRecoverHpMpPattern = "48 8B ? 80 ? ? ? 00 00 03 75";
	const char* kTab2TotalMoveSpeedPattern = "74 ? F3 0F 10 ? ? ? ? ? 48 83 C4 ? ? C3 F3 0F 10 ? ? ? ? ? 48 83 C4 ? ? C3 F3 0F 10";

	const unsigned char kTab2UseSkillHookTemplate[] = {
		0x49, 0x89, 0xCA,
		0x50,
		0x51,
		0x52,
		0x41, 0x50,
		0x41, 0x51,
		0x41, 0x52,
		0x41, 0x53,
		0x53,
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0x41, 0xC7, 0x03, 0x00, 0x00, 0x00, 0x00,
		0x48, 0x83, 0xEC, 0x20,
		0x4C, 0x89, 0xD1,
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0x41, 0xFF, 0xD3,
		0x48, 0x83, 0xC4, 0x20,
		0x5B,
		0x41, 0x5B,
		0x41, 0x5A,
		0x41, 0x59,
		0x41, 0x58,
		0x5A,
		0x59,
		0x58
	};
	constexpr size_t kTab2UseSkillFlagImm64Offset = 17;
	constexpr size_t kTab2UseSkillHelperImm64Offset = 41;

	const unsigned char kTab2SkillNoCDHookTemplate[] = {
		0x8B, 0x80, 0xDC, 0x01, 0x00, 0x00,
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0x41, 0x83, 0x3B, 0x01,
		0x75, 0x05,
		0xB8, 0x01, 0x00, 0x00, 0x00
	};
	constexpr size_t kTab2SkillNoCDFlagImm64Offset = 8;

	const unsigned char kTab2BattleSpeedHookTemplate[] = {
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0x41, 0x83, 0x3B, 0x00,
		0x74, 0x0F,
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0xF3, 0x41, 0x0F, 0x10, 0x33
	};
	constexpr size_t kTab2BattleSpeedFlagImm64Offset = 2;
	constexpr size_t kTab2BattleSpeedValueImm64Offset = 18;

	const unsigned char kTab2AutoRecoverHpMpHookTemplate[] = {
		0x50,
		0x51,
		0x52,
		0x41, 0x50,
		0x41, 0x51,
		0x41, 0x52,
		0x41, 0x53,
		0x48, 0x83, 0xEC, 0x20,
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0x41, 0xFF, 0xD3,
		0x48, 0x83, 0xC4, 0x20,
		0x41, 0x5B,
		0x41, 0x5A,
		0x41, 0x59,
		0x41, 0x58,
		0x5A,
		0x59,
		0x58
	};
	constexpr size_t kTab2AutoRecoverHpMpHelperImm64Offset = 17;

	const unsigned char kTab2TotalMoveSpeedHookTemplate[] = {
		0x41, 0x50,
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0x41, 0x83, 0x3B, 0x01,
		0x75, 0x16,
		0x4C, 0x8B, 0x83, 0x30, 0x01, 0x00, 0x00,
		0x4C, 0x3B, 0xC4,
		0x76, 0x19,
		0x41, 0x83, 0xB8, 0xC0, 0x04, 0x00, 0x00, 0x00,
		0x75, 0x0F,
		0x49, 0xBB,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
		0xF3, 0x41, 0x0F, 0x59, 0x03,
		0x41, 0x58
	};
	constexpr size_t kTab2TotalMoveSpeedFlagImm64Offset = 4;
	constexpr size_t kTab2TotalMoveSpeedMultiplierImm64Offset = 42;

	const unsigned char kTab2DefeatAsVictoryHookTemplate[] = {
		0xB2, 0x00,
		0x41, 0x55,
		0x41, 0x56,
		0x41, 0x57
	};

	const unsigned char kTab2NeiGongFillLastSlotHookTemplate[] = {
		0x49, 0xFF, 0xC0,
		0x48, 0x83, 0xC0, 0x18,
		0x52,
		0x44, 0x89, 0xC2,
		0xFF, 0xC2,
		0x39, 0xCA,
		0x5A,
		0x75, 0x12,
		0xC7, 0x00, 0x01, 0x00, 0x00, 0x00,
		0xC7, 0x40, 0x18, 0x01, 0x00, 0x00, 0x00,
		0x89, 0x48, 0x1C,
		0xFF, 0xC1
	};

	const unsigned char kTab2AllInFightHookTemplate1[] = {
		0x49, 0x8B, 0x8D, 0xF0, 0x00, 0x00, 0x00,
		0x49, 0x63, 0x85, 0xF8, 0x00, 0x00, 0x00
	};

	const unsigned char kTab2AllInFightHookTemplate2[] = {
		0x41, 0x8B, 0xB7, 0xF8, 0x00, 0x00, 0x00,
		0xFF, 0xCE,
		0x89, 0x74, 0x24, 0x58,
		0x89, 0x74, 0x24, 0x5C,
		0x49, 0x8D, 0xB7, 0xF0, 0x00, 0x00, 0x00
	};

	const unsigned char kTab2AllInFightHookTemplate3[] = {
		0x50,
		0x52,
		0x53,
		0x48, 0x8B, 0x16,
		0x4A, 0x8B, 0x1C, 0x22,
		0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x19,
		0x44, 0x89, 0xF0,
		0x48, 0x31, 0xD2,
		0xBB, 0x04, 0x00, 0x00, 0x00,
		0xF7, 0xF3,
		0x48, 0x89, 0xD0,
		0x4D, 0x8D, 0x14, 0xC1,
		0x49, 0x8B, 0x3A,
		0xEB, 0x03,
		0x48, 0x31, 0xFF,
		0x5B,
		0x5A,
		0x58,
		0x83, 0xE8, 0x01
	};

	const unsigned char kTab2AllInFightHookTemplate4[] = {
		0x49, 0x8B, 0x0C, 0x04,
		0x48, 0x83, 0x7C, 0x24, 0x50, 0x00,
		0x75, 0x04,
		0x39, 0xC0,
		0xEB, 0x04,
		0x83, 0x79, 0x38, 0x00
	};

uintptr_t ScanPatternInModuleRange(const char* moduleName, const char* pattern, uintptr_t rangeStart, uintptr_t rangeSize)
	{
		if (!moduleName || !pattern || rangeStart == 0 || rangeSize == 0)
			return 0;

		HMODULE hModule = GetModuleHandleA(moduleName);
		if (!hModule)
			return 0;

		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(moduleBase);
		if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
			return 0;

		const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(moduleBase + static_cast<uintptr_t>(dos->e_lfanew));
		if (!nt || nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage == 0)
			return 0;

		const uintptr_t moduleEnd = moduleBase + static_cast<uintptr_t>(nt->OptionalHeader.SizeOfImage);
		if (moduleEnd <= moduleBase)
			return 0;

		uintptr_t start = rangeStart;
		uintptr_t end = rangeStart + rangeSize;
		if (end <= start)
			return 0;
		if (start < moduleBase)
			start = moduleBase;
		if (end > moduleEnd)
			end = moduleEnd;
		if (start >= end)
			return 0;

		return InlineHook::HookManager::AobScanFirst(pattern, start, end, false);
	}

	void RecoverHpMpForTeamInfo(UTeamInfo* teamInfo)
	{
		if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(teamInfo), UTeamInfo::StaticClass()))
			return;

		UJHAttributeSet* attrSet = teamInfo->GetAttributeSet();
		if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(attrSet), UJHAttributeSet::StaticClass()))
			return;

		auto RecoverAttrToMax = [](FGameplayAttributeData& Cur, const FGameplayAttributeData& Max)
		{
			float maxValue = Max.CurrentValue;
			if (maxValue <= 0.0001f)
				maxValue = Max.BaseValue;
			if (maxValue < 0.0f)
				maxValue = 0.0f;
			Cur.BaseValue = maxValue;
			Cur.CurrentValue = maxValue;
		};

		RecoverAttrToMax(attrSet->Health, attrSet->MaxHealth);
		RecoverAttrToMax(attrSet->Mana, attrSet->MaxMana);
	}

	void RecoverFriendlyTeamHpMpNow()
	{
		UTeamManager* teamMgr = UManagerFuncLib::GetTeamManager();
		if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(teamMgr), UTeamManager::StaticClass()))
			return;

		auto RecoverArray = [](const TArray<UTeamInfo*>& arr)
		{
			for (int32 i = 0; i < arr.Num(); ++i)
			{
				UTeamInfo* teamInfo = arr[i];
				if (!teamInfo)
					continue;
				RecoverHpMpForTeamInfo(teamInfo);
			}
		};

		if (teamMgr->TeamInfos.IsValid())
			RecoverArray(teamMgr->TeamInfos);
		if (teamMgr->FightTeamInfos.IsValid())
			RecoverArray(teamMgr->FightTeamInfos);
	}

	bool IsFightWorldState(EWorldStateType state)
	{
		return state == EWorldStateType::IntoFight || state == EWorldStateType::Fighting;
	}

	void TickAutoRecoverHpMpBySDK()
	{
		const EWorldStateType worldState = UManagerFuncLib::GetWorldType();
		const bool inFight = IsFightWorldState(worldState);
		if (!inFight)
		{
			GTab2AutoRecoverHpMpAppliedInFight = false;
			return;
		}
		if (!GTab2AutoRecoverHpMpEnabled)
			return;
		if (GTab2AutoRecoverHpMpAppliedInFight)
			return;

		RecoverFriendlyTeamHpMpNow();
		GTab2AutoRecoverHpMpAppliedInFight = true;
		LOGI_STREAM("Tab2Battle") << "[SDK] AutoRecoverHpMp applied by SDK at fight-enter\n";
	}

	void __fastcall AutoRecoverHpMpHookCallback()
	{
		RecoverFriendlyTeamHpMpNow();
	}

	bool IsReadablePointerAddress(uintptr_t address, size_t size = sizeof(uintptr_t))
	{
		if (address == 0 || size == 0)
			return false;

		MEMORY_BASIC_INFORMATION mbi{};
		if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0)
			return false;
		if (mbi.State != MEM_COMMIT)
			return false;

		const DWORD prot = (mbi.Protect & 0xFFu);
		const bool readable =
			prot == PAGE_READONLY ||
			prot == PAGE_READWRITE ||
			prot == PAGE_WRITECOPY ||
			prot == PAGE_EXECUTE_READ ||
			prot == PAGE_EXECUTE_READWRITE ||
			prot == PAGE_EXECUTE_WRITECOPY;
		if (!readable)
			return false;

		const uintptr_t start = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
		const uintptr_t end = start + static_cast<uintptr_t>(mbi.RegionSize);
		if (end <= start)
			return false;

		return (address >= start) && ((address + size) <= end);
	}

	bool TryReadPointerValue(uintptr_t address, uintptr_t& outValue)
	{
		outValue = 0;
		if (!IsReadablePointerAddress(address))
			return false;
		return InlineHook::HookManager::ReadValue(address, outValue);
	}

	UTeamInfo* ResolveOwnerTeamInfoFromUseSkillContext(uintptr_t contextPtr)
	{
		if (contextPtr == 0)
			return nullptr;

		auto TryFromASC = [](uintptr_t ascPtr) -> UTeamInfo*
		{
			if (ascPtr == 0)
				return nullptr;

			auto* ascObj = reinterpret_cast<UObject*>(ascPtr);
			if (!IsSafeLiveObjectOfClass(ascObj, UJHAbilitySystemComponent::StaticClass()))
				return nullptr;

			uintptr_t ownerPtr = 0;
			if (!TryReadPointerValue(ascPtr + GTab2OwnerTeamInfoOffset, ownerPtr) || ownerPtr == 0)
				return nullptr;

			auto* ownerObj = reinterpret_cast<UObject*>(ownerPtr);
			if (!IsSafeLiveObjectOfClass(ownerObj, UTeamInfo::StaticClass()))
				return nullptr;

			return static_cast<UTeamInfo*>(ownerObj);
		};

		auto* directOwner = TryFromASC(contextPtr);
		if (directOwner)
			return directOwner;

		constexpr uintptr_t kPossibleASCOffsets[] = { 0x290, 0x298, 0x518, 0x540 };
		for (const uintptr_t offset : kPossibleASCOffsets)
		{
			uintptr_t ascPtr = 0;
			if (!TryReadPointerValue(contextPtr + offset, ascPtr) || ascPtr == 0)
				continue;
			auto* owner = TryFromASC(ascPtr);
			if (owner)
				return owner;
		}

		return nullptr;
	}

	bool IsFriendlyOwnerTeamInfo(UTeamInfo* ownerTeamInfo)
	{
		if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(ownerTeamInfo), UTeamInfo::StaticClass()))
			return false;

		UTeamManager* teamMgr = UManagerFuncLib::GetTeamManager();
		if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(teamMgr), UTeamManager::StaticClass()))
			return false;

		if (!teamMgr->TeamInfos.IsValid())
			return false;

		for (int32 i = 0; i < teamMgr->TeamInfos.Num(); ++i)
		{
			if (teamMgr->TeamInfos[i] == ownerTeamInfo)
				return true;
		}

		return false;
	}

	void __fastcall UpdateSkillNoCooldownFlagFromUseSkillCtx(uintptr_t contextPtr)
	{
		LONG newFlag = 0;
		UTeamInfo* ownerTeamInfo = ResolveOwnerTeamInfoFromUseSkillContext(contextPtr);
		if (ownerTeamInfo && IsFriendlyOwnerTeamInfo(ownerTeamInfo))
			newFlag = 1;

		InterlockedExchange(&GTab2SkillNoCooldownFlag, newFlag);
	}

	UJHNeoUIBattleModuleView* FindLiveBattleModuleViewForSpeed()
	{
		auto* ObjArray = UObject::GObjects.GetTypedPtr();
		if (!ObjArray)
			return nullptr;

		UJHNeoUIBattleModuleView* fallback = nullptr;
		int32 fallbackIndex = -1;
		const int32 num = ObjArray->Num();
		if (num <= 0)
			return nullptr;

		int32 startIdx = kTab2BattleModuleAnchorIndex - kTab2BattleModuleSearchRadius;
		int32 endIdx = kTab2BattleModuleAnchorIndex + kTab2BattleModuleSearchRadius;
		if (startIdx < 0)
			startIdx = 0;
		if (endIdx >= num)
			endIdx = num - 1;

		for (int32 i = startIdx; i <= endIdx; ++i)
		{
			UObject* obj = ObjArray->GetByIndex(i);
			if (!IsSafeLiveObjectOfClass(obj, UJHNeoUIBattleModuleView::StaticClass()))
				continue;
			if (obj->IsDefaultObject())
				continue;

			auto* view = static_cast<UJHNeoUIBattleModuleView*>(obj);
			if (!fallback)
			{
				fallback = view;
				fallbackIndex = i;
			}

			if (view->IsInViewport())
			{
				static uintptr_t sLastLoggedPtr = 0;
				static int32 sLastLoggedIndex = -1;
				const uintptr_t ptr = reinterpret_cast<uintptr_t>(view);
				if (ptr != sLastLoggedPtr || i != sLastLoggedIndex)
				{
					sLastLoggedPtr = ptr;
					sLastLoggedIndex = i;
					LOGI_STREAM("Tab2Battle") << "[SDK] BattleModuleView selected(inViewport): ptr=0x"
						<< std::hex << ptr << std::dec << " index=" << i
						<< " window=[" << startIdx << "," << endIdx << "]\n";
				}
				return view;
			}
		}

		if (fallback)
		{
			static uintptr_t sLastFallbackPtr = 0;
			static int32 sLastFallbackIndex = -1;
			const uintptr_t ptr = reinterpret_cast<uintptr_t>(fallback);
			if (ptr != sLastFallbackPtr || fallbackIndex != sLastFallbackIndex)
			{
				sLastFallbackPtr = ptr;
				sLastFallbackIndex = fallbackIndex;
				LOGI_STREAM("Tab2Battle") << "[SDK] BattleModuleView selected(fallback): ptr=0x"
					<< std::hex << ptr << std::dec << " index=" << fallbackIndex
					<< " window=[" << startIdx << "," << endIdx << "]\n";
			}
		}
		else
		{
			static int32 sLastMissStart = -1;
			static int32 sLastMissEnd = -1;
			if (sLastMissStart != startIdx || sLastMissEnd != endIdx)
			{
				sLastMissStart = startIdx;
				sLastMissEnd = endIdx;
				LOGI_STREAM("Tab2Battle") << "[SDK] BattleModuleView window miss: ["
					<< startIdx << "," << endIdx << "]\n";
			}
		}

		return fallback;
	}

	bool TryAutoApplyBattleSpeedIfNeeded(bool ForceApply)
	{
		if (InterlockedCompareExchange(&GTab2BattleSpeedHookEnabled, 0, 0) == 0)
		{
			GTab2BattleSpeedAutoAppliedInFight = false;
			return false;
		}

		const EWorldStateType worldState = UManagerFuncLib::GetWorldType();
		const bool isInFight =
			UManagerFuncLib::IsFighting() ||
			(worldState == EWorldStateType::Fighting) ||
			(worldState == EWorldStateType::IntoFight);
		if (!isInFight)
		{
			GTab2BattleSpeedAutoAppliedInFight = false;
			return false;
		}

		const DWORD now = GetTickCount();
		if (!ForceApply)
		{
			const bool multiplierChanged = std::fabs(GTab2BattleSpeedLastAppliedMultiplier - GTab2BattleSpeedHookMultiplier) > 0.001f;
			if (GTab2BattleSpeedAutoAppliedInFight && !multiplierChanged)
				return false;
			if (GTab2BattleSpeedLastAutoApplyTick != 0 && (now - GTab2BattleSpeedLastAutoApplyTick) < 500)
				return false;
		}

		UJHNeoUIBattleModuleView* view = FindLiveBattleModuleViewForSpeed();
		if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(view), UJHNeoUIBattleModuleView::StaticClass()))
			return false;

		if (view->BtnSpeedUpFlagIs1())
			view->HandleBtn_SpeedUp_1();
		else
			view->HandleBtn_SpeedUp_2();

		GTab2BattleSpeedAutoAppliedInFight = true;
		GTab2BattleSpeedLastAppliedMultiplier = GTab2BattleSpeedHookMultiplier;
		GTab2BattleSpeedLastAutoApplyTick = now;
		LOGI_STREAM("Tab2Battle") << "[SDK] BattleSpeed auto-apply triggered: "
			<< GTab2BattleSpeedHookMultiplier << "x\n";
		return true;
	}
}

void EnableSkillNoCooldownHooks()
{
	if (GTab2UseSkillHookId != UINT32_MAX && GTab2SkillNoCDHookId != UINT32_MAX)
		return;

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule)
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown failed to get module handle\n";
		return;
	}
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);

	{
		const uintptr_t fieldAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2JHASCFieldPattern);
		if (fieldAddr != 0)
		{
			int32 parsedOffset = 0;
			if (InlineHook::HookManager::ReadValue(fieldAddr + 3, parsedOffset) &&
				parsedOffset > 0 && parsedOffset < 0x4000)
			{
				GTab2OwnerTeamInfoOffset = static_cast<uintptr_t>(parsedOffset);
			}
		}
	}

	if (GTab2UseSkillOffset == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2UseSkillPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown UseSkill AobScan failed\n";
			return;
		}
		GTab2UseSkillOffset = foundAddr - moduleBase;
		LOGI_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown UseSkill found: 0x"
			<< std::hex << foundAddr << ", offset: 0x" << GTab2UseSkillOffset << std::dec << "\n";
	}

	if (GTab2SkillNoCDOffset == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2SkillNoCDPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown SkillNoCD AobScan failed\n";
			return;
		}
		GTab2SkillNoCDOffset = foundAddr - moduleBase;
		LOGI_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown SkillNoCD found: 0x"
			<< std::hex << foundAddr << ", offset: 0x" << GTab2SkillNoCDOffset << std::dec << "\n";
	}

	if (GTab2UseSkillHookId == UINT32_MAX)
	{
		unsigned char useSkillCode[sizeof(kTab2UseSkillHookTemplate)] = {};
		std::memcpy(useSkillCode, kTab2UseSkillHookTemplate, sizeof(useSkillCode));

		const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GTab2SkillNoCooldownFlag);
		const uintptr_t helperAddr = reinterpret_cast<uintptr_t>(&UpdateSkillNoCooldownFlagFromUseSkillCtx);
		std::memcpy(useSkillCode + kTab2UseSkillFlagImm64Offset, &flagAddr, sizeof(flagAddr));
		std::memcpy(useSkillCode + kTab2UseSkillHelperImm64Offset, &helperAddr, sizeof(helperAddr));

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2UseSkillOffset),
			useSkillCode,
			sizeof(useSkillCode),
			hookId))
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown UseSkill hook install failed\n";
			return;
		}

		GTab2UseSkillHookId = hookId;
	}

	if (GTab2SkillNoCDHookId == UINT32_MAX)
	{
		unsigned char skillNoCDCode[sizeof(kTab2SkillNoCDHookTemplate)] = {};
		std::memcpy(skillNoCDCode, kTab2SkillNoCDHookTemplate, sizeof(skillNoCDCode));

		const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GTab2SkillNoCooldownFlag);
		std::memcpy(skillNoCDCode + kTab2SkillNoCDFlagImm64Offset, &flagAddr, sizeof(flagAddr));

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2SkillNoCDOffset),
			skillNoCDCode,
			sizeof(skillNoCDCode),
			hookId,
			false,
			true,
			false))
		{
			if (GTab2UseSkillHookId != UINT32_MAX)
			{
				InlineHook::HookManager::UninstallHook(GTab2UseSkillHookId);
				GTab2UseSkillHookId = UINT32_MAX;
			}
			LOGE_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown SkillNoCD hook install failed\n";
			return;
		}

		GTab2SkillNoCDHookId = hookId;
	}

	InterlockedExchange(&GTab2SkillNoCooldownFlag, 0);
	LOGI_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown hooks enabled, IDs: "
		<< GTab2UseSkillHookId << ", " << GTab2SkillNoCDHookId << "\n";
}

void DisableSkillNoCooldownHooks()
{
	if (GTab2SkillNoCDHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2SkillNoCDHookId);
		GTab2SkillNoCDHookId = UINT32_MAX;
	}

	if (GTab2UseSkillHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2UseSkillHookId);
		GTab2UseSkillHookId = UINT32_MAX;
	}

	InterlockedExchange(&GTab2SkillNoCooldownFlag, 0);
	LOGI_STREAM("Tab2Battle") << "[SDK] SkillNoCooldown hooks disabled\n";
}

void EnableNoEncounterPatch()
{
	if (GTab2NoEncounterPatchAddr == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2NoEncounterPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] NoEncounter AobScan failed, pattern not found\n";
			return;
		}
		GTab2NoEncounterPatchAddr = foundAddr + 9;
		LOGI_STREAM("Tab2Battle") << "[SDK] NoEncounter patch found: base=0x"
			<< std::hex << foundAddr << ", patch=0x" << GTab2NoEncounterPatchAddr << std::dec << "\n";
	}

	if (!GTab2NoEncounterOriginalCaptured)
	{
		unsigned char originalBytes[2] = {};
		if (InlineHook::HookManager::ReadMemory(GTab2NoEncounterPatchAddr, originalBytes, sizeof(originalBytes)))
		{
			GTab2NoEncounterOriginalBytes[0] = originalBytes[0];
			GTab2NoEncounterOriginalBytes[1] = originalBytes[1];
			GTab2NoEncounterOriginalCaptured = true;
		}
	}

	const unsigned char enableBytes[2] = { 0x90, 0xE9 };
	if (!InlineHook::HookManager::WriteMemory(GTab2NoEncounterPatchAddr, enableBytes, sizeof(enableBytes)))
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] NoEncounter patch enable write failed at: 0x"
			<< std::hex << GTab2NoEncounterPatchAddr << std::dec << "\n";
		return;
	}
}

void DisableNoEncounterPatch()
{
	if (GTab2NoEncounterPatchAddr == 0)
		return;

	unsigned char disableBytes[2] = { 0x0F, 0x84 };
	if (GTab2NoEncounterOriginalCaptured)
	{
		disableBytes[0] = GTab2NoEncounterOriginalBytes[0];
		disableBytes[1] = GTab2NoEncounterOriginalBytes[1];
	}
	if (!InlineHook::HookManager::WriteMemory(GTab2NoEncounterPatchAddr, disableBytes, sizeof(disableBytes)))
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] NoEncounter patch disable write failed at: 0x"
			<< std::hex << GTab2NoEncounterPatchAddr << std::dec << "\n";
	}
}

void EnableAutoRecoverHpMpHook()
{
	GTab2AutoRecoverHpMpEnabled = true;
	GTab2AutoRecoverHpMpAppliedInFight = false;

	TickAutoRecoverHpMpBySDK();
	LOGI_STREAM("Tab2Battle") << "[SDK] AutoRecoverHpMp enabled (SDK mode)\n";
}

void DisableAutoRecoverHpMpHook()
{
	GTab2AutoRecoverHpMpEnabled = false;
	GTab2AutoRecoverHpMpAppliedInFight = false;

	if (GTab2AutoRecoverHpMpHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2AutoRecoverHpMpHookId);
		GTab2AutoRecoverHpMpHookId = UINT32_MAX;
	}
	LOGI_STREAM("Tab2Battle") << "[SDK] AutoRecoverHpMp disabled (SDK mode)\n";
}

void SetTotalMoveSpeedMultiplier(float Value)
{
	TickAutoRecoverHpMpBySDK();

	if (Value < 0.0f)
		Value = 0.0f;
	if (Value > 10.0f)
		Value = 10.0f;
	GTab2TotalMoveSpeedHookMultiplier = Value;
}

void SetTotalMoveSpeedFriendlyOnly(bool Enabled)
{
	InterlockedExchange(&GTab2TotalMoveSpeedFriendlyOnly, Enabled ? 1 : 0);
}

void EnableTotalMoveSpeedHook()
{
	if (GTab2TotalMoveSpeedHookId != UINT32_MAX)
		return;

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule)
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] TotalMoveSpeed failed to get module handle\n";
		return;
	}
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);

	if (GTab2TotalMoveSpeedOffset == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2TotalMoveSpeedPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] TotalMoveSpeed AobScan failed\n";
			return;
		}
		GTab2TotalMoveSpeedOffset = (foundAddr + 0x10) - moduleBase;
		LOGI_STREAM("Tab2Battle") << "[SDK] TotalMoveSpeed found: base=0x"
			<< std::hex << foundAddr << ", hook=0x" << (moduleBase + GTab2TotalMoveSpeedOffset)
			<< " (+0x10)" << std::dec << "\n";
	}

	unsigned char hookCode[sizeof(kTab2TotalMoveSpeedHookTemplate)] = {};
	std::memcpy(hookCode, kTab2TotalMoveSpeedHookTemplate, sizeof(hookCode));
	const uintptr_t flagAddr = reinterpret_cast<uintptr_t>(&GTab2TotalMoveSpeedFriendlyOnly);
	const uintptr_t multiplierAddr = reinterpret_cast<uintptr_t>(&GTab2TotalMoveSpeedHookMultiplier);
	std::memcpy(hookCode + kTab2TotalMoveSpeedFlagImm64Offset, &flagAddr, sizeof(flagAddr));
	std::memcpy(hookCode + kTab2TotalMoveSpeedMultiplierImm64Offset, &multiplierAddr, sizeof(multiplierAddr));

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GTab2TotalMoveSpeedOffset),
		hookCode,
		sizeof(hookCode),
		hookId))
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] TotalMoveSpeed hook install failed\n";
		return;
	}

	GTab2TotalMoveSpeedHookId = hookId;
	LOGI_STREAM("Tab2Battle") << "[SDK] TotalMoveSpeed hook enabled, id=" << GTab2TotalMoveSpeedHookId << "\n";
}

void DisableTotalMoveSpeedHook()
{
	if (GTab2TotalMoveSpeedHookId == UINT32_MAX)
		return;

	InlineHook::HookManager::UninstallHook(GTab2TotalMoveSpeedHookId);
	GTab2TotalMoveSpeedHookId = UINT32_MAX;
	LOGI_STREAM("Tab2Battle") << "[SDK] TotalMoveSpeed hook disabled\n";
}

void EnableDefeatAsVictoryHook()
{
	if (GTab2DefeatAsVictoryHookId != UINT32_MAX)
		return;

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule)
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] DefeatAsVictory failed to get module handle\n";
		return;
	}
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);

	if (GTab2DefeatAsVictoryOffset == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2DefeatAsVictoryPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] DefeatAsVictory AobScan failed\n";
			return;
		}
		GTab2DefeatAsVictoryOffset = foundAddr - moduleBase;
		LOGI_STREAM("Tab2Battle") << "[SDK] DefeatAsVictory found: 0x"
			<< std::hex << foundAddr << ", offset: 0x" << GTab2DefeatAsVictoryOffset << std::dec << "\n";
	}

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GTab2DefeatAsVictoryOffset),
		kTab2DefeatAsVictoryHookTemplate,
		sizeof(kTab2DefeatAsVictoryHookTemplate),
		hookId,
		false,
		true,
		false))
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] DefeatAsVictory hook install failed\n";
		return;
	}

	GTab2DefeatAsVictoryHookId = hookId;
	LOGI_STREAM("Tab2Battle") << "[SDK] DefeatAsVictory hook enabled, id=" << GTab2DefeatAsVictoryHookId << "\n";
}

void DisableDefeatAsVictoryHook()
{
	if (GTab2DefeatAsVictoryHookId == UINT32_MAX)
		return;

	InlineHook::HookManager::UninstallHook(GTab2DefeatAsVictoryHookId);
	GTab2DefeatAsVictoryHookId = UINT32_MAX;
	LOGI_STREAM("Tab2Battle") << "[SDK] DefeatAsVictory hook disabled\n";
}

void EnableNeiGongFillLastSlotFeature()
{
	if (GTab2NeiGongFillLastSlotHookId == UINT32_MAX)
	{
		if (GTab2NeiGongFillLastSlotOffset == 0)
		{
			HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
			if (!hModule)
			{
				LOGE_STREAM("Tab2Battle") << "[SDK] NeiGongFillLastSlot failed to get module handle\n";
				return;
			}
			const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);
			const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2NeiGongFillLastSlotPattern);
			if (foundAddr == 0)
			{
				LOGE_STREAM("Tab2Battle") << "[SDK] NeiGongFillLastSlot AobScan failed\n";
				return;
			}
			GTab2NeiGongFillLastSlotOffset = foundAddr - moduleBase;
			LOGI_STREAM("Tab2Battle") << "[SDK] NeiGongFillLastSlot found: 0x"
				<< std::hex << foundAddr << ", offset: 0x" << GTab2NeiGongFillLastSlotOffset << std::dec << "\n";
		}

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2NeiGongFillLastSlotOffset),
			kTab2NeiGongFillLastSlotHookTemplate,
			sizeof(kTab2NeiGongFillLastSlotHookTemplate),
			hookId,
			false,
			true,
			false))
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] NeiGongFillLastSlot hook install failed\n";
			return;
		}
		GTab2NeiGongFillLastSlotHookId = hookId;
		LOGI_STREAM("Tab2Battle") << "[SDK] NeiGongFillLastSlot hook enabled, id=" << GTab2NeiGongFillLastSlotHookId << "\n";
	}

	if (GTab2AddNeiGongNoLimitPatchAddr == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2AddNeiGongNoLimitPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AddNeiGongNoLimit AobScan failed\n";
			return;
		}
		GTab2AddNeiGongNoLimitPatchAddr = foundAddr + 8;
		LOGI_STREAM("Tab2Battle") << "[SDK] AddNeiGongNoLimit patch addr=0x"
			<< std::hex << GTab2AddNeiGongNoLimitPatchAddr << std::dec << "\n";
	}
	if (!GTab2AddNeiGongNoLimitOriginalCaptured)
	{
		uint8_t originalByte = 0x02;
		if (InlineHook::HookManager::ReadMemory(GTab2AddNeiGongNoLimitPatchAddr, &originalByte, sizeof(originalByte)))
		{
			GTab2AddNeiGongNoLimitOriginalByte = originalByte;
			GTab2AddNeiGongNoLimitOriginalCaptured = true;
		}
	}
	const uint8_t patchValue = 0x03;
	if (!InlineHook::HookManager::WriteMemory(GTab2AddNeiGongNoLimitPatchAddr, &patchValue, sizeof(patchValue)))
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] AddNeiGongNoLimit patch write failed at 0x"
			<< std::hex << GTab2AddNeiGongNoLimitPatchAddr << std::dec << "\n";
	}

	if (GTab2NeiGongLimitAddr == 0)
	{
		const uintptr_t refAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2NeiGongLimitRefPattern);
		if (refAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] NeiGongLimitRef AobScan failed\n";
			return;
		}
		int32 disp = 0;
		if (!InlineHook::HookManager::ReadValue(refAddr + 6, disp))
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] NeiGongLimitRef read disp failed\n";
			return;
		}
		GTab2NeiGongLimitAddr = (refAddr + 10) + static_cast<int64>(disp);
		LOGI_STREAM("Tab2Battle") << "[SDK] NeiGongLimit addr=0x"
			<< std::hex << GTab2NeiGongLimitAddr << std::dec << "\n";
	}
	if (!GTab2NeiGongLimitOriginalCaptured)
	{
		int32 originalLimit = 6;
		if (InlineHook::HookManager::ReadValue(GTab2NeiGongLimitAddr, originalLimit))
		{
			GTab2NeiGongLimitOriginalValue = originalLimit;
			GTab2NeiGongLimitOriginalCaptured = true;
		}
	}
	const int32 newLimit = 7;
	if (!InlineHook::HookManager::WriteValue(GTab2NeiGongLimitAddr, newLimit))
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] NeiGongLimit write failed at 0x"
			<< std::hex << GTab2NeiGongLimitAddr << std::dec << "\n";
	}
}

void DisableNeiGongFillLastSlotFeature()
{
	if (GTab2NeiGongFillLastSlotHookId != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2NeiGongFillLastSlotHookId);
		GTab2NeiGongFillLastSlotHookId = UINT32_MAX;
	}

	if (GTab2AddNeiGongNoLimitPatchAddr != 0)
	{
		const uint8_t restoreValue = GTab2AddNeiGongNoLimitOriginalCaptured ? GTab2AddNeiGongNoLimitOriginalByte : 0x02;
		if (!InlineHook::HookManager::WriteMemory(GTab2AddNeiGongNoLimitPatchAddr, &restoreValue, sizeof(restoreValue)))
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AddNeiGongNoLimit restore failed at 0x"
				<< std::hex << GTab2AddNeiGongNoLimitPatchAddr << std::dec << "\n";
		}
	}

	if (GTab2NeiGongLimitAddr != 0)
	{
		const int32 restoreLimit = GTab2NeiGongLimitOriginalCaptured ? GTab2NeiGongLimitOriginalValue : 6;
		if (!InlineHook::HookManager::WriteValue(GTab2NeiGongLimitAddr, restoreLimit))
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] NeiGongLimit restore failed at 0x"
				<< std::hex << GTab2NeiGongLimitAddr << std::dec << "\n";
		}
	}

	LOGI_STREAM("Tab2Battle") << "[SDK] NeiGongFillLastSlot feature disabled\n";
}

void EnableAllTeammatesInFightHooks()
{
	if (GTab2AllInFightHook1Id != UINT32_MAX &&
		GTab2AllInFightHook2Id != UINT32_MAX &&
		GTab2AllInFightHook3Id != UINT32_MAX &&
		GTab2AllInFightHook4Id != UINT32_MAX &&
		GTab2AllInFightHook5Id != UINT32_MAX)
	{
		return;
	}

	HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
	if (!hModule)
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight failed to get module handle\n";
		return;
	}
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);

	if (GTab2AllInFightOffset1 == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2AllInFightPattern1);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern1 scan failed\n";
			return;
		}
		GTab2AllInFightOffset1 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightOffset2 == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2AllInFightPattern2);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern2 scan failed\n";
			return;
		}
		GTab2AllInFightOffset2 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightOffset3 == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2AllInFightPattern3);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern3 scan failed\n";
			return;
		}
		GTab2AllInFightOffset3 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightOffset4 == 0)
	{
		const uintptr_t foundAddr = InlineHook::HookManager::ScanModulePatternRobust("JH-Win64-Shipping.exe", kTab2AllInFightPattern4);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern4 scan failed\n";
			return;
		}
		GTab2AllInFightOffset4 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightOffset5 == 0)
	{
		const uintptr_t base4 = moduleBase + GTab2AllInFightOffset4;
		const uintptr_t foundAddr = ScanPatternInModuleRange(
			"JH-Win64-Shipping.exe",
			kTab2AllInFightPattern5,
			base4,
			0x50);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern5(scan region allInFight4~+0x50) failed\n";
			return;
		}
		GTab2AllInFightOffset5 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightHook1Id == UINT32_MAX)
	{
		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2AllInFightOffset1),
			kTab2AllInFightHookTemplate1,
			sizeof(kTab2AllInFightHookTemplate1),
			hookId,
			false,
			true,
			false))
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight hook1 install failed\n";
			return;
		}
		GTab2AllInFightHook1Id = hookId;
	}

	if (GTab2AllInFightHook2Id == UINT32_MAX)
	{
		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2AllInFightOffset2),
			kTab2AllInFightHookTemplate2,
			sizeof(kTab2AllInFightHookTemplate2),
			hookId,
			false,
			true,
			false))
		{
			DisableAllTeammatesInFightHooks();
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight hook2 install failed\n";
			return;
		}
		GTab2AllInFightHook2Id = hookId;
	}

	if (GTab2AllInFightHook3Id == UINT32_MAX)
	{
		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2AllInFightOffset3),
			kTab2AllInFightHookTemplate3,
			sizeof(kTab2AllInFightHookTemplate3),
			hookId,
			false,
			true,
			false))
		{
			DisableAllTeammatesInFightHooks();
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight hook3 install failed\n";
			return;
		}
		GTab2AllInFightHook3Id = hookId;
	}

	if (GTab2AllInFightHook4Id == UINT32_MAX)
	{
		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2AllInFightOffset4),
			kTab2AllInFightHookTemplate4,
			sizeof(kTab2AllInFightHookTemplate4),
			hookId,
			false,
			true,
			false))
		{
			DisableAllTeammatesInFightHooks();
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight hook4 install failed\n";
			return;
		}
		GTab2AllInFightHook4Id = hookId;
	}

	if (GTab2AllInFightHook5Id == UINT32_MAX)
	{
		const uintptr_t targetAddr = moduleBase + GTab2AllInFightOffset5;
		unsigned char originalCmp[7] = {};
		if (!InlineHook::HookManager::ReadMemory(targetAddr, originalCmp, sizeof(originalCmp)))
		{
			DisableAllTeammatesInFightHooks();
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight hook5 read original bytes failed\n";
			return;
		}

		unsigned char hookCode[26] = {};
		std::memcpy(hookCode, originalCmp, 7);
		const unsigned char cmpRsp50[] = { 0x48, 0x83, 0x7C, 0x24, 0x50, 0x00 };
		std::memcpy(hookCode + 7, cmpRsp50, sizeof(cmpRsp50));
		hookCode[13] = 0x75; hookCode[14] = 0x04;
		hookCode[15] = 0x39; hookCode[16] = 0xE4;
		hookCode[17] = 0xEB; hookCode[18] = 0x07;
		std::memcpy(hookCode + 19, originalCmp, 7);

		uint32_t hookId = UINT32_MAX;
		if (!InlineHook::HookManager::InstallHook(
			"JH-Win64-Shipping.exe",
			static_cast<uint32_t>(GTab2AllInFightOffset5),
			hookCode,
			sizeof(hookCode),
			hookId,
			false,
			true,
			false))
		{
			DisableAllTeammatesInFightHooks();
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight hook5 install failed\n";
			return;
		}
		GTab2AllInFightHook5Id = hookId;
	}

	LOGI_STREAM("Tab2Battle") << "[SDK] AllInFight hooks enabled, IDs: "
		<< GTab2AllInFightHook1Id << ", "
		<< GTab2AllInFightHook2Id << ", "
		<< GTab2AllInFightHook3Id << ", "
		<< GTab2AllInFightHook4Id << ", "
		<< GTab2AllInFightHook5Id << "\n";
}

void DisableAllTeammatesInFightHooks()
{
	if (GTab2AllInFightHook5Id != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2AllInFightHook5Id);
		GTab2AllInFightHook5Id = UINT32_MAX;
	}
	if (GTab2AllInFightHook4Id != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2AllInFightHook4Id);
		GTab2AllInFightHook4Id = UINT32_MAX;
	}
	if (GTab2AllInFightHook3Id != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2AllInFightHook3Id);
		GTab2AllInFightHook3Id = UINT32_MAX;
	}
	if (GTab2AllInFightHook2Id != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2AllInFightHook2Id);
		GTab2AllInFightHook2Id = UINT32_MAX;
	}
	if (GTab2AllInFightHook1Id != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2AllInFightHook1Id);
		GTab2AllInFightHook1Id = UINT32_MAX;
	}
}

void SetBattleSpeedHookMultiplier(float Value)
{
	if (Value < 0.0f)
		Value = 0.0f;
	if (Value > 10.0f)
		Value = 10.0f;
	GTab2BattleSpeedHookMultiplier = Value;
	TryAutoApplyBattleSpeedIfNeeded(false);
}

void EnableBattleSpeedHooks()
{
	if (GTab2BattleSpeedHook1Id != UINT32_MAX && GTab2BattleSpeedHook2Id != UINT32_MAX)
	{
		InterlockedExchange(&GTab2BattleSpeedHookEnabled, 1);
		return;
	}

	unsigned char hookCode1[sizeof(kTab2BattleSpeedHookTemplate)] = {};
	std::memcpy(hookCode1, kTab2BattleSpeedHookTemplate, sizeof(hookCode1));
	const uintptr_t enableFlagAddr = reinterpret_cast<uintptr_t>(&GTab2BattleSpeedHookEnabled);
	const uintptr_t multiplierAddr = reinterpret_cast<uintptr_t>(&GTab2BattleSpeedHookMultiplier);
	std::memcpy(hookCode1 + kTab2BattleSpeedFlagImm64Offset, &enableFlagAddr, sizeof(enableFlagAddr));
	std::memcpy(hookCode1 + kTab2BattleSpeedValueImm64Offset, &multiplierAddr, sizeof(multiplierAddr));

	uint32_t hookId1 = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		kTab2BattleSpeedHookOffset1,
		hookCode1,
		sizeof(hookCode1),
		hookId1))
	{
		LOGE_STREAM("Tab2Battle") << "[SDK] BattleSpeed hook#1 install failed\n";
		return;
	}

	unsigned char hookCode2[sizeof(kTab2BattleSpeedHookTemplate)] = {};
	std::memcpy(hookCode2, kTab2BattleSpeedHookTemplate, sizeof(hookCode2));
	std::memcpy(hookCode2 + kTab2BattleSpeedFlagImm64Offset, &enableFlagAddr, sizeof(enableFlagAddr));
	std::memcpy(hookCode2 + kTab2BattleSpeedValueImm64Offset, &multiplierAddr, sizeof(multiplierAddr));

	uint32_t hookId2 = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		kTab2BattleSpeedHookOffset2,
		hookCode2,
		sizeof(hookCode2),
		hookId2))
	{
		InlineHook::HookManager::UninstallHook(hookId1);
		LOGE_STREAM("Tab2Battle") << "[SDK] BattleSpeed hook#2 install failed\n";
		return;
	}

	GTab2BattleSpeedHook1Id = hookId1;
	GTab2BattleSpeedHook2Id = hookId2;
	InterlockedExchange(&GTab2BattleSpeedHookEnabled, 1);
	GTab2BattleSpeedAutoAppliedInFight = false;
	GTab2BattleSpeedLastAppliedMultiplier = 0.0f;
	GTab2BattleSpeedLastAutoApplyTick = 0;
	TryAutoApplyBattleSpeedIfNeeded(true);

	LOGI_STREAM("Tab2Battle") << "[SDK] BattleSpeed hooks enabled, IDs: "
		<< GTab2BattleSpeedHook1Id << ", " << GTab2BattleSpeedHook2Id << "\n";
}

void DisableBattleSpeedHooks()
{
	InterlockedExchange(&GTab2BattleSpeedHookEnabled, 0);
	GTab2BattleSpeedAutoAppliedInFight = false;
	GTab2BattleSpeedLastAppliedMultiplier = 0.0f;
	GTab2BattleSpeedLastAutoApplyTick = 0;

	if (UGameTimeManager* gameTimeMgr = UManagerFuncLib::GetGameTimeManager();
		IsSafeLiveObjectOfClass(static_cast<UObject*>(gameTimeMgr), UGameTimeManager::StaticClass()))
	{
		gameTimeMgr->SetGameTimeDilationInFight(1.0f, true);
	}

	if (UJHNeoUIBattleModuleView* view = FindLiveBattleModuleViewForSpeed();
		IsSafeLiveObjectOfClass(static_cast<UObject*>(view), UJHNeoUIBattleModuleView::StaticClass()))
	{
		view->HandleBtn_SpeedUp_1();
		if (UGameTimeManager* gameTimeMgr = UManagerFuncLib::GetGameTimeManager();
			IsSafeLiveObjectOfClass(static_cast<UObject*>(gameTimeMgr), UGameTimeManager::StaticClass()))
		{
			gameTimeMgr->SetGameTimeDilationInFight(1.0f, true);
		}
	}

	if (GTab2BattleSpeedHook2Id != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2BattleSpeedHook2Id);
		GTab2BattleSpeedHook2Id = UINT32_MAX;
	}

	if (GTab2BattleSpeedHook1Id != UINT32_MAX)
	{
		InlineHook::HookManager::UninstallHook(GTab2BattleSpeedHook1Id);
		GTab2BattleSpeedHook1Id = UINT32_MAX;
	}

	LOGI_STREAM("Tab2Battle") << "[SDK] BattleSpeed hooks disabled\n";
}
