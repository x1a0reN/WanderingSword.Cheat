void PopulateTab_Character(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VolumeSlot, "Tab0(VolumeSlot)");
	if (!Container)
	{
		return;
	}

	Container->ClearChildren();
	GTab0Bindings.clear();
	GTab0EnterWasDown = false;
	GTab0LastFocusedEdit = nullptr;
	GTab0FocusCacheEdit = nullptr;       // 重置焦点缓存
	GTab0FocusCacheTick = 0;             // 重置缓存时间戳
	GTab0LastCleanupTick = 0;            // 重置清理时间戳
	GTab0LastUiPollTick = 0;             // 重置UI轮询时间戳
	GTab0RoleSelectDD = nullptr;         // 重置角色选择下拉框
	GTab0RoleSelectNPCIds.clear();        // 清空角色NPCId列表
	GTab0RoleSelectNames.clear();         // 清空角色名字列表
	GTab0SelectedRoleIdx = -1;            // 重置选中的角色索引
	GTab0LastSelectedRoleIdx = -1;       // 重置上一次选中的角色索引
	GTab0MoneyMultiplierItem = nullptr;
	GTab0SkillExpMultiplierItem = nullptr;
	GTab0ManaCostMultiplierItem = nullptr;
	GTab0ExtraNeiGongLimitDD = nullptr;
	GTab0GuildDD = nullptr;
	GTab0ExtraNeiGongLimitLastIdx = -1;
	GTab0GuildLastIdx = -1;
	GTab0GuildOptionsResolvedFromTable = false;
	GTab0GuildLastRebuildTick = 0;
	GTab0MoneyMultiplierLastPercent = -1.0f;
	GTab0SkillExpMultiplierLastPercent = -1.0f;
	GTab0ManaCostMultiplierLastPercent = -1.0f;
	GTab0MoneyMinusWasPressed = false;
	GTab0MoneyPlusWasPressed = false;
	GTab0SkillExpMinusWasPressed = false;
	GTab0SkillExpPlusWasPressed = false;
	GTab0ManaCostMinusWasPressed = false;
	GTab0ManaCostPlusWasPressed = false;
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
			Pad.Left = 0.0f;
			Pad.Top = TopGap;
			Pad.Right = 0.0f;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) -> UBPVE_JHConfigVolumeItem2_C* {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
		return Item;
	};

	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) -> UBPVE_JHConfigVolumeItem2_C* {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			RegisterTab0Binding(Title, Item, PC);
			Count++;
		}
		return Item;
	};

	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) -> UBPVE_JHConfigVideoItem2_C* {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
		return Item;
	};

	auto AddDropdownDynamic = [&](UPanelWidget* Box, const wchar_t* Title, const std::vector<std::wstring>& Options) -> UBPVE_JHConfigVideoItem2_C* {
		auto* Item = CreateVideoItem(PC, Title);
		if (!Item)
			return nullptr;
		if (Item->CB_Main)
		{
			Item->CB_Main->ClearOptions();
			for (const auto& Opt : Options)
				Item->CB_Main->AddOption(FString(Opt.c_str()));
			if (GetComboOptionCountFast(Item->CB_Main) > 0)
				Item->CB_Main->SetSelectedIndex(0);
		}
		if (Box) Box->AddChild(Item);
		else Container->AddChild(Item);
		Count++;
		return Item;
	};

	// 角色选择下拉框 - 放在最上方
	RefreshTab0RoleSelectOptions();
	auto* RoleSelectPanel = CreateCollapsiblePanel(PC, L"选择角色");
	auto* RoleSelectBox = RoleSelectPanel ? RoleSelectPanel->CT_Contents : nullptr;
	if (!GTab0RoleSelectNames.empty())
	{
	// 使用 CreateVideoItemWithOptions 的方式创建带选项的下拉框
	GTab0RoleSelectDD = CreateVideoItemWithOptions(PC, L"当前角色", {});
	if (GTab0RoleSelectDD && GTab0RoleSelectDD->CB_Main)
	{
		GTab0RoleSelectDD->CB_Main->ClearOptions();
		for (const auto& Name : GTab0RoleSelectNames)
		{
			// Name 已经是 std::wstring，直接使用
			GTab0RoleSelectDD->CB_Main->AddOption(FString(Name.c_str()));
		}
		if (GetComboOptionCountFast(GTab0RoleSelectDD->CB_Main) > 0)
			GTab0RoleSelectDD->CB_Main->SetSelectedIndex(0);
		GTab0SelectedRoleIdx = 0;
		GTab0LastSelectedRoleIdx = 0;
		std::cout << "[SDK] Created role dropdown with " << GTab0RoleSelectNames.size() << " options\n";
	}
	if (RoleSelectBox)
		RoleSelectBox->AddChild(GTab0RoleSelectDD);
	else if (GTab0RoleSelectDD)
		Container->AddChild(GTab0RoleSelectDD);
	if (GTab0RoleSelectDD)
		Count++;
	}
	AddPanelWithFixedGap(RoleSelectPanel, 0.0f, 10.0f);

	// 角色选项面板 - 放在基础数值之前
	auto* RolePanel = CreateCollapsiblePanel(PC, L"角色选项");
	auto* RoleBox = RolePanel ? RolePanel->CT_Contents : nullptr;
	GTab0ExtraNeiGongLimitDD = AddDropdown(RoleBox, L"额外心法栏", { L"0", L"1", L"2" });
	RebuildTab0GuildOptionsFromGame(PC);
	GTab0GuildDD = AddDropdownDynamic(RoleBox, L"门派", GTab0GuildOptionLabels);
	RefreshTab0GuildDropdownOptionsIfNeeded(PC, true);
	AddPanelWithFixedGap(RolePanel, 0.0f, 10.0f);

	auto* BasePanel = CreateCollapsiblePanel(PC, L"基础数值");
	auto* BaseBox = BasePanel ? BasePanel->CT_Contents : nullptr;
	AddNumeric(BaseBox, L"金钱", L"99999");
	AddNumeric(BaseBox, L"武学点", L"100");
	AddNumeric(BaseBox, L"经脉点", L"100");
	AddNumeric(BaseBox, L"门派贡献", L"100");
	AddNumeric(BaseBox, L"继承点", L"100");
	AddNumeric(BaseBox, L"等级", L"10");
	AddNumeric(BaseBox, L"钓鱼等级", L"10");
	AddPanelWithFixedGap(BasePanel, 0.0f, 10.0f);

	auto* AttrPanel = CreateCollapsiblePanel(PC, L"基础属性");
	auto* AttrBox = AttrPanel ? AttrPanel->CT_Contents : nullptr;
	AddNumeric(AttrBox, L"气血", L"100");
	AddNumeric(AttrBox, L"气血上限", L"100");
	AddNumeric(AttrBox, L"真气", L"100");
	AddNumeric(AttrBox, L"真气上限", L"100");
	AddNumeric(AttrBox, L"精力", L"100");
	AddNumeric(AttrBox, L"精力上限", L"100");
	AddNumeric(AttrBox, L"力道", L"100");
	AddNumeric(AttrBox, L"根骨", L"100");
	AddNumeric(AttrBox, L"身法", L"100");
	AddNumeric(AttrBox, L"内功", L"100");
	AddNumeric(AttrBox, L"攻击", L"100");
	AddNumeric(AttrBox, L"防御", L"100");
	AddNumeric(AttrBox, L"暴击", L"100");
	AddNumeric(AttrBox, L"暴击抗性", L"100");
	AddNumeric(AttrBox, L"闪避", L"100");
	AddNumeric(AttrBox, L"命中", L"100");
	AddNumeric(AttrBox, L"移动", L"100");
	AddNumeric(AttrBox, L"聚气速率", L"100");
	AddNumeric(AttrBox, L"真气护盾", L"100");
	AddNumeric(AttrBox, L"气血护盾", L"100");
	AddNumeric(AttrBox, L"名声", L"100");
	AddNumeric(AttrBox, L"暴击伤害百分比", L"100");
	AddPanelWithFixedGap(AttrPanel, 0.0f, 10.0f);

	auto* RecoverPanel = CreateCollapsiblePanel(PC, L"恢复速率");
	auto* RecoverBox = RecoverPanel ? RecoverPanel->CT_Contents : nullptr;
	AddNumeric(RecoverBox, L"气血恢复速率1", L"100");
	AddNumeric(RecoverBox, L"气血恢复速率2", L"100");
	AddNumeric(RecoverBox, L"真气恢复速率1", L"100");
	AddNumeric(RecoverBox, L"真气恢复速率2", L"100");
	AddPanelWithFixedGap(RecoverPanel, 0.0f, 10.0f);

	auto* RatioPanel = CreateCollapsiblePanel(PC, L"倍率与消耗");
	auto* RatioBox = RatioPanel ? RatioPanel->CT_Contents : nullptr;
	GTab0MoneyMultiplierItem = AddSlider(RatioBox, L"金钱倍率");
	GTab0SkillExpMultiplierItem = AddSlider(RatioBox, L"武学点倍率");
	GTab0ManaCostMultiplierItem = AddSlider(RatioBox, L"真气消耗倍率");
	ConfigureTab0RatioSlider(GTab0MoneyMultiplierItem);
	ConfigureTab0RatioSlider(GTab0SkillExpMultiplierItem);
	ConfigureTab0RatioSlider(GTab0ManaCostMultiplierItem);
	RemoveVolumeItemFromGlobalPoll(GTab0MoneyMultiplierItem);
	RemoveVolumeItemFromGlobalPoll(GTab0SkillExpMultiplierItem);
	RemoveVolumeItemFromGlobalPoll(GTab0ManaCostMultiplierItem);
	{
		FTab0HeroContext RatioCtx = BuildTab0HeroContext(PC);
			float Pct = 1.0f;

		if (TryGetTab0MultiplierPercent(RatioCtx, L"MoneyMultiplier", &Pct))
			SetVolumeItemPercent(GTab0MoneyMultiplierItem, Pct);
		else
				SetVolumeItemPercent(GTab0MoneyMultiplierItem, 1.0f);
		GTab0MoneyMultiplierLastPercent = GetVolumeItemPercent(GTab0MoneyMultiplierItem);

			Pct = 1.0f;
		if (TryGetTab0MultiplierPercent(RatioCtx, L"SExpMultiplier", &Pct))
			SetVolumeItemPercent(GTab0SkillExpMultiplierItem, Pct);
		else
				SetVolumeItemPercent(GTab0SkillExpMultiplierItem, 1.0f);
		GTab0SkillExpMultiplierLastPercent = GetVolumeItemPercent(GTab0SkillExpMultiplierItem);

			Pct = 1.0f;
		if (TryGetTab0MultiplierPercent(RatioCtx, L"ManaCostMultiplier", &Pct))
			SetVolumeItemPercent(GTab0ManaCostMultiplierItem, Pct);
		else
				SetVolumeItemPercent(GTab0ManaCostMultiplierItem, 1.0f);
		GTab0ManaCostMultiplierLastPercent = GetVolumeItemPercent(GTab0ManaCostMultiplierItem);
	}
	AddPanelWithFixedGap(RatioPanel, 0.0f, 10.0f);

	auto* WeaponPanel = CreateCollapsiblePanel(PC, L"武学精通与经验");
	auto* WeaponBox = WeaponPanel ? WeaponPanel->CT_Contents : nullptr;
	AddNumeric(WeaponBox, L"拳掌精通", L"100");
	AddNumeric(WeaponBox, L"拳掌经验", L"100");
	AddNumeric(WeaponBox, L"剑法精通", L"100");
	AddNumeric(WeaponBox, L"剑法经验", L"100");
	AddNumeric(WeaponBox, L"刀法精通", L"100");
	AddNumeric(WeaponBox, L"刀法经验", L"100");
	AddNumeric(WeaponBox, L"枪棍精通", L"100");
	AddNumeric(WeaponBox, L"枪棍经验", L"100");
	AddNumeric(WeaponBox, L"暗器精通", L"100");
	AddNumeric(WeaponBox, L"暗器经验", L"100");
	AddNumeric(WeaponBox, L"其他武器精通", L"100");
	AddNumeric(WeaponBox, L"其他武器经验", L"100");
	AddPanelWithFixedGap(WeaponPanel, 0.0f, 8.0f);
	SyncTab0RoleDropdownsFromLive(PC);
	RefreshTab0BindingsText(PC);
	DumpTab0InitSnapshot(PC);
}

