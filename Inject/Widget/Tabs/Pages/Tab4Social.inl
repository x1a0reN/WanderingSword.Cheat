void PopulateTab_Social(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->OthersSlot, "Tab4(OthersSlot)");
	if (!Container) return;
	Container->ClearChildren();
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

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
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

	auto* MainPanel = CreateCollapsiblePanel(PC, L"社交开关");
	auto* MainBox = MainPanel ? MainPanel->CT_Contents : nullptr;
	AddToggle(MainBox, L"送礼必定喜欢");
	AddToggle(MainBox, L"邀请无视条件");
	AddToggle(MainBox, L"切磋无视好感");
	AddToggle(MainBox, L"请教无视要求");
	AddToggle(MainBox, L"切磋获得对手背包");
	AddToggle(MainBox, L"NPC装备可脱");
	AddToggle(MainBox, L"NPC无视武器功法限制");
	AddToggle(MainBox, L"强制显示NPC互动");
	AddPanelWithFixedGap(MainPanel, 0.0f, 10.0f);

	auto* GiftPanel = CreateCollapsiblePanel(PC, L"送礼设置");
	auto* GiftBox = GiftPanel ? GiftPanel->CT_Contents : nullptr;
		AddDropdown(GiftBox, L"物品质量(送礼)", { L"全部", L"白", L"绿", L"蓝", L"紫", L"橙", L"红" });
	AddPanelWithFixedGap(GiftPanel, 0.0f, 8.0f);
}
