void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VideoSlot, "Tab1(VideoSlot)");
	if (!Container) return;

	Container->ClearChildren();
	ClearItemBrowserState();
	GTab1ItemNoDecreaseToggle = nullptr;
	GTab1ItemGainMultiplierToggle = nullptr;
	GTab1ItemGainMultiplierSlider = nullptr;
	GTab1AllItemsSellableToggle = nullptr;
	GTab1DropRate100Toggle = nullptr;
	GTab1CraftEffectMultiplierToggle = nullptr;
	GTab1CraftItemIncrementSlider = nullptr;
	GTab1CraftExtraEffectSlider = nullptr;
	GTab1MaxExtraAffixesEdit = nullptr;
	GTab1IgnoreItemUseCountToggle = nullptr;
	GTab1IgnoreItemRequirementsToggle = nullptr;
	int Count = 0;
	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	auto* OptionsPanelRoot = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	UPanelWidget* OptionsBox = OptionsPanelRoot;

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (wcscmp(Title, L"物品不减") == 0) GTab1ItemNoDecreaseToggle = Item;
			else if (wcscmp(Title, L"物品获得加倍") == 0) GTab1ItemGainMultiplierToggle = Item;
			else if (wcscmp(Title, L"所有物品可出售") == 0) GTab1AllItemsSellableToggle = Item;
			else if (wcscmp(Title, L"掉落率100%") == 0) GTab1DropRate100Toggle = Item;
			else if (wcscmp(Title, L"锻造制衣效果加倍") == 0) GTab1CraftEffectMultiplierToggle = Item;
			else if (wcscmp(Title, L"无视物品使用次数") == 0) GTab1IgnoreItemUseCountToggle = Item;
			else if (wcscmp(Title, L"无视物品使用要求") == 0) GTab1IgnoreItemRequirementsToggle = Item;

			if (Box) Box->AddChild(Item);
			else if (OptionsBox) OptionsBox->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (wcscmp(Title, L"加倍倍数") == 0) GTab1ItemGainMultiplierSlider = Item;
			else if (wcscmp(Title, L"道具增量效果倍率") == 0) GTab1CraftItemIncrementSlider = Item;
			else if (wcscmp(Title, L"额外效果倍率") == 0) GTab1CraftExtraEffectSlider = Item;

			// 滑块范围1-10，默认值2
			if (Item->VolumeSlider)
			{
				Item->VolumeSlider->MinValue = 1.0f;
				Item->VolumeSlider->MaxValue = 10.0f;
				Item->VolumeSlider->StepSize = 1.0f;
				Item->VolumeSlider->SetValue(2.0f);
			}

			if (Box) Box->AddChild(Item);
			else if (OptionsBox) OptionsBox->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : (OptionsBox ? OptionsBox : Container), Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (wcscmp(Title, L"最大额外词条数") == 0)
				GTab1MaxExtraAffixesEdit = FindFirstEditableTextBox(Item);

			if (Box) Box->AddChild(Item);
			else if (OptionsBox) OptionsBox->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};

	if (OptionsBox)
	{
		auto AddSubPanel = [&](const wchar_t* Title) -> UPanelWidget*
		{
			auto* Sub = CreateCollapsiblePanel(PC, Title);
			if (!Sub)
				return nullptr;
			OptionsBox->AddChild(Sub);
			Count++;
			return Sub->CT_Contents;
		};

		auto* CoreBox = AddSubPanel(L"基础开关");
		AddToggle(CoreBox, L"物品不减");
		AddToggle(CoreBox, L"物品获得加倍");
		AddToggle(CoreBox, L"所有物品可出售");
		AddToggle(CoreBox, L"掉落率100%");
		AddToggle(CoreBox, L"锻造制衣效果加倍");

		auto* RatioBox = AddSubPanel(L"倍率设置");
		AddSlider(RatioBox, L"加倍倍数");
		AddSlider(RatioBox, L"道具增量效果倍率");
		AddSlider(RatioBox, L"额外效果倍率");

		auto* LimitBox = AddSubPanel(L"限制与词条");
		AddNumeric(LimitBox, L"最大额外词条数", L"3");
		AddToggle(LimitBox, L"无视物品使用次数");
		AddToggle(LimitBox, L"无视物品使用要求");
	}
	else
	{
		AddToggle(nullptr, L"物品不减");
		AddToggle(nullptr, L"物品获得加倍");
		AddSlider(nullptr, L"加倍倍数");
		AddToggle(nullptr, L"所有物品可出售");
		AddToggle(nullptr, L"掉落率100%");
		AddToggle(nullptr, L"锻造制衣效果加倍");
		AddSlider(nullptr, L"道具增量效果倍率");
		AddSlider(nullptr, L"额外效果倍率");
		AddNumeric(nullptr, L"最大额外词条数", L"3");
		AddToggle(nullptr, L"无视物品使用次数");
		AddToggle(nullptr, L"无视物品使用要求");
	}

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

	if (OptionsPanelRoot)
	{
		UPanelSlot* Slot = Container->AddChild(OptionsPanelRoot);
		if (Slot && Slot->IsA(UVerticalBoxSlot::StaticClass()))
		{
			auto* VSlot = static_cast<UVerticalBoxSlot*>(Slot);
			FMargin Pad{};
			Pad.Left = 0.0f;
			Pad.Top = 0.0f;
			Pad.Right = 0.0f;
			Pad.Bottom = 14.0f;
			VSlot->SetPadding(Pad);
		}
		Count++;
	}

	
	auto* BrowserPanel = CreateCollapsiblePanel(PC, L"物品浏览器");
	UPanelWidget* BrowserBox = BrowserPanel ? BrowserPanel->CT_Contents : nullptr;
	if (BrowserBox)
	{
		const int32 OldChildren = BrowserBox->GetChildrenCount();
		if (OldChildren > 0)
		{
			BrowserBox->ClearChildren();
			LOGI_STREAM("Tab1Items")
				<< "[SDK] ItemBrowser panel cleared template children: " << OldChildren << "\n";
		}
	}

	BuildItemCache();
	GItemQuantityEdit = nullptr;
	GItemCategoryDD = CreateVideoItemWithOptions(PC,
		L"\u2501\u2501\u7269\u54C1\u7BA1\u7406\u2501\u2501",
		{ L"\u5168\u90E8", L"\u6B66\u5668", L"\u9632\u5177", L"\u6D88\u8017\u54C1", L"\u5176\u4ED6" });
	GItemLastCatIdx = 0;

	GItemQuantityRow = nullptr;
	if (GItemCategoryDD)
	{
		auto* SearchEdit = static_cast<UEditableTextBox*>(
			CreateRawWidget(UEditableTextBox::StaticClass(), Outer));
		if (SearchEdit)
		{
			SearchEdit->SetHintText(MakeText(L"\u8F93\u5165\u4EE5\u641C\u7D22..."));
			SearchEdit->SetText(MakeText(L""));
			SearchEdit->SetJustification(ETextJustify::Left);
			SearchEdit->MinimumDesiredWidth = 380.0f;
			SearchEdit->SelectAllTextWhenFocused = true;
			SearchEdit->ClearKeyboardFocusOnCommit = false;
			SearchEdit->Font.Size = 16;
			SearchEdit->WidgetStyle.Font.Size = 16;
			SearchEdit->WidgetStyle.Padding.Left = 10.0f;
			SearchEdit->WidgetStyle.Padding.Top = 3.0f;
			SearchEdit->WidgetStyle.Padding.Right = 10.0f;
			SearchEdit->WidgetStyle.Padding.Bottom = 3.0f;
			ClearEditableTextBindings(SearchEdit);

			auto MakeSlateColor = [](float R, float G, float B, float A) -> FSlateColor
			{
				FSlateColor C{};
				C.SpecifiedColor = FLinearColor{ R, G, B, A };
				C.ColorUseRule = ESlateColorStylingMode::UseColor_Specified;
				return C;
			};

			SearchEdit->ForegroundColor = FLinearColor{ 0.95f, 0.95f, 0.95f, 1.0f };
			SearchEdit->BackgroundColor = FLinearColor{ 0.0f, 0.0f, 0.0f, 0.0f };
			SearchEdit->WidgetStyle.ForegroundColor = MakeSlateColor(0.95f, 0.95f, 0.95f, 1.0f);
			SearchEdit->WidgetStyle.BackgroundColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.ReadOnlyForegroundColor = MakeSlateColor(0.75f, 0.75f, 0.75f, 1.0f);
			SearchEdit->WidgetStyle.BackgroundImageNormal.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.BackgroundImageHovered.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.BackgroundImageFocused.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.BackgroundImageReadOnly.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);

			UWidget* SearchWidget = SearchEdit;
			auto* SearchSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
			if (SearchSize)
			{
				SearchSize->SetWidthOverride(310.0f);
				SearchSize->SetHeightOverride(64.0f);
				SearchSize->SetContent(SearchWidget);
				SearchWidget = SearchSize;
			}
			SearchWidget->SetRenderTranslation(FVector2D{ 0.0f, -0.75f });

			if (GItemCategoryDD->TXT_Title)
			{
				GItemCategoryDD->TXT_Title->SetText(MakeText(L""));
				GItemCategoryDD->TXT_Title->SetVisibility(ESlateVisibility::Collapsed);
				UPanelWidget* TitleParent = GItemCategoryDD->TXT_Title->GetParent();
				if (TitleParent)
				{
					UPanelSlot* SearchPanelSlot = nullptr;
					if (TitleParent->IsA(UHorizontalBox::StaticClass()))
					{
						auto* HParent = static_cast<UHorizontalBox*>(TitleParent);
						HParent->RemoveChild(GItemCategoryDD->TXT_Title);

						bool bReaddCombo = false;
						if (GItemCategoryDD->CB_Main && GItemCategoryDD->CB_Main->GetParent() == HParent)
						{
							HParent->RemoveChild(GItemCategoryDD->CB_Main);
							bReaddCombo = true;
						}

						SearchPanelSlot = HParent->AddChildToHorizontalBox(SearchWidget);
						if (bReaddCombo)
						{
							UWidget* ComboWidget = GItemCategoryDD->CB_Main;
							auto* ComboSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
							if (ComboSize && GItemCategoryDD->CB_Main)
							{
								ComboSize->SetWidthOverride(260.0f);
								ComboSize->SetHeightOverride(40.0f);
								ComboSize->SetContent(GItemCategoryDD->CB_Main);
								ComboWidget = ComboSize;
							}

							auto* ComboSlot = HParent->AddChildToHorizontalBox(ComboWidget);
							if (ComboSlot)
							{
								FSlateChildSize AutoSize{};
								AutoSize.SizeRule = ESlateSizeRule::Automatic;
								AutoSize.Value = 0.0f;
								ComboSlot->SetSize(AutoSize);
								ComboSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
								ComboSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
								FMargin ComboPad{};
								ComboPad.Right = 4.0f;
								ComboSlot->SetPadding(ComboPad);
							}
							ComboWidget->SetRenderTranslation(FVector2D{ 2.0f, 0.0f });
						}
					}
					else
					{
						// 非 Horizontal 容器（常见是 Canvas）时，重建一层横向容器承载搜索框和下拉框，
						// 避免原蓝图左侧标题区域固定宽度导致搜索框宽度不生效。
						auto* ReplaceRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
						if (ReplaceRow)
						{
							TitleParent->RemoveChild(GItemCategoryDD->TXT_Title);
							if (GItemCategoryDD->CB_Main && GItemCategoryDD->CB_Main->GetParent() == TitleParent)
								TitleParent->RemoveChild(GItemCategoryDD->CB_Main);

							auto* NewSearchSlot = ReplaceRow->AddChildToHorizontalBox(SearchWidget);
							if (NewSearchSlot)
							{
								FSlateChildSize Fill{};
								Fill.SizeRule = ESlateSizeRule::Fill;
								Fill.Value = 1.0f;
								NewSearchSlot->SetSize(Fill);
								NewSearchSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
								NewSearchSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
								FMargin SearchPad{};
								SearchPad.Right = 10.0f;
								NewSearchSlot->SetPadding(SearchPad);
							}

							if (GItemCategoryDD->CB_Main)
							{
								UWidget* ComboWidget = GItemCategoryDD->CB_Main;
								auto* ComboSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
								if (ComboSize)
								{
									ComboSize->SetWidthOverride(300.0f);
									ComboSize->SetHeightOverride(40.0f);
									ComboSize->SetContent(GItemCategoryDD->CB_Main);
									ComboWidget = ComboSize;
								}

								auto* NewComboSlot = ReplaceRow->AddChildToHorizontalBox(ComboWidget);
								if (NewComboSlot)
								{
									FSlateChildSize AutoSize{};
									AutoSize.SizeRule = ESlateSizeRule::Automatic;
									NewComboSlot->SetSize(AutoSize);
									NewComboSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
									NewComboSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
									FMargin ComboPad{};
									ComboPad.Right = 4.0f;
									NewComboSlot->SetPadding(ComboPad);
								}
								ComboWidget->SetRenderTranslation(FVector2D{ 2.0f, 0.0f });
							}

							SearchPanelSlot = TitleParent->AddChild(ReplaceRow);
						}
						else
						{
							SearchPanelSlot = TitleParent->AddChild(SearchWidget);
						}
					}

					if (SearchPanelSlot && SearchPanelSlot->IsA(UHorizontalBoxSlot::StaticClass()))
					{
						auto* SearchHSlot = static_cast<UHorizontalBoxSlot*>(SearchPanelSlot);
						FSlateChildSize Fill{};
						Fill.SizeRule = ESlateSizeRule::Fill;
						Fill.Value = 1.0f;
						SearchHSlot->SetSize(Fill);
						SearchHSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
						SearchHSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
						FMargin SearchPad{};
						SearchPad.Left = 0.0f;
						SearchPad.Top = 0.0f;
						SearchPad.Right = 10.0f;
						SearchPad.Bottom = 0.0f;
						SearchHSlot->SetPadding(SearchPad);
					}
					else if (SearchPanelSlot && SearchPanelSlot->IsA(UCanvasPanelSlot::StaticClass()))
					{
						auto* SearchCSlot = static_cast<UCanvasPanelSlot*>(SearchPanelSlot);
						SearchCSlot->SetAutoSize(false);
						FAnchors Anchors{};
						Anchors.Minimum = FVector2D{ 0.0f, 0.0f };
						Anchors.Maximum = FVector2D{ 1.0f, 1.0f };
						SearchCSlot->SetAnchors(Anchors);
						FMargin Offsets{};
						Offsets.Left = 0.0f;
						Offsets.Top = 0.0f;
						Offsets.Right = 0.0f;
						Offsets.Bottom = 0.0f;
						SearchCSlot->SetOffsets(Offsets);
						SearchCSlot->SetAlignment(FVector2D{ 0.0f, 0.0f });
					}
				}
			}
		}

		if (BrowserBox) BrowserBox->AddChild(GItemCategoryDD);
		else Container->AddChild(GItemCategoryDD);
		Count++;
	}

	GItemPagerRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
	if (GItemPagerRow)
	{
		UWidget* PrevLayout = nullptr;
		GItemPrevPageBtn = CreateGameStyleButton(PC, L"上一页", "ItemPrevPage",
			136.0f, 48.0f, &PrevLayout);
		if (PrevLayout)
		{
			auto* PrevSlot = GItemPagerRow->AddChildToHorizontalBox(PrevLayout);
			if (PrevSlot)
			{
				PrevSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
				PrevSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			}
		}

		GItemPageLabel = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
		if (GItemPageLabel)
		{
			GItemPageLabel->SetText(MakeText(L"1/1"));
			GItemPageLabel->SetJustification(ETextJustify::Center);
			GItemPageLabel->SetMinDesiredWidth(92.0f);
			GItemPageLabel->Font.Size = 18;
			auto* LabelSlot = GItemPagerRow->AddChildToHorizontalBox(GItemPageLabel);
			if (LabelSlot)
			{
				LabelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
				LabelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			}
		}

		UWidget* NextLayout = nullptr;
		GItemNextPageBtn = CreateGameStyleButton(PC, L"下一页", "ItemNextPage",
			136.0f, 48.0f, &NextLayout);
		if (NextLayout)
		{
			auto* NextSlot = GItemPagerRow->AddChildToHorizontalBox(NextLayout);
			if (NextSlot)
			{
				NextSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
				NextSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			}
		}
	}

	GItemGridPanel = nullptr;
	GItemListView = nullptr;
	for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
	{
		GItemSlotButtons[i] = nullptr;
		GItemSlotImages[i] = nullptr;
		GItemSlotQualityBorders[i] = nullptr;
		GItemSlotEntryWidgets[i] = nullptr;
		GItemSlotItemIndices[i] = -1;
		GItemSlotWasPressed[i] = false;
	}

	UWidget* GridRootWidget = nullptr;
	UListView* BuiltListView = nullptr;
	bool bUsingPlainTile = false;

	// 使用 BP_ItemGridWDT，确保走游戏原生 WDT 初始化与数据绑定链路。
	auto* ItemGridTemplate = static_cast<UObject*>(UBP_ItemGridWDT_C::GetDefaultObj());
	auto* ItemGrid = static_cast<UBP_ItemGridWDT_C*>(
		CreateRawWidgetFromTemplate(
			UBP_ItemGridWDT_C::StaticClass(),
			Outer,
			ItemGridTemplate,
			"Tab1.ItemGridWDT"));
	if (!ItemGrid)
		ItemGrid = static_cast<UBP_ItemGridWDT_C*>(CreateRawWidget(UBP_ItemGridWDT_C::StaticClass(), Outer));
	if (ItemGrid && IsSafeLiveObject(static_cast<UObject*>(ItemGrid)))
	{
		ItemGrid->EVT_InitOnce();
		GridRootWidget = static_cast<UWidget*>(ItemGrid);
		BuiltListView = static_cast<UListView*>(ItemGrid);
	}

	if (GridRootWidget && BuiltListView)
	{
		GItemGridPanel = GridRootWidget;
		GItemListView = BuiltListView;
		MarkAsGCRoot(static_cast<UObject*>(GridRootWidget));

		// 强制指定 Entry 类，避免错误 entry blueprint 导致显示错乱。
		UClass* EntryCls = UObject::FindClassFast("BPEntry_Item_WDT_C");
		if (!EntryCls)
			EntryCls = UObject::FindClass("BPEntry_Item_WDT_C");
		if (EntryCls)
			GItemListView->EntryWidgetClass = EntryCls;

		GItemListView->SetSelectionMode(ESelectionMode::Single);
		GItemListView->BP_ClearSelection();
		if (IsSafeLiveObjectOfClass(static_cast<UObject*>(GItemListView), UTileView::StaticClass()))
		{
			auto* Tile = static_cast<UTileView*>(GItemListView);
			Tile->SetEntryWidth(72.0f);
			Tile->SetEntryHeight(88.0f);
		}

		UWidget* GridHostWidget = GridRootWidget;
		auto* GridHostSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
		if (GridHostSize)
		{
			// TileView 在 VerticalBox 中若无稳定高度，容易布局塌缩为 0，导致不生成可见 Entry。
			GridHostSize->SetHeightOverride(420.0f);
			GridHostSize->SetContent(GridRootWidget);
			GridHostWidget = static_cast<UWidget*>(GridHostSize);
		}

		if (BrowserBox) BrowserBox->AddChild(GridHostWidget);
		else Container->AddChild(GridHostWidget);
		Count++;
		LOGI_STREAM("Tab1Items")
			<< "[SDK] ItemGrid created: widget=0x" << std::hex << reinterpret_cast<uintptr_t>(GridRootWidget)
			<< " listView=0x" << std::hex << reinterpret_cast<uintptr_t>(GItemListView)
			<< " source=" << (bUsingPlainTile ? "PlainTileView" : "BP_ItemGridWDT")
			<< " entryClass=0x" << std::hex
			<< reinterpret_cast<uintptr_t>(GItemListView ? GItemListView->EntryWidgetClass.Get() : nullptr)
			<< " entryClassName="
			<< ((GItemListView && GItemListView->EntryWidgetClass.Get())
				? GItemListView->EntryWidgetClass.Get()->GetName().c_str()
				: "null")
			<< std::dec << "\n";
	}
	else
	{
		LOGE_STREAM("Tab1Items") << "[SDK] ItemGrid create failed (BP_ItemGridWDT_C)\n";
	}

	if (GItemPagerRow)
	{
		UPanelSlot* PagerSlot = nullptr;
		if (BrowserBox)
			PagerSlot = BrowserBox->AddChild(GItemPagerRow);
		else
			PagerSlot = Container->AddChild(GItemPagerRow);

		if (PagerSlot && PagerSlot->IsA(UVerticalBoxSlot::StaticClass()))
		{
			auto* VSlot = static_cast<UVerticalBoxSlot*>(PagerSlot);
			VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
			VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			FMargin Pad{};
			Pad.Left = 0.0f;
			Pad.Top = 14.0f;
			Pad.Right = 12.0f;
			Pad.Bottom = 0.0f;
			VSlot->SetPadding(Pad);
		}
		Count++;
	}

	AddPanelWithFixedGap(BrowserPanel, 0.0f, 8.0f);

	GItemCurrentPage = 0;
	FilterItems(0);
	LOGI_STREAM("Tab1Items") << "[SDK] ItemGrid init deferred: wait Tab8 shown then refresh\n";
}

