void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->InputSlot, "Tab2(InputSlot)");
	if (!Container) return;
	Container->ClearChildren();

	GTab2DamageBoostToggle = nullptr;
	GTab2DamageFriendlyOnlyToggle = nullptr;
	GTab2SkillNoCooldownToggle = nullptr;
	GTab2NoEncounterToggle = nullptr;
	GTab2AllTeammatesInFightToggle = nullptr;
	GTab2DefeatAsVictoryToggle = nullptr;
	GTab2DamageMultiplierSlider = nullptr;

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
				const std::wstring titleKey(Title);
				const auto it = GUIRememberState.SliderValueByTitle.find(titleKey);
				hasRememberedValue = (it != GUIRememberState.SliderValueByTitle.end());
				if (hasRememberedValue)
					rememberedValue = it->second;
			}

			if (Item->VolumeSlider)
			{
				Item->VolumeSlider->MinValue = 1.0f;
				Item->VolumeSlider->MaxValue = 10.0f;
				Item->VolumeSlider->StepSize = 1.0f;
				if (hasRememberedValue)
				{
					if (rememberedValue < 1.0f) rememberedValue = 1.0f;
					if (rememberedValue > 10.0f) rememberedValue = 10.0f;
					Item->VolumeSlider->SetValue(rememberedValue);
				}
				else
					Item->VolumeSlider->SetValue(2.0f);
			}
			if (Item->TXT_CurrentValue && Item->VolumeSlider)
			{
				float value = Item->VolumeSlider->GetValue();
				if (value < 1.0f) value = 1.0f;
				if (value > 10.0f) value = 10.0f;
				const int32 displayValue = static_cast<int32>(value + 0.5f);
				wchar_t buf[16] = {};
				swprintf_s(buf, 16, L"%d", displayValue);
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
	GTab2SkillNoCooldownToggle = AddToggle(SwitchBox, L"招式无视冷却");
	GTab2DamageBoostToggle = AddToggle(SwitchBox, L"战斗加速");
	GTab2NoEncounterToggle = AddToggle(SwitchBox, L"不遇敌");
	GTab2AllTeammatesInFightToggle = AddToggle(SwitchBox, L"全队友参战");
	GTab2DefeatAsVictoryToggle = AddToggle(SwitchBox, L"战败视为胜利");
	AddToggle(SwitchBox, L"心法填装最后一格");
	AddToggle(SwitchBox, L"战斗前自动恢复");
	AddToggle(SwitchBox, L"移动速度加倍");
	GTab2DamageFriendlyOnlyToggle = AddToggle(SwitchBox, L"只对本方生效");
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);

	auto* RatioPanel = CreateCollapsiblePanel(PC, L"倍数与速度");
	auto* RatioBox = RatioPanel ? RatioPanel->CT_Contents : nullptr;
	GTab2DamageMultiplierSlider = AddSlider(RatioBox, L"战斗加速倍数");
	AddSlider(RatioBox, L"移动倍数");
	AddSlider(RatioBox, L"逃跑成功率");
	AddPanelWithFixedGap(RatioPanel, 0.0f, 10.0f);
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
	uintptr_t GTab2UseSkillOffset = 0;
	uintptr_t GTab2SkillNoCDOffset = 0;
	uintptr_t GTab2AllInFightOffset1 = 0;
	uintptr_t GTab2AllInFightOffset2 = 0;
	uintptr_t GTab2AllInFightOffset3 = 0;
	uintptr_t GTab2AllInFightOffset4 = 0;
	uintptr_t GTab2AllInFightOffset5 = 0;
	uintptr_t GTab2DefeatAsVictoryOffset = 0;
	volatile LONG GTab2SkillNoCooldownFlag = 0;
	volatile LONG GTab2BattleSpeedHookEnabled = 0;
	float GTab2BattleSpeedHookMultiplier = 2.0f;
	bool GTab2BattleSpeedAutoAppliedInFight = false;
	float GTab2BattleSpeedLastAppliedMultiplier = 0.0f;
	DWORD GTab2BattleSpeedLastAutoApplyTick = 0;
	uintptr_t GTab2OwnerTeamInfoOffset = 0x1408;
	uintptr_t GTab2NoEncounterPatchAddr = 0;
	unsigned char GTab2NoEncounterOriginalBytes[2] = { 0x0F, 0x84 };
	bool GTab2NoEncounterOriginalCaptured = false;

	constexpr uint32_t kTab2BattleSpeedHookOffset1 = 0x100D67B; // sub_14100D620 + 0x5B
	constexpr uint32_t kTab2BattleSpeedHookOffset2 = 0x100D73B; // sub_14100D6E0 + 0x5B
	constexpr int32 kTab2BattleModuleAnchorIndex = 52869;
	constexpr int32 kTab2BattleModuleSearchRadius = 10;

	const char* kTab2UseSkillPattern = "48 8B ? 48 8B ? 8B 12 E8 ? ? ? ? 48 85 C0";
	const char* kTab2SkillNoCDPattern = "8B 80 DC 01 00 00 FF C8 83 F8 05 77 ? 48 8D 15";
	const char* kTab2JHASCFieldPattern = "48 8B B9 ? ? 00 00 0F B6 F2 48 8B";
	const char* kTab2NoEncounterPattern = "? 8B EA 4C 8B F1 48 85 D2 0F 84 ? ? 00 00 E8";
	const char* kTab2DefeatAsVictoryPattern = "41 55 41 56 41 57 48 83 EC 50 44 0F B6 ? 48 8B";
	const char* kTab2AllInFightPattern1 = "49 63 85 08 01 00 00 48 8D";
	const char* kTab2AllInFightPattern2 = "49 8D B7 00 01 00 00 48 89 75";
	const char* kTab2AllInFightPattern3 = "83 E8 01 49 8B 3A";
	const char* kTab2AllInFightPattern4 = "49 8B 0C 04 83 79 38 00";
	const char* kTab2AllInFightPattern5 = "80 BD ? ? 00 00 05";

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

	const unsigned char kTab2BattleSpeedHookTemplate[] = {
		0x49, 0xBB,                                           // mov r11, imm64(enableFlag)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                  // imm64 low
		0x00, 0x00,                                           // imm64 high
		0x41, 0x83, 0x3B, 0x00,                               // cmp dword ptr [r11],0
		0x74, 0x0F,                                           // je +0F
		0x49, 0xBB,                                           // mov r11, imm64(multiplier)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                  // imm64 low
		0x00, 0x00,                                           // imm64 high
		0xF3, 0x41, 0x0F, 0x10, 0x33                          // movss xmm6,dword ptr [r11]
	};
	constexpr size_t kTab2BattleSpeedFlagImm64Offset = 2;
	constexpr size_t kTab2BattleSpeedValueImm64Offset = 18;

	// CT 语义:
	// [ENABLE] aobscanmodule(..., 41 55 41 56 41 57 48 83 EC 50 44 0F B6 ? 48 8B)
	// 注入后先 mov dl,0，再执行被覆盖的 push r13/push r14/push r15。
	// 这里通过 appendRelocatedOriginalCode=false，确保不拼接原地 stolen bytes。
	const unsigned char kTab2DefeatAsVictoryHookTemplate[] = {
		0xB2, 0x00,       // mov dl,0
		0x41, 0x55,       // push r13
		0x41, 0x56,       // push r14
		0x41, 0x57        // push r15
	};

	const unsigned char kTab2AllInFightHookTemplate1[] = {
		0x49, 0x8B, 0x8D, 0xF0, 0x00, 0x00, 0x00,             // mov rcx,[r13+F0]
		0x49, 0x63, 0x85, 0xF8, 0x00, 0x00, 0x00              // movsxd rax,dword ptr [r13+F8]
	};

	const unsigned char kTab2AllInFightHookTemplate2[] = {
		0x41, 0x8B, 0xB7, 0xF8, 0x00, 0x00, 0x00,             // mov esi,[r15+F8]
		0xFF, 0xCE,                                           // dec esi
		0x89, 0x74, 0x24, 0x58,                               // mov [rsp+58],esi
		0x89, 0x74, 0x24, 0x5C,                               // mov [rsp+5C],esi
		0x49, 0x8D, 0xB7, 0xF0, 0x00, 0x00, 0x00              // lea rsi,[r15+F0]
	};

	const unsigned char kTab2AllInFightHookTemplate3[] = {
		0x50,                                                 // push rax
		0x52,                                                 // push rdx
		0x53,                                                 // push rbx
		0x48, 0x8B, 0x16,                                     // mov rdx,[rsi]
		0x4A, 0x8B, 0x1C, 0x22,                               // mov rbx,[rdx+r12]
		0x83, 0xBB, 0x90, 0x00, 0x00, 0x00, 0x00,             // cmp dword ptr [rbx+90],0
		0x74, 0x19,                                           // je +0x19 (to xor rdi,rdi)
		0x44, 0x89, 0xF0,                                     // mov eax,r14d
		0x48, 0x31, 0xD2,                                     // xor rdx,rdx
		0xBB, 0x04, 0x00, 0x00, 0x00,                         // mov ebx,4
		0xF7, 0xF3,                                           // div ebx
		0x48, 0x89, 0xD0,                                     // mov rax,rdx
		0x4D, 0x8D, 0x14, 0xC1,                               // lea r10,[r9+rax*8]
		0x49, 0x8B, 0x3A,                                     // mov rdi,[r10]
		0xEB, 0x03,                                           // jmp +3
		0x48, 0x31, 0xFF,                                     // xor rdi,rdi
		0x5B,                                                 // pop rbx
		0x5A,                                                 // pop rdx
		0x58,                                                 // pop rax
		0x83, 0xE8, 0x01                                      // sub eax,1
	};

	const unsigned char kTab2AllInFightHookTemplate4[] = {
		0x49, 0x8B, 0x0C, 0x04,                               // mov rcx,[r12+rax]
		0x48, 0x83, 0x7C, 0x24, 0x50, 0x00,                   // cmp qword ptr [rsp+50],0
		0x75, 0x04,                                           // jne +4
		0x39, 0xC0,                                           // cmp eax,eax
		0xEB, 0x04,                                           // jmp +4
		0x83, 0x79, 0x38, 0x00                                // cmp dword ptr [rcx+38],0
	};

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

		// 保持原生按钮状态语义：当前是 1x 就走 _1，否则走 _2。
		// 我们的 inline hook 会把两条分支最终都改成自定义倍速。
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

