void PopulateTab_Life(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->LanSlot, "Tab3(LanSlot)");
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

	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"生活开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	AddToggle(SwitchBox, L"锻造/制衣/炼丹/烹饪无视要求");
	AddToggle(SwitchBox, L"设置产出数量");
	AddToggle(SwitchBox, L"采集一秒冷却");
	AddToggle(SwitchBox, L"钓鱼只钓稀有物");
	AddToggle(SwitchBox, L"钓鱼收杆必有收获");
	AddToggle(SwitchBox, L"家园随时收获");
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);

	auto* OutputPanel = CreateCollapsiblePanel(PC, L"产出与掉落");
	auto* OutputBox = OutputPanel ? OutputPanel->CT_Contents : nullptr;
	AddNumeric(OutputBox, L"产出数量", L"1");
	AddPanelWithFixedGap(OutputPanel, 0.0f, 10.0f);

	auto* MasteryPanel = CreateCollapsiblePanel(PC, L"生活精通");
	auto* MasteryBox = MasteryPanel ? MasteryPanel->CT_Contents : nullptr;
	AddNumeric(MasteryBox, L"锻造精通", L"100");
	AddNumeric(MasteryBox, L"医术精通", L"100");
	AddNumeric(MasteryBox, L"制衣精通", L"100");
	AddNumeric(MasteryBox, L"炼丹精通", L"100");
	AddNumeric(MasteryBox, L"烹饪精通", L"100");
	AddNumeric(MasteryBox, L"采集精通", L"100");
	AddNumeric(MasteryBox, L"钓鱼精通", L"100");
	AddNumeric(MasteryBox, L"饮酒精通", L"100");
	AddNumeric(MasteryBox, L"茶道精通", L"100");
	AddNumeric(MasteryBox, L"口才精通", L"100");
	AddNumeric(MasteryBox, L"书法精通", L"100");
	AddPanelWithFixedGap(MasteryPanel, 0.0f, 10.0f);

	auto* ExpPanel = CreateCollapsiblePanel(PC, L"生活经验");
	auto* ExpBox = ExpPanel ? ExpPanel->CT_Contents : nullptr;
	AddNumeric(ExpBox, L"锻造经验", L"100");
	AddNumeric(ExpBox, L"医术经验", L"100");
	AddNumeric(ExpBox, L"制衣经验", L"100");
	AddNumeric(ExpBox, L"炼丹经验", L"100");
	AddNumeric(ExpBox, L"烹饪经验", L"100");
	AddNumeric(ExpBox, L"采集经验", L"100");
	AddNumeric(ExpBox, L"钓鱼经验", L"100");
	AddNumeric(ExpBox, L"饮酒经验", L"100");
	AddNumeric(ExpBox, L"茶道经验", L"100");
	AddNumeric(ExpBox, L"口才经验", L"100");
	AddNumeric(ExpBox, L"书法经验", L"100");
	AddPanelWithFixedGap(ExpPanel, 0.0f, 8.0f);
}