namespace
{
    uint32_t GTab1ItemNoDecreaseHookId = UINT32_MAX;
    uintptr_t GItemNoDecreaseOffset = 0;  // 保存搜索到的偏移量
    uint32_t GTab1ItemGainMultiplierHookId = UINT32_MAX;
    uintptr_t GItemGainMultiplierOffset = 0;
    volatile LONG GItemGainMultiplierAsmValue = 2;
    uint32_t GTab1CraftCaptureHookId = UINT32_MAX;
    uint32_t GTab1CraftEffectHookId = UINT32_MAX;
    uint32_t GTab1CraftRandEffectHookId = UINT32_MAX;
    uintptr_t GInForgingOffset = 0;
    uintptr_t GForgeEffectOffset = 0;
    uintptr_t GForgeRandEffectOffset = 0;
    volatile LONG GCraftEffectModeAsmValue = 0;
    volatile float GCraftItemIncrementAsmValue = 2.0f;
    volatile float GCraftExtraEffectAsmValue = 2.0f;

    // 所有物品可出售
    uintptr_t GAllItemsSellableAddr = 0;
    uintptr_t GIncludeQuestItemsAddr = 0;
    uintptr_t GIncludeQuestItemsAddr2 = 0;
    uintptr_t GDropRate100Addr = 0;

