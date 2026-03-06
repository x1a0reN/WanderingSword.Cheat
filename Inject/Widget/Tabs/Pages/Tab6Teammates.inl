void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTab.Content6) return;
	GDynTab.Content6->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GTeammate.FollowToggle = nullptr;
	GTeammate.FollowCount = nullptr;
	GTeammate.AddDD = nullptr;
	GTeammate.ReplaceToggle = nullptr;
	GTeammate.ReplaceDD = nullptr;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTab.Content6->AddChild(Panel);
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
	GTeammate.FollowToggle = CreateToggleItem(PC, L"设置队友跟随数量");
	if (GTeammate.FollowToggle)
	{
		if (TeamBox) TeamBox->AddChild(GTeammate.FollowToggle);
		else GDynTab.Content6->AddChild(GTeammate.FollowToggle);
		Count++;
	}
	GTeammate.FollowCount = CreateVolumeNumericEditBoxItem(PC, Outer, TeamBox ? TeamBox : GDynTab.Content6, L"跟随数量", L"输入数字", L"3");
	if (GTeammate.FollowCount)
	{
		if (TeamBox) TeamBox->AddChild(GTeammate.FollowCount);
		else GDynTab.Content6->AddChild(GTeammate.FollowCount);
		Count++;
	}
	AddPanelWithFixedGap(TeamPanel, 0.0f, 10.0f);

	auto* OperatePanel = CreateCollapsiblePanel(PC, L"队友操作");
	auto* OperateBox = OperatePanel ? OperatePanel->CT_Contents : nullptr;
		GTeammate.AddDD = CreateVideoItemWithOptions(PC, L"添加队友",
			{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammate.AddDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.AddDD);
		else GDynTab.Content6->AddChild(GTeammate.AddDD);
		Count++;
	}
	GTeammate.ReplaceToggle = CreateToggleItem(PC, L"替换指定队友");
	if (GTeammate.ReplaceToggle)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.ReplaceToggle);
		else GDynTab.Content6->AddChild(GTeammate.ReplaceToggle);
		Count++;
	}
		GTeammate.ReplaceDD = CreateVideoItemWithOptions(PC, L"指定队友",
			{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammate.ReplaceDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.ReplaceDD);
		else GDynTab.Content6->AddChild(GTeammate.ReplaceDD);
		Count++;
	}
	AddPanelWithFixedGap(OperatePanel, 0.0f, 8.0f);
}
