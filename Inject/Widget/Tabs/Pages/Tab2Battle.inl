void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->InputSlot, "Tab2(InputSlot)");
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
	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
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

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"战斗开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	AddToggle(SwitchBox, L"伤害加倍");
	AddToggle(SwitchBox, L"招式无视冷却");
	AddToggle(SwitchBox, L"战斗加速");
	AddToggle(SwitchBox, L"不遇敌");
	AddToggle(SwitchBox, L"全队友参战");
	AddToggle(SwitchBox, L"战败视为胜利");
	AddToggle(SwitchBox, L"心法填装最后一格");
	AddToggle(SwitchBox, L"战斗前自动恢复");
	AddToggle(SwitchBox, L"移动速度加倍");
	AddToggle(SwitchBox, L"只对本方生效");
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);

	auto* RatioPanel = CreateCollapsiblePanel(PC, L"倍率与速度");
	auto* RatioBox = RatioPanel ? RatioPanel->CT_Contents : nullptr;
	AddSlider(RatioBox, L"伤害倍率");
	AddSlider(RatioBox, L"战斗加速倍数");
	AddSlider(RatioBox, L"移动倍率");
	AddSlider(RatioBox, L"逃跑成功率");
	AddPanelWithFixedGap(RatioPanel, 0.0f, 10.0f);

	auto* ExtraPanel = CreateCollapsiblePanel(PC, L"额外参数");
	auto* ExtraBox = ExtraPanel ? ExtraPanel->CT_Contents : nullptr;
	AddNumeric(ExtraBox, L"战斗时间流速", L"1");
	AddPanelWithFixedGap(ExtraPanel, 0.0f, 8.0f);

}