    // 物品不减特征码
    const char* kItemNoDecreasePattern = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 41 0F B6 F1 41 8B E8 48 8B FA 48 8B D9";
    const char* kItemGainMultiplierPattern = "0B 50 30 0B 50 2C 0B 50 28";
    const char* kInForgingPattern = "0F 10 00 0F 11 46 28 44 89";
    const char* kForgeEffectPattern = "E8 ? ? ? ? F2 0F 10 ? 44 8B 78 08 F2 0F 11 ? 24";
    const char* kForgeRandEffectPattern = "0F 29 ? 24 ? 0F 28 ? 0F 29 ? 24 ? F2 0F 10";

    // 所有物品可出售特征码
    const char* kAllItemsSellablePattern = "80 ?? ?? 3C 75 ?? 32 C0 C3 80 ?? 83 00 00 00 00 0F 94 C0 C3";
    // 任务物品可出售特征码
    const char* kQuestItemsSellablePattern = "48 8B ?? 70 80 78 40 3C";
    // 掉落率100%（DropItem + A: 84 C0 -> 90 90）
    const char* kDropRate100Pattern = "F3 0F 2C ? 04 E8 ? ? ? ? 84 C0 74 ? 48";

    // 功能：如果 Num < 0，则设为 0（防止负数扣除）
    const unsigned char kItemNoDecreaseTrampolineCode[] = {
        0x41, 0x83, 0xF8, 0x00,               // cmp r8d, 0
        0x0F, 0x8D, 0x03, 0x00, 0x00, 0x00,   // jge +3
        0x45, 0x31, 0xC0                      // xor r8d, r8d
    };

