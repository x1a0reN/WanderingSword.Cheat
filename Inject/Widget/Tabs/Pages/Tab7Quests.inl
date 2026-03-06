void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTab.Content7) return;
	GDynTab.Content7->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GQuest.Toggle = nullptr;
	GQuest.TypeDD = nullptr;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTab.Content7->AddChild(Panel);
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

	auto* MainPanel = CreateCollapsiblePanel(PC, L"任务执行");
	auto* MainBox = MainPanel ? MainPanel->CT_Contents : nullptr;
	GQuest.Toggle = CreateToggleItem(PC, L"接到/完成任务");
	if (GQuest.Toggle)
	{
		if (MainBox) MainBox->AddChild(GQuest.Toggle);
		else GDynTab.Content7->AddChild(GQuest.Toggle);
		Count++;
	}
	GQuest.TypeDD = CreateVideoItemWithOptions(PC, L"执行类型", { L"接到", L"完成" });
	if (GQuest.TypeDD)
	{
		if (MainBox) MainBox->AddChild(GQuest.TypeDD);
		else GDynTab.Content7->AddChild(GQuest.TypeDD);
		Count++;
	}
	AddPanelWithFixedGap(MainPanel, 0.0f, 10.0f);

	auto* ArgPanel = CreateCollapsiblePanel(PC, L"任务参数");
	auto* ArgBox = ArgPanel ? ArgPanel->CT_Contents : nullptr;
	auto* QuestIdItem = CreateVolumeNumericEditBoxItem(PC, Outer, ArgBox ? ArgBox : GDynTab.Content7, L"任务ID", L"输入数字", L"1");
	if (QuestIdItem)
	{
		if (ArgBox) ArgBox->AddChild(QuestIdItem);
		else GDynTab.Content7->AddChild(QuestIdItem);
		Count++;
	}
	AddPanelWithFixedGap(ArgPanel, 0.0f, 8.0f);
}

void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTab.Content8 || !CV || !PC) return;
	GDynTab.Content8->ClearChildren();

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
		: static_cast<UObject*>(CV);
	UWidget* BtnLayout = nullptr;
	auto* ResetBtn = CreateGameStyleButton(PC, L"下一页", "Tab8Showcase",
		0.0f, 0.0f, &BtnLayout);
	if (BtnLayout)
	{
		GDynTab.Content8->AddChild(BtnLayout);
	}
}
