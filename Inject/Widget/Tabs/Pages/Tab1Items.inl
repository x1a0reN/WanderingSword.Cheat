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
	GTab1IncludeQuestItemsToggle = nullptr;
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
			else if (wcscmp(Title, L"包括任务物品") == 0) GTab1IncludeQuestItemsToggle = Item;
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
		AddToggle(CoreBox, L"包括任务物品");
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
		AddToggle(nullptr, L"包括任务物品");
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

	GItemGridPanel = static_cast<UUniformGridPanel*>(CreateRawWidget(UUniformGridPanel::StaticClass(), Outer));
	if (GItemGridPanel)
	{
		for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
		{
			GItemSlotButtons[i] = nullptr;
			GItemSlotImages[i] = nullptr;
			GItemSlotQualityBorders[i] = nullptr;
			GItemSlotEntryWidgets[i] = nullptr;
			GItemSlotItemIndices[i] = -1;
			GItemSlotWasPressed[i] = false;
		}

		GItemGridPanel->SetMinDesiredSlotWidth(68.0f);
		GItemGridPanel->SetMinDesiredSlotHeight(84.0f);
		GItemGridPanel->SetSlotPadding(FMargin{ 3.0f, 6.0f, 3.0f, 6.0f });
		if (BrowserBox) BrowserBox->AddChild(GItemGridPanel);
		else Container->AddChild(GItemGridPanel);
		Count++;

		for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
		{
			UButton* Btn = nullptr;
			UImage* Img = nullptr;
			UImage* QualityBorder = nullptr;
			UUserWidget* EntryWidget = nullptr;

			auto* Entry = static_cast<UBPEntry_Item_C*>(
				UWidgetBlueprintLibrary::Create(PC, UBPEntry_Item_C::StaticClass(), PC));
			if (Entry)
			{
				MarkAsGCRoot(Entry);
				EntryWidget = Entry;

				if (Entry->ItemDisplay && Entry->ItemDisplay->CMP)
				{
					auto* Display = Entry->ItemDisplay->CMP;
					Img = static_cast<UImage*>(Display->IMG_Item);
					QualityBorder = static_cast<UImage*>(Display->IMG_QualityBorder);

					if (Display->IMG_SolidBG)
					{
						Display->IMG_SolidBG->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
						Display->IMG_SolidBG->SetColorAndOpacity(FLinearColor{ 0.0f, 0.0f, 0.0f, 0.85f });
					}
					if (Display->TXT_Count)
						Display->TXT_Count->SetVisibility(ESlateVisibility::Collapsed);
				}

				if (Entry->BTN_JHItem && Entry->BTN_JHItem->BtnMain)
					Btn = static_cast<UButton*>(Entry->BTN_JHItem->BtnMain);
			}

			// Fallback to a plain slot only if backpack-style widget creation fails.
			if (!EntryWidget || !Btn || !Img)
			{
				auto* FallbackBtn = static_cast<UButton*>(CreateRawWidget(UButton::StaticClass(), Outer));
				auto* FallbackImg = static_cast<UImage*>(CreateRawWidget(UImage::StaticClass(), Outer));
				if (!FallbackBtn || !FallbackImg)
				{
					GItemSlotButtons[i] = nullptr;
					GItemSlotImages[i] = nullptr;
					GItemSlotQualityBorders[i] = nullptr;
					GItemSlotEntryWidgets[i] = nullptr;
					GItemSlotItemIndices[i] = -1;
					GItemSlotWasPressed[i] = false;
					continue;
				}
				FallbackImg->SetVisibility(ESlateVisibility::Collapsed);
				FallbackBtn->SetContent(FallbackImg);
				Btn = FallbackBtn;
				Img = FallbackImg;
				EntryWidget = nullptr;
			}

			int32 Row = i / ITEM_GRID_COLS;
			int32 Col = i % ITEM_GRID_COLS;
			GItemGridPanel->AddChildToUniformGrid(
				EntryWidget ? static_cast<UWidget*>(EntryWidget) : static_cast<UWidget*>(Btn),
				Row, Col);

			GItemSlotButtons[i] = Btn;
			GItemSlotImages[i] = Img;
			GItemSlotQualityBorders[i] = QualityBorder;
			GItemSlotEntryWidgets[i] = EntryWidget;
			GItemSlotItemIndices[i] = -1;
			GItemSlotWasPressed[i] = false;
		}
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
	RefreshItemPage();
}

namespace
{
    uint32_t GTab1ItemNoDecreaseHookId = UINT32_MAX;
    uintptr_t GItemNoDecreaseOffset = 0;  // 保存搜索到的偏移量
    uint32_t GTab1ItemGainMultiplierHookId = UINT32_MAX;
    uintptr_t GItemGainMultiplierOffset = 0;
    volatile LONG GItemGainMultiplierAsmValue = 2;

    // 物品不减特征码
    const char* kItemNoDecreasePattern = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 41 0F B6 F1 41 8B E8 48 8B FA 48 8B D9";
    const char* kItemGainMultiplierPattern = "0B 50 30 0B 50 2C 0B 50 28";

    // 功能：如果 Num < 0，则设为 0（防止负数扣除）
    const unsigned char kItemNoDecreaseTrampolineCode[] = {
        0x41, 0x83, 0xF8, 0x00,               // cmp r8d, 0
        0x0F, 0x8D, 0x03, 0x00, 0x00, 0x00,   // jge +3
        0x45, 0x31, 0xC0                      // xor r8d, r8d
    };

    // 逻辑：
    // 1) mov r10,[rax+70]
    // 2) cmp byte ptr [r10+84],0   ; 只对指定来源生效
    // 3) je skip
    // 4) mov r11, &GItemGainMultiplierAsmValue
    // 5) mov r11d,[r11]
    // 6) mov [rax+40],r11d
    const unsigned char kItemGainMultiplierTrampolineTemplate[] = {
        0x4C, 0x8B, 0x50, 0x70,                         // mov r10,[rax+70]
        0x41, 0x80, 0xBA, 0x84, 0x00, 0x00, 0x00, 0x00,// cmp byte ptr [r10+84],00
        0x0F, 0x84, 0x11, 0x00, 0x00, 0x00,            // je +0x11
        0x49, 0xBB,                                     // mov r11, imm64
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // imm64 low
        0x00, 0x00,                                     // imm64 high
        0x45, 0x8B, 0x1B,                               // mov r11d,[r11]
        0x44, 0x89, 0x58, 0x40                          // mov [rax+40],r11d
    };
    constexpr size_t kItemGainMulImm64Offset = 20;
}

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
    if (Value > 9999)
        Value = 9999;
    InterlockedExchange(&GItemGainMultiplierAsmValue, static_cast<LONG>(Value));
    LOGI_STREAM("Tab1Items") << "[SDK] ItemGainMultiplier value set to: " << Value << "\n";
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
    const uintptr_t valueAddr = reinterpret_cast<uintptr_t>(&GItemGainMultiplierAsmValue);
    std::memcpy(code + kItemGainMulImm64Offset, &valueAddr, sizeof(valueAddr));

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
            << ", valueAddr=0x" << std::hex << valueAddr << std::dec << "\n";
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