    const unsigned char kItemGainMultiplierTrampolineTemplate[] = {
        0x41, 0x50,                                      // push r8
        0x4C, 0x8B, 0x40, 0x70,                          // mov r8,[rax+70]
        0x41, 0x80, 0xB8, 0x84, 0x00, 0x00, 0x00, 0x00, // cmp byte ptr [r8+84],00
        0x0F, 0x84, 0x15, 0x00, 0x00, 0x00,             // je +0x15 (jump to pop r8)
        0x41, 0x53,                                      // push r11
        0x49, 0xBB,                                      // mov r11, imm64
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // imm64 low
        0x00, 0x00,                                      // imm64 high
        0x45, 0x8B, 0x1B,                                // mov r11d,[r11]
        0x44, 0x89, 0x58, 0x40,                          // mov [rax+40],r11d
        0x41, 0x5B,                                      // pop r11
        0x41, 0x58                                       // pop r8
    };
    // Template offset of imm64 in "49 BB imm64"
    constexpr size_t kItemGainMulImm64Offset = 24;

    const unsigned char kCraftCaptureTrampolineTemplate[] = {
        0x41, 0x53,                                     // push r11
        0x49, 0xBB,                                     // mov r11, imm64
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            // imm64 low
        0x00, 0x00,                                     // imm64 high
        0x45, 0x89, 0x3B,                               // mov [r11], r15d
        0x41, 0x5B                                      // pop r11
    };
    constexpr size_t kCraftCaptureCtxImm64Offset = 4;

