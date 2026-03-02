void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent7) return;
	GDynTabContent7->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GQuestToggle = nullptr;
	GQuestTypeDD = nullptr;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTabContent7->AddChild(Panel);
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
	GQuestToggle = CreateToggleItem(PC, L"接到/完成任务");
	if (GQuestToggle)
	{
		if (MainBox) MainBox->AddChild(GQuestToggle);
		else GDynTabContent7->AddChild(GQuestToggle);
		Count++;
	}
	GQuestTypeDD = CreateVideoItemWithOptions(PC, L"执行类型", { L"接到", L"完成" });
	if (GQuestTypeDD)
	{
		if (MainBox) MainBox->AddChild(GQuestTypeDD);
		else GDynTabContent7->AddChild(GQuestTypeDD);
		Count++;
	}
	AddPanelWithFixedGap(MainPanel, 0.0f, 10.0f);

	auto* ArgPanel = CreateCollapsiblePanel(PC, L"任务参数");
	auto* ArgBox = ArgPanel ? ArgPanel->CT_Contents : nullptr;
	auto* QuestIdItem = CreateVolumeNumericEditBoxItem(PC, Outer, ArgBox ? ArgBox : GDynTabContent7, L"任务ID", L"输入数字", L"1");
	if (QuestIdItem)
	{
		if (ArgBox) ArgBox->AddChild(QuestIdItem);
		else GDynTabContent7->AddChild(QuestIdItem);
		Count++;
	}
	AddPanelWithFixedGap(ArgPanel, 0.0f, 8.0f);
}

void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent8 || !CV || !PC) return;
	GDynTabContent8->ClearChildren();

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
		: static_cast<UObject*>(CV);
	UWidget* BtnLayout = nullptr;
	auto* ResetBtn = CreateGameStyleButton(PC, L"下一页", "Tab8Showcase",
		0.0f, 0.0f, &BtnLayout);
	if (BtnLayout)
	{
		GDynTabContent8->AddChild(BtnLayout);
	}
}
