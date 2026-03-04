void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->InputSlot, "Tab2(InputSlot)");
	if (!Container) return;
	Container->ClearChildren();

	GTab2DamageBoostToggle = nullptr;
	GTab2DamageFriendlyOnlyToggle = nullptr;
	GTab2SkillNoCooldownToggle = nullptr;
	GTab2DamageMultiplierSlider = nullptr;

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
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
			return Item;
		}
		return nullptr;
	};
	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"杈撳叆鏁板瓧", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"战斗开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	GTab2SkillNoCooldownToggle = AddToggle(SwitchBox, L"招式无视冷却");
	AddToggle(SwitchBox, L"战斗加速");
	AddToggle(SwitchBox, L"不遇敌");
	AddToggle(SwitchBox, L"全队友参战");
	AddToggle(SwitchBox, L"战败视为胜利");
	AddToggle(SwitchBox, L"心法填装最后一格");
	AddToggle(SwitchBox, L"战斗前自动恢复");
	AddToggle(SwitchBox, L"移动速度加倍");
	GTab2DamageFriendlyOnlyToggle = AddToggle(SwitchBox, L"只对本方生效");
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);

	auto* RatioPanel = CreateCollapsiblePanel(PC, L"倍数与速度");
	auto* RatioBox = RatioPanel ? RatioPanel->CT_Contents : nullptr;
	AddSlider(RatioBox, L"战斗加速倍数");
	AddSlider(RatioBox, L"移动倍数");
	AddSlider(RatioBox, L"逃跑成功率");
	AddPanelWithFixedGap(RatioPanel, 0.0f, 10.0f);

	auto* ExtraPanel = CreateCollapsiblePanel(PC, L"额外参数");
	auto* ExtraBox = ExtraPanel ? ExtraPanel->CT_Contents : nullptr;
	AddNumeric(ExtraBox, L"战斗时间流速", L"1");
	AddPanelWithFixedGap(ExtraPanel, 0.0f, 8.0f);
}

namespace
{
	uint32_t GTab2UseSkillHookId = UINT32_MAX;
	uint32_t GTab2SkillNoCDHookId = UINT32_MAX;
	uintptr_t GTab2UseSkillOffset = 0;
	uintptr_t GTab2SkillNoCDOffset = 0;
	volatile LONG GTab2SkillNoCooldownFlag = 0;
	uintptr_t GTab2OwnerTeamInfoOffset = 0x1408;

	const char* kTab2UseSkillPattern = "48 8B ? 48 8B ? 8B 12 E8 ? ? ? ? 48 85 C0";
	const char* kTab2SkillNoCDPattern = "8B 80 DC 01 00 00 FF C8 83 F8 05 77 ? 48 8D 15";
	const char* kTab2JHASCFieldPattern = "48 8B B9 ? ? 00 00 0F B6 F2 48 8B";

	const unsigned char kTab2UseSkillHookTemplate[] = {
		0x49, 0x89, 0xCA,                                     // mov r10,rcx
		0x50,                                                 // push rax
		0x51,                                                 // push rcx
		0x52,                                                 // push rdx
		0x41, 0x50,                                           // push r8
		0x41, 0x51,                                           // push r9
		0x41, 0x52,                                           // push r10
		0x41, 0x53,                                           // push r11
		0x53,                                                 // push rbx
		0x49, 0xBB,                                           // mov r11, imm64(flag)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                  // imm64 low
		0x00, 0x00,                                           // imm64 high
		0x41, 0xC7, 0x03, 0x00, 0x00, 0x00, 0x00,            // mov dword ptr [r11],0
		0x48, 0x83, 0xEC, 0x20,                               // sub rsp,20h
		0x4C, 0x89, 0xD1,                                     // mov rcx,r10
		0x49, 0xBB,                                           // mov r11, imm64(helper)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                  // imm64 low
		0x00, 0x00,                                           // imm64 high
		0x41, 0xFF, 0xD3,                                     // call r11
		0x48, 0x83, 0xC4, 0x20,                               // add rsp,20h
		0x5B,                                                 // pop rbx
		0x41, 0x5B,                                           // pop r11
		0x41, 0x5A,                                           // pop r10
		0x41, 0x59,                                           // pop r9
		0x41, 0x58,                                           // pop r8
		0x5A,                                                 // pop rdx
		0x59,                                                 // pop rcx
		0x58                                                  // pop rax
	};
	constexpr size_t kTab2UseSkillFlagImm64Offset = 17;
	constexpr size_t kTab2UseSkillHelperImm64Offset = 41;

	const unsigned char kTab2SkillNoCDHookTemplate[] = {
		0x8B, 0x80, 0xDC, 0x01, 0x00, 0x00,                  // mov eax,[rax+1DC]
		0x49, 0xBB,                                           // mov r11, imm64(flag)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                  // imm64 low
		0x00, 0x00,                                           // imm64 high
		0x41, 0x83, 0x3B, 0x01,                               // cmp dword ptr [r11],1
		0x75, 0x05,                                           // jne +5
		0xB8, 0x01, 0x00, 0x00, 0x00                          // mov eax,1
	};
	constexpr size_t kTab2SkillNoCDFlagImm64Offset = 8;

	uintptr_t ScanModulePatternRobust_Tab2NoCD(const char* moduleName, const char* pattern)
	{
		if (!moduleName || !pattern)
			return 0;

		uintptr_t addr = InlineHook::HookManager::AobScanModuleFirst(moduleName, pattern, true);
		if (addr != 0)
			return addr;

		addr = InlineHook::HookManager::AobScanModuleFirst(moduleName, pattern, false);
		if (addr != 0)
			return addr;

		HMODULE hModule = GetModuleHandleA(moduleName);
		if (!hModule)
			return 0;

		const uintptr_t base = reinterpret_cast<uintptr_t>(hModule);
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
		if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
			return 0;

		const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + static_cast<uintptr_t>(dos->e_lfanew));
		if (!nt || nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage == 0)
			return 0;

		const uintptr_t end = base + static_cast<uintptr_t>(nt->OptionalHeader.SizeOfImage);
		if (end <= base)
			return 0;

		return InlineHook::HookManager::AobScanFirst(pattern, base, end, false);
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
		const uintptr_t fieldAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2JHASCFieldPattern);
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
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2UseSkillPattern);
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
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2SkillNoCDPattern);
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