    const unsigned char kCraftEffectTrampolineTemplate[] = {
        0x41, 0x53,                                     // push r11
        0x49, 0xBB,                                     // mov r11, modeAddr
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            // imm64 low
        0x00, 0x00,                                     // imm64 high
        0x41, 0x83, 0x3B, 0x02,                         // cmp dword ptr [r11], 2
        0x75, 0x13,                                     // jne +0x13 (skip to pop r11)
        0xD9, 0x40, 0x08,                               // fld dword ptr [rax+8]
        0x49, 0xBB,                                     // mov r11, incrementAddr
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            // imm64 low
        0x00, 0x00,                                     // imm64 high
        0x41, 0xD8, 0x0B,                               // fmul dword ptr [r11]
        0xD9, 0x58, 0x08,                               // fstp dword ptr [rax+8]
        0x41, 0x5B                                      // pop r11
    };
    constexpr size_t kCraftEffectModeImm64Offset = 4;
    constexpr size_t kCraftEffectIncImm64Offset = 23;

    const unsigned char kCraftRandEffectTrampolineTemplate[] = {
        0x41, 0x53,                                     // push r11
        0x49, 0xBB,                                     // mov r11, modeAddr
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            // imm64 low
        0x00, 0x00,                                     // imm64 high
        0x41, 0x83, 0x3B, 0x02,                         // cmp dword ptr [r11], 2
        0x75, 0x19,                                     // jne +0x19
        0x83, 0x39, 0x05,                               // cmp dword ptr [rcx], 5
        0x74, 0x14,                                     // je +0x14
        0x83, 0x39, 0x06,                               // cmp dword ptr [rcx], 6
        0x74, 0x0F,                                     // je +0x0F
        0x49, 0xBB,                                     // mov r11, extraAddr
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            // imm64 low
        0x00, 0x00,                                     // imm64 high
        0xF3, 0x41, 0x0F, 0x10, 0x13,                   // movss xmm2, dword ptr [r11]
        0x41, 0x5B                                      // pop r11
    };
    constexpr size_t kCraftRandModeImm64Offset = 4;
    constexpr size_t kCraftRandExtraImm64Offset = 30;

