void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent6) return;
	GDynTabContent6->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GTeammateFollowToggle = nullptr;
	GTeammateFollowCount = nullptr;
	GTeammateAddDD = nullptr;
	GTeammateReplaceToggle = nullptr;
	GTeammateReplaceDD = nullptr;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTabContent6->AddChild(Panel);
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
	GTeammateFollowToggle = CreateToggleItem(PC, L"设置队友跟随数量");
	if (GTeammateFollowToggle)
	{
		if (TeamBox) TeamBox->AddChild(GTeammateFollowToggle);
		else GDynTabContent6->AddChild(GTeammateFollowToggle);
		Count++;
	}
	GTeammateFollowCount = CreateVolumeNumericEditBoxItem(PC, Outer, TeamBox ? TeamBox : GDynTabContent6, L"跟随数量", L"输入数字", L"3");
	if (GTeammateFollowCount)
	{
		if (TeamBox) TeamBox->AddChild(GTeammateFollowCount);
		else GDynTabContent6->AddChild(GTeammateFollowCount);
		Count++;
	}
	AddPanelWithFixedGap(TeamPanel, 0.0f, 10.0f);

	auto* OperatePanel = CreateCollapsiblePanel(PC, L"队友操作");
	auto* OperateBox = OperatePanel ? OperatePanel->CT_Contents : nullptr;
		GTeammateAddDD = CreateVideoItemWithOptions(PC, L"添加队友",
			{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammateAddDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammateAddDD);
		else GDynTabContent6->AddChild(GTeammateAddDD);
		Count++;
	}
	GTeammateReplaceToggle = CreateToggleItem(PC, L"替换指定队友");
	if (GTeammateReplaceToggle)
	{
		if (OperateBox) OperateBox->AddChild(GTeammateReplaceToggle);
		else GDynTabContent6->AddChild(GTeammateReplaceToggle);
		Count++;
	}
		GTeammateReplaceDD = CreateVideoItemWithOptions(PC, L"指定队友",
			{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammateReplaceDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammateReplaceDD);
		else GDynTabContent6->AddChild(GTeammateReplaceDD);
		Count++;
	}
	AddPanelWithFixedGap(OperatePanel, 0.0f, 8.0f);
}