void PollTab0CharacterInput(bool bTab0Active)
{
	Tab0Trace("InputPoll.Begin", "tab0Active=", (bTab0Active ? 1 : 0), " bindingsBefore=", GTab0Bindings.size());

	// 方案1: 清理降频 - 每500ms运行一次清理，而不是每帧都运行
	const ULONGLONG Now = GetTickCount64();
	const bool bDoCleanup = !GTab0LastCleanupTick || (Now - GTab0LastCleanupTick) >= kTab0CleanupIntervalMs;
	if (bDoCleanup)
	{
		GTab0Bindings.erase(
			std::remove_if(
				GTab0Bindings.begin(),
				GTab0Bindings.end(),
				[](const FTab0Binding& Binding)
				{
					return !Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit));
				}),
			GTab0Bindings.end());
		Tab0Trace("InputPoll.BindingsPruned", "bindingsAfter=", GTab0Bindings.size());

		if (GTab0LastFocusedEdit &&
			!IsSafeLiveObject(static_cast<UObject*>(GTab0LastFocusedEdit)))
		{
			GTab0LastFocusedEdit = nullptr;
		}
		// 同时清理焦点缓存中无效的条目
		if (GTab0FocusCacheEdit && !IsSafeLiveObject(static_cast<UObject*>(GTab0FocusCacheEdit)))
		{
			GTab0FocusCacheEdit = nullptr;
		}
		GTab0LastCleanupTick = Now;
	}

	const bool EnterDown = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
	const bool EnterTriggered = EnterDown && !GTab0EnterWasDown;
	Tab0Trace("InputPoll.KeyState", "enterDown=", (EnterDown ? 1 : 0), " enterTriggered=", (EnterTriggered ? 1 : 0));
	if (!bTab0Active)
	{
		PollTab0RoleDropdowns(nullptr, false);
		GTab0EnterWasDown = EnterDown;
		GTab0LastFocusedEdit = nullptr;
		GTab0FocusCacheEdit = nullptr;  // Tab 不活跃时清空焦点缓存
		Tab0Trace("InputPoll.End", "reason=TabInactive");
		return;
	}

	APlayerController* PC = nullptr;
	if (UWorld* World = UWorld::GetWorld())
		PC = UGameplayStatics::GetPlayerController(World, 0);
	Tab0Trace("InputPoll.Controller", "pc=", (void*)PC);
	if (!PC)
		Tab0Trace("InputPoll.Controller", "reason=PlayerControllerNull");

	// 降频轮询: 每100ms运行一次下拉框和滑块轮询
	const bool bDoUiPoll = !GTab0LastUiPollTick || (Now - GTab0LastUiPollTick) >= kTab0UiPollIntervalMs;
	if (bDoUiPoll)
	{
		// 下拉框轮询
		PollTab0RoleDropdowns(PC, true);
		// Tab0 三个倍率滑块：实时同步到游戏属性
		PollTab0RatioSliders(PC);
		GTab0LastUiPollTick = Now;
	}

	// 方案2: 焦点缓存 - 只有在 EnterTriggered 或缓存过期时才扫描焦点
	// 计算缓存是否过期
	const bool bCacheExpired = !GTab0FocusCacheTick || (Now - GTab0FocusCacheTick) >= kTab0FocusCacheDurationMs;
	// 检查缓存的编辑框是否仍然有效
	const bool bCachedEditValid = GTab0FocusCacheEdit && IsSafeLiveObject(static_cast<UObject*>(GTab0FocusCacheEdit));
	// 是否需要进行焦点扫描: EnterTriggered 时必须扫描，或者缓存已过期，或者缓存的编辑框已失效
	const bool bNeedFocusScan = EnterTriggered || bCacheExpired || !bCachedEditValid;

	FTab0Binding* Focused = nullptr;
	if (bNeedFocusScan)
	{
		// 执行真正的焦点扫描
		for (auto& Binding : GTab0Bindings)
		{
			if (!Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit)))
				continue;
			if (Binding.Edit->HasKeyboardFocus())
			{
				Focused = &Binding;
				break;
			}
		}
		// 更新缓存
		GTab0FocusCacheEdit = Focused ? Focused->Edit : nullptr;
		GTab0FocusCacheTick = Now;
	}
	else
	{
		// 使用缓存的结果
		Focused = FindTab0BindingByEdit(GTab0FocusCacheEdit);
	}
	Tab0Trace("InputPoll.FocusScan",
		"focused=", (void*)(Focused ? Focused->Edit : nullptr),
		" lastFocused=", (void*)GTab0LastFocusedEdit,
		" cached=", (!bNeedFocusScan ? 1 : 0));

	// 某些编辑框按下回车瞬间会丢失键盘焦点，回车提交时回退到“上一次有焦点的编辑框”。
	if (EnterTriggered && !Focused && GTab0LastFocusedEdit)
	{
		Focused = FindTab0BindingByEdit(GTab0LastFocusedEdit);
		Tab0Trace("InputPoll.FallbackFocus", "fromLast=", (void*)GTab0LastFocusedEdit, " to=", (void*)(Focused ? Focused->Edit : nullptr));
		if (kTab0VerboseLog && Focused)
			std::cout << "[SDK][Tab0Input] EnterTriggered fallbackFocus field="
			          << Tab0FieldToString(Focused->Field) << "\n";
	}

	if (Focused && Focused->Edit)
	{
		const std::string RawFocused = Focused->Edit->GetText().ToString();
		const std::string SanitizedFocused = SanitizeNumericInputText(RawFocused, Focused->bInteger);
		if (SanitizedFocused != RawFocused)
		{
			const std::wstring Wide = AsciiToWide(SanitizedFocused);
			Focused->Edit->SetText(MakeText(Wide.c_str()));
			Tab0Trace("InputPoll.SanitizeLive",
				"field=", Tab0FieldToString(Focused->Field),
				" from=", RawFocused,
				" to=", SanitizedFocused);
		}
	}

	if (!EnterTriggered &&
		GTab0LastFocusedEdit &&
		(!Focused || Focused->Edit != GTab0LastFocusedEdit))
	{
		FTab0Binding* LastFocusedBinding = FindTab0BindingByEdit(GTab0LastFocusedEdit);
		if (LastFocusedBinding)
		{
			Tab0Trace("InputPoll.RollbackOnBlur", "field=", Tab0FieldToString(LastFocusedBinding->Field));
			RefreshSingleTab0BindingText(*LastFocusedBinding, PC);
		}
	}

	if (EnterTriggered)
	{
		if (kTab0VerboseLog && !Focused)
			std::cout << "[SDK][Tab0Input] EnterTriggered but no focused editable binding\n";
		if (!Focused)
			Tab0Trace("InputPoll.Commit.Skip", "reason=NoFocusedBinding");

		if (Focused && Focused->Edit &&
			IsSafeLiveObject(static_cast<UObject*>(Focused->Edit)))
		{
			std::string Raw = Focused->Edit->GetText().ToString();
			Raw = SanitizeNumericInputText(Raw, Focused->bInteger);
			const std::wstring Wide = AsciiToWide(Raw);
			Focused->Edit->SetText(MakeText(Wide.c_str()));

			char* EndPtr = nullptr;
			const double Parsed = std::strtod(Raw.c_str(), &EndPtr);
			const bool bParsedAll = (EndPtr && *EndPtr == '\0');
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Input] EnterTriggered field=" << Tab0FieldToString(Focused->Field)
				          << " raw=" << Raw << " parsed=" << Parsed
				          << " parsedAll=" << (bParsedAll ? 1 : 0) << "\n";
			Tab0Trace("InputPoll.Commit.Parse",
				"field=", Tab0FieldToString(Focused->Field),
				" raw=", Raw,
				" parsed=", Parsed,
				" parsedAll=", (bParsedAll ? 1 : 0));
			if (EndPtr != Raw.c_str() && bParsedAll)
			{
				FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
				Tab0Trace("InputPoll.Commit.Try", "field=", Tab0FieldToString(Focused->Field), " parsed=", Parsed);
				if (TrySetTab0FieldValue(Ctx, Focused->Field, Parsed))
				{
					RefreshTab0BindingsText(PC);
					std::cout << "[SDK] Tab0Commit: " << (Focused->Title ? "ok" : "unnamed")
					          << " raw=" << Raw << "\n";
					Tab0Trace("InputPoll.Commit.Ok", "field=", Tab0FieldToString(Focused->Field));
				}
				else
				{
					std::cout << "[SDK][Tab0Input] Commit failed field=" << Tab0FieldToString(Focused->Field)
					          << " raw=" << Raw << "\n";
					Tab0Trace("InputPoll.Commit.Fail", "field=", Tab0FieldToString(Focused->Field));
				}
			}
			else
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Input] Invalid numeric input, rollback field="
					          << Tab0FieldToString(Focused->Field) << " raw=" << Raw << "\n";
				RefreshSingleTab0BindingText(*Focused, PC);
				Tab0Trace("InputPoll.Commit.InvalidInput", "field=", Tab0FieldToString(Focused->Field), " raw=", Raw);
			}
		}
	}

	GTab0LastFocusedEdit = Focused ? Focused->Edit : nullptr;
	GTab0EnterWasDown = EnterDown;
	Tab0Trace("InputPoll.End", "focusedNow=", (void*)GTab0LastFocusedEdit, " enterWasDown=", (GTab0EnterWasDown ? 1 : 0));
}