    uintptr_t ScanModulePatternRobust(const char* moduleName, const char* pattern)
    {
        if (!moduleName || !pattern)
            return 0;

        // 1) 优先扫可执行段
        uintptr_t addr = InlineHook::HookManager::AobScanModuleFirst(moduleName, pattern, true);
        if (addr != 0)
            return addr;

        // 2) 再扫模块可读段
        addr = InlineHook::HookManager::AobScanModuleFirst(moduleName, pattern, false);
        if (addr != 0)
            return addr;

        // 3) 回退：按模块地址范围做通用扫描（绕过模块节区过滤逻辑）
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
}

void EnableItemGainMultiplierHook();
void DisableItemGainMultiplierHook();
void EnableDropRate100Patch();
void DisableDropRate100Patch();
void SetCraftItemIncrementHookValue(float Value);
void SetCraftExtraEffectHookValue(float Value);
void EnableCraftEffectMultiplierHook();
void DisableCraftEffectMultiplierHook();

void EnableItemNoDecreaseHook()
{
    if (GTab1ItemNoDecreaseHookId != UINT32_MAX)
        return;

    // 第一次开启时搜索特征码获取偏移量
    if (GItemNoDecreaseOffset == 0)
    {
        uintptr_t foundAddr = InlineHook::HookManager::AobScanModuleFirst("JH-Win64-Shipping.exe", kItemNoDecreasePattern);
        if (foundAddr == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] ItemNoDecrease AobScan failed, pattern not found\n";
            return;
        }

        // 计算模块内偏移
        HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
        if (!hModule)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] ItemNoDecrease failed to get module handle\n";
            return;
        }

        uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);
        GItemNoDecreaseOffset = foundAddr - moduleBase;
        LOGI_STREAM("Tab1Items") << "[SDK] ItemNoDecrease found at: 0x" << std::hex << foundAddr
            << ", offset: 0x" << GItemNoDecreaseOffset << std::dec << "\n";
    }

    uint32_t hookId = UINT32_MAX;
    const bool success = InlineHook::HookManager::InstallHook(
        "JH-Win64-Shipping.exe",
        GItemNoDecreaseOffset,
        kItemNoDecreaseTrampolineCode,
        sizeof(kItemNoDecreaseTrampolineCode),
        hookId
    );

    if (success && hookId != UINT32_MAX)
    {
        GTab1ItemNoDecreaseHookId = hookId;
        LOGI_STREAM("Tab1Items") << "[SDK] ItemNoDecrease hook enabled, ID: " << hookId << "\n";
    }
    else
    {
        LOGI_STREAM("Tab1Items") << "[SDK] ItemNoDecrease hook failed\n";
    }
}

void DisableItemNoDecreaseHook()
{
    if (GTab1ItemNoDecreaseHookId == UINT32_MAX)
        return;

    const bool success = InlineHook::HookManager::UninstallHook(GTab1ItemNoDecreaseHookId);
    if (success)
    {
        LOGI_STREAM("Tab1Items") << "[SDK] ItemNoDecrease hook disabled\n";
    }
    else
    {
        LOGI_STREAM("Tab1Items") << "[SDK] ItemNoDecrease hook disable failed\n";
    }

    GTab1ItemNoDecreaseHookId = UINT32_MAX;
}

void SetItemGainMultiplierHookValue(int32 Value)
{
    if (Value < 1)
        Value = 1;
    if (Value > 10)
        Value = 10;
    const LONG oldValue = InterlockedExchange(&GItemGainMultiplierAsmValue, static_cast<LONG>(Value));
    if (oldValue == Value)
        return;

    LOGI_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier value set to: " << Value << "\n";
}

void SetCraftItemIncrementHookValue(float Value)
{
    if (Value < 1.0f)
        Value = 1.0f;
    if (Value > 10.0f)
        Value = 10.0f;

    const float diff = static_cast<float>(GCraftItemIncrementAsmValue) - Value;
    if (diff >= -0.001f && diff <= 0.001f)
        return;

    GCraftItemIncrementAsmValue = Value;
    LOGI_STREAM("Tab1Items") << "[SDK] CraftItemIncrement value set to: " << Value << "\n";
}

void SetCraftExtraEffectHookValue(float Value)
{
    if (Value < 1.0f)
        Value = 1.0f;
    if (Value > 10.0f)
        Value = 10.0f;

    const float diff = static_cast<float>(GCraftExtraEffectAsmValue) - Value;
    if (diff >= -0.001f && diff <= 0.001f)
        return;

    GCraftExtraEffectAsmValue = Value;
    LOGI_STREAM("Tab1Items") << "[SDK] CraftExtraEffect value set to: " << Value << "\n";
}

void EnableItemGainMultiplierHook()
{
    if (GTab1ItemGainMultiplierHookId != UINT32_MAX)
        return;

    if (GItemGainMultiplierOffset == 0)
    {
        const uintptr_t foundAddr = InlineHook::HookManager::AobScanModuleFirst(
            "JH-Win64-Shipping.exe",
            kItemGainMultiplierPattern,
            true);
        if (foundAddr == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier AobScan failed, pattern not found\n";
            return;
        }

        HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
        if (!hModule)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier failed to get module handle\n";
            return;
        }

        const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(hModule);
        GItemGainMultiplierOffset = foundAddr - moduleBase;
        LOGI_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier found at: 0x" << std::hex << foundAddr
            << ", offset: 0x" << GItemGainMultiplierOffset << std::dec << "\n";
    }

    unsigned char code[sizeof(kItemGainMultiplierTrampolineTemplate)] = {};
    std::memcpy(code, kItemGainMultiplierTrampolineTemplate, sizeof(code));
    const uintptr_t gainValueAddr = reinterpret_cast<uintptr_t>(&GItemGainMultiplierAsmValue);
    std::memcpy(code + kItemGainMulImm64Offset, &gainValueAddr, sizeof(gainValueAddr));

    uint32_t hookId = UINT32_MAX;
    const bool success = InlineHook::HookManager::InstallHook(
        "JH-Win64-Shipping.exe",
        static_cast<uint32_t>(GItemGainMultiplierOffset),
        code,
        sizeof(code),
        hookId
    );

    if (success && hookId != UINT32_MAX)
    {
        GTab1ItemGainMultiplierHookId = hookId;
        LOGI_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier hook enabled, ID: " << hookId
            << ", valueAddr=0x" << std::hex << gainValueAddr << std::dec << "\n";
    }
    else
    {
        LOGE_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier hook failed\n";
    }
}

void DisableItemGainMultiplierHook()
{
    if (GTab1ItemGainMultiplierHookId == UINT32_MAX)
        return;

    const bool success = InlineHook::HookManager::UninstallHook(GTab1ItemGainMultiplierHookId);
    if (success)
    {
        LOGI_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier hook disabled\n";
    }
    else
    {
        LOGE_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier hook disable failed\n";
    }

    GTab1ItemGainMultiplierHookId = UINT32_MAX;
}