void EnableNoEncounterPatch()
{
	if (GTab2NoEncounterPatchAddr == 0)
	{
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2NoEncounterPattern);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] NoEncounter AobScan failed, pattern not found\n";
			return;
		}
		GTab2NoEncounterPatchAddr = foundAddr + 9; // CE: BuYuDi+9
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
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2DefeatAsVictoryPattern);
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
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2AllInFightPattern1);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern1 scan failed\n";
			return;
		}
		GTab2AllInFightOffset1 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightOffset2 == 0)
	{
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2AllInFightPattern2);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern2 scan failed\n";
			return;
		}
		GTab2AllInFightOffset2 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightOffset3 == 0)
	{
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2AllInFightPattern3);
		if (foundAddr == 0)
		{
			LOGE_STREAM("Tab2Battle") << "[SDK] AllInFight pattern3 scan failed\n";
			return;
		}
		GTab2AllInFightOffset3 = foundAddr - moduleBase;
	}

	if (GTab2AllInFightOffset4 == 0)
	{
		const uintptr_t foundAddr = ScanModulePatternRobust_Tab2NoCD("JH-Win64-Shipping.exe", kTab2AllInFightPattern4);
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
		std::memcpy(hookCode, originalCmp, 7);                          // readmem(allInFight5,7)
		const unsigned char cmpRsp50[] = { 0x48, 0x83, 0x7C, 0x24, 0x50, 0x00 };
		std::memcpy(hookCode + 7, cmpRsp50, sizeof(cmpRsp50));          // cmp [rsp+50],0
		hookCode[13] = 0x75; hookCode[14] = 0x04;                       // jne +4
		hookCode[15] = 0x39; hookCode[16] = 0xE4;                       // cmp esp,esp
		hookCode[17] = 0xEB; hookCode[18] = 0x07;                       // jmp +7
		std::memcpy(hookCode + 19, originalCmp, 7);                     // readmem(allInFight5,7)

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
	if (Value < 1.0f)
		Value = 1.0f;
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

	// 关闭加速时主动回收到 1x，避免上一次战斗残留倍率持续生效。
	// 顺序：
	// 1) 用 GameTimeManager 强制落地 1.0f
	// 2) 同步一次战斗界面按钮状态（尽量与原生 UI 语义一致）
	if (UGameTimeManager* gameTimeMgr = UManagerFuncLib::GetGameTimeManager();
		IsSafeLiveObjectOfClass(static_cast<UObject*>(gameTimeMgr), UGameTimeManager::StaticClass()))
	{
		gameTimeMgr->SetGameTimeDilationInFight(1.0f, true);
	}

	if (UJHNeoUIBattleModuleView* view = FindLiveBattleModuleViewForSpeed();
		IsSafeLiveObjectOfClass(static_cast<UObject*>(view), UJHNeoUIBattleModuleView::StaticClass()))
	{
		// 与用户操作逻辑保持一致：关加速时触发一次“1x按钮”路径。
		view->HandleBtn_SpeedUp_1();
		if (UGameTimeManager* gameTimeMgr = UManagerFuncLib::GetGameTimeManager();
			IsSafeLiveObjectOfClass(static_cast<UObject*>(gameTimeMgr), UGameTimeManager::StaticClass()))
		{
			// 二次兜底，确保最终战斗时间流速确实是 1x。
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