// 所有物品可出售（合并任务物品）
void EnableAllItemsSellable()
{
    uintptr_t foundAddr = 0;
    uintptr_t foundAddr2 = 0;

    // 1) 搜索所有物品可出售特征码
    if (GAllItemsSellableAddr == 0)
    {
        foundAddr = ScanModulePatternRobust("JH-Win64-Shipping.exe", kAllItemsSellablePattern);
        if (foundAddr == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] AllItemsSellable AobScan failed (pattern可能已随版本变化)\n";
            return;
        }
        GAllItemsSellableAddr = foundAddr + 0x10;
        LOGI_STREAM("Tab1Items") << "[SDK] AllItemsSellable found at: 0x" << std::hex << GAllItemsSellableAddr << std::dec << "\n";
    }
    else
    {
        // 已缓存地址，直接使用
        foundAddr = GAllItemsSellableAddr - 0x10;
    }

    // ENABLE: mov al, 1; nop
    const unsigned char enableBytes[] = { 0x0C, 0x01, 0x90 };
    InlineHook::HookManager::WriteMemory(GAllItemsSellableAddr, enableBytes, sizeof(enableBytes));
    LOGI_STREAM("Tab1Items") << "[SDK] AllItemsSellable enabled\n";

    // 2) 搜索任务物品可出售特征码并启用
    if (GIncludeQuestItemsAddr == 0)
    {
        foundAddr2 = ScanModulePatternRobust("JH-Win64-Shipping.exe", kQuestItemsSellablePattern);
        if (foundAddr2 == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] QuestItemsSellable AobScan failed\n";
        }
        else
        {
            GIncludeQuestItemsAddr = foundAddr2 + 0x6;
            GIncludeQuestItemsAddr2 = foundAddr2 + 0x7;
            LOGI_STREAM("Tab1Items") << "[SDK] QuestItemsSellable found at: 0x" << std::hex << foundAddr2 << std::dec << "\n";
        }
    }

    // ENABLE: 任务物品可出售
    if (GIncludeQuestItemsAddr != 0)
    {
        const unsigned char enableByte1[] = { 0x0C, 0x01 };
        const unsigned char enableByte2[] = { 0xFF };
        InlineHook::HookManager::WriteMemory(GIncludeQuestItemsAddr, enableByte1, sizeof(enableByte1));
        InlineHook::HookManager::WriteMemory(GIncludeQuestItemsAddr2, enableByte2, sizeof(enableByte2));
        LOGI_STREAM("Tab1Items") << "[SDK] QuestItemsSellable enabled\n";
    }
}

void DisableAllItemsSellable()
{
    if (GAllItemsSellableAddr == 0)
        return;

    // DISABLE: setz al
    const unsigned char disableBytes[] = { 0x0F, 0x94, 0xC0 };
    InlineHook::HookManager::WriteMemory(GAllItemsSellableAddr, disableBytes, sizeof(disableBytes));
    LOGI_STREAM("Tab1Items") << "[SDK] AllItemsSellable disabled\n";

    // DISABLE: 任务物品可出售
    if (GIncludeQuestItemsAddr != 0)
    {
        const unsigned char disableByte1[] = { 0x32, 0xC0 };
        const unsigned char disableByte2[] = { 0x3C };
        InlineHook::HookManager::WriteMemory(GIncludeQuestItemsAddr, disableByte1, sizeof(disableByte1));
        InlineHook::HookManager::WriteMemory(GIncludeQuestItemsAddr2, disableByte2, sizeof(disableByte2));
        LOGI_STREAM("Tab1Items") << "[SDK] QuestItemsSellable disabled\n";
    }
}

void EnableDropRate100Patch()
{
    if (GDropRate100Addr == 0)
    {
        const uintptr_t foundAddr = ScanModulePatternRobust("JH-Win64-Shipping.exe", kDropRate100Pattern);
        if (foundAddr == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] DropRate100 AobScan failed, pattern not found\n";
            return;
        }

        // CE: DropItem + A
        GDropRate100Addr = foundAddr + 0xA;
        LOGI_STREAM("Tab1Items") << "[SDK] DropRate100 found at: 0x" << std::hex << foundAddr
            << ", patch=0x" << GDropRate100Addr << std::dec << "\n";
    }

    const unsigned char enableBytes[] = { 0x90, 0x90 };
    if (!InlineHook::HookManager::WriteMemory(GDropRate100Addr, enableBytes, sizeof(enableBytes)))
    {
        LOGE_STREAM("Tab1Items") << "[SDK] DropRate100 enable write failed at: 0x"
            << std::hex << GDropRate100Addr << std::dec << "\n";
        return;
    }

    LOGI_STREAM("Tab1Items") << "[SDK] DropRate100 enabled\n";
}

void DisableDropRate100Patch()
{
    if (GDropRate100Addr == 0)
        return;

    const unsigned char disableBytes[] = { 0x84, 0xC0 };
    if (!InlineHook::HookManager::WriteMemory(GDropRate100Addr, disableBytes, sizeof(disableBytes)))
    {
        LOGE_STREAM("Tab1Items") << "[SDK] DropRate100 disable write failed at: 0x"
            << std::hex << GDropRate100Addr << std::dec << "\n";
        return;
    }

    LOGI_STREAM("Tab1Items") << "[SDK] DropRate100 disabled\n";
}

void EnableCraftEffectMultiplierHook()
{
    if (GTab1CraftCaptureHookId != UINT32_MAX &&
        GTab1CraftEffectHookId != UINT32_MAX &&
        GTab1CraftRandEffectHookId != UINT32_MAX)
    {
        return;
    }

    if (GInForgingOffset == 0)
    {
        const uintptr_t foundAddr = ScanModulePatternRobust("JH-Win64-Shipping.exe", kInForgingPattern);
        if (foundAddr == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect InForging AobScan failed\n";
            return;
        }

        HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
        if (!hModule)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect InForging failed to get module handle\n";
            return;
        }

        GInForgingOffset = foundAddr - reinterpret_cast<uintptr_t>(hModule);
        LOGI_STREAM("Tab1Items") << "[SDK] CraftEffect InForging found at: 0x" << std::hex << foundAddr
            << ", offset: 0x" << GInForgingOffset << std::dec << "\n";
    }

    if (GForgeEffectOffset == 0)
    {
        const uintptr_t foundAddr = ScanModulePatternRobust("JH-Win64-Shipping.exe", kForgeEffectPattern);
        if (foundAddr == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeEffect AobScan failed\n";
            return;
        }

        HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
        if (!hModule)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeEffect failed to get module handle\n";
            return;
        }

        GForgeEffectOffset = foundAddr - reinterpret_cast<uintptr_t>(hModule);
        LOGI_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeEffect found at: 0x" << std::hex << foundAddr
            << ", offset: 0x" << GForgeEffectOffset << std::dec << "\n";
    }

    if (GForgeRandEffectOffset == 0)
    {
        const uintptr_t foundAddr = ScanModulePatternRobust("JH-Win64-Shipping.exe", kForgeRandEffectPattern);
        if (foundAddr == 0)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeRandEffect AobScan failed\n";
            return;
        }

        HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
        if (!hModule)
        {
            LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeRandEffect failed to get module handle\n";
            return;
        }

        GForgeRandEffectOffset = foundAddr - reinterpret_cast<uintptr_t>(hModule);
        LOGI_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeRandEffect found at: 0x" << std::hex << foundAddr
            << ", offset: 0x" << GForgeRandEffectOffset << std::dec << "\n";
    }

    unsigned char captureCode[sizeof(kCraftCaptureTrampolineTemplate)] = {};
    std::memcpy(captureCode, kCraftCaptureTrampolineTemplate, sizeof(captureCode));
    const uintptr_t modeAddr = reinterpret_cast<uintptr_t>(&GCraftEffectModeAsmValue);
    std::memcpy(captureCode + kCraftCaptureCtxImm64Offset, &modeAddr, sizeof(modeAddr));

    unsigned char effectCode[sizeof(kCraftEffectTrampolineTemplate)] = {};
    std::memcpy(effectCode, kCraftEffectTrampolineTemplate, sizeof(effectCode));
    const uintptr_t incrementAddr = reinterpret_cast<uintptr_t>(&GCraftItemIncrementAsmValue);
    std::memcpy(effectCode + kCraftEffectModeImm64Offset, &modeAddr, sizeof(modeAddr));
    std::memcpy(effectCode + kCraftEffectIncImm64Offset, &incrementAddr, sizeof(incrementAddr));

    unsigned char randEffectCode[sizeof(kCraftRandEffectTrampolineTemplate)] = {};
    std::memcpy(randEffectCode, kCraftRandEffectTrampolineTemplate, sizeof(randEffectCode));
    const uintptr_t extraAddr = reinterpret_cast<uintptr_t>(&GCraftExtraEffectAsmValue);
    std::memcpy(randEffectCode + kCraftRandModeImm64Offset, &modeAddr, sizeof(modeAddr));
    std::memcpy(randEffectCode + kCraftRandExtraImm64Offset, &extraAddr, sizeof(extraAddr));

    uint32_t captureHookId = UINT32_MAX;
    if (!InlineHook::HookManager::InstallHook(
        "JH-Win64-Shipping.exe",
        static_cast<uint32_t>(GInForgingOffset),
        captureCode,
        sizeof(captureCode),
        captureHookId,
        false,
        false))
    {
        LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect InForging hook failed\n";
        return;
    }

    uint32_t effectHookId = UINT32_MAX;
    if (!InlineHook::HookManager::InstallHook(
        "JH-Win64-Shipping.exe",
        static_cast<uint32_t>(GForgeEffectOffset + 0x5),
        effectCode,
        sizeof(effectCode),
        effectHookId,
        true,
        false))
    {
        InlineHook::HookManager::UninstallHook(captureHookId);
        LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeEffect hook failed\n";
        return;
    }

    uint32_t randEffectHookId = UINT32_MAX;
    if (!InlineHook::HookManager::InstallHook(
        "JH-Win64-Shipping.exe",
        static_cast<uint32_t>(GForgeRandEffectOffset),
        randEffectCode,
        sizeof(randEffectCode),
        randEffectHookId,
        false,
        false))
    {
        InlineHook::HookManager::UninstallHook(effectHookId);
        InlineHook::HookManager::UninstallHook(captureHookId);
        LOGE_STREAM("Tab1Items") << "[SDK] CraftEffect ForgeRandEffect hook failed\n";
        return;
    }

    InterlockedExchange(&GCraftEffectModeAsmValue, 0);
    GTab1CraftCaptureHookId = captureHookId;
    GTab1CraftEffectHookId = effectHookId;
    GTab1CraftRandEffectHookId = randEffectHookId;

    LOGI_STREAM("Tab1Items") << "[SDK] CraftEffect hooks enabled, IDs: "
        << captureHookId << ", " << effectHookId << ", " << randEffectHookId << "\n";
}

void DisableCraftEffectMultiplierHook()
{
    if (GTab1CraftRandEffectHookId != UINT32_MAX)
    {
        InlineHook::HookManager::UninstallHook(GTab1CraftRandEffectHookId);
        GTab1CraftRandEffectHookId = UINT32_MAX;
    }
    if (GTab1CraftEffectHookId != UINT32_MAX)
    {
        InlineHook::HookManager::UninstallHook(GTab1CraftEffectHookId);
        GTab1CraftEffectHookId = UINT32_MAX;
    }
    if (GTab1CraftCaptureHookId != UINT32_MAX)
    {
        InlineHook::HookManager::UninstallHook(GTab1CraftCaptureHookId);
        GTab1CraftCaptureHookId = UINT32_MAX;
    }

    InterlockedExchange(&GCraftEffectModeAsmValue, 0);
    LOGI_STREAM("Tab1Items") << "[SDK] CraftEffect hooks disabled\n";
}

