#include <iostream>
#include "TabContent.hpp"
#include "GCManager.hpp"
#include "ItemBrowser.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"
#include "SDK/BPEntry_Item_classes.hpp"
UPanelWidget* GetOrCreateSlotContainer(UBPMV_ConfigView2_C* CV, UNeoUINamedSlot* Slot, const char* SlotName)
{
	if (!Slot)
	{
		std::cout << "[SDK] " << SlotName << ": slot pointer is null\n";
		return nullptr;
	}

	
	
	
	int childCount = Slot->GetChildrenCount();
	std::cout << "[SDK] " << SlotName << ": ptr=" << (void*)Slot << " children=" << childCount << "\n";
	while (Slot->GetChildrenCount() > 0)
	{
		UWidget* Child = Slot->GetChildAt(0);
		if (Child)
		{
			
			if (Slot == CV->LanSlot && !GOriginalLanPanel)
			{
				GOriginalLanPanel = Child;
				MarkAsGCRoot(GOriginalLanPanel);
				std::cout << "[SDK] Captured original Lan panel: " << (void*)Child << "\n";
			}
			else if (Slot == CV->InputSlot && !GOriginalInputMappingPanel)
			{
				GOriginalInputMappingPanel = Child;
				MarkAsGCRoot(GOriginalInputMappingPanel);
				std::cout << "[SDK] Captured original InputMapping panel: " << (void*)Child << "\n";
			}

			std::cout << "[SDK] " << SlotName << ": removing game panel " << (void*)Child << "\n";
			Child->RemoveFromParent();
		}
		else
			break;
	}

	
	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);



	std::cout << "[SDK] " << SlotName << ": creating UVerticalBox (WidgetTree=" << (void*)WidgetTree << ")\n";

	auto* VBox = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (!VBox)
	{
		std::cout << "[SDK] " << SlotName << ": failed to create UVerticalBox\n";
		return nullptr;
	}

	Slot->AddChild(VBox);
	std::cout << "[SDK] " << SlotName << ": UVerticalBox created and added to slot\n";
	return VBox;
}
void PopulateTab_Character(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VolumeSlot, "Tab0(VolumeSlot)");
	if (!Container)
	{
		std::cout << "[SDK] Tab0: no container available, skipping\n";
		return;
	}

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
			Pad.Left = 0.0f;
			Pad.Top = TopGap;
			Pad.Right = 0.0f;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};

	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};

	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};

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

	auto* RolePanel = CreateCollapsiblePanel(PC, L"角色选项");
	auto* RoleBox = RolePanel ? RolePanel->CT_Contents : nullptr;
	AddDropdown(RoleBox, L"额外心法栏", { L"0", L"1", L"2" });
	AddDropdown(RoleBox, L"门派",
		{ L"无门派", L"少林", L"武当", L"峨眉",
		  L"明教", L"丐帮", L"唐门", L"天山" });
	AddPanelWithFixedGap(RolePanel, 0.0f, 10.0f);

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
	AddSlider(RatioBox, L"金钱倍率");
	AddSlider(RatioBox, L"武学点倍率");
	AddSlider(RatioBox, L"真气消耗倍率");
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

	std::cout << "[SDK] Tab0 (Character): " << Count << " widgets added\n";
}


void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VideoSlot, "Tab1(VideoSlot)");
	if (!Container) return;

	Container->ClearChildren();
	ClearItemBrowserState();
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

	std::cout << "[SDK] Tab1 (Items): " << Count << " widgets, "
	          << GAllItems.size() << " items cached\n";
}
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

	std::cout << "[SDK] Tab2 (Battle): " << Count << " widgets added\n";
}
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

	std::cout << "[SDK] Tab3 (Life): " << Count << " widgets added\n";
}
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

	std::cout << "[SDK] Tab4 (Social): " << Count << " widgets added\n";
}
void PopulateTab_System(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->GamepadSlot, "Tab5(GamepadSlot)");
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
	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
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

	auto* MovePanel = CreateCollapsiblePanel(PC, L"移动与跳跃");
	auto* MoveBox = MovePanel ? MovePanel->CT_Contents : nullptr;
	AddToggle(MoveBox, L"空格跳跃");
	AddSlider(MoveBox, L"跳跃速度");
	AddToggle(MoveBox, L"无限跳跃");
	AddToggle(MoveBox, L"奔跑/骑马加速");
	AddSlider(MoveBox, L"加速倍率");
	AddSlider(MoveBox, L"世界移动速度");
	AddSlider(MoveBox, L"场景移动速度");
	AddPanelWithFixedGap(MovePanel, 0.0f, 10.0f);

	auto* MountPanel = CreateCollapsiblePanel(PC, L"坐骑设置");
	auto* MountBox = MountPanel ? MountPanel->CT_Contents : nullptr;
	AddToggle(MountBox, L"坐骑替换");
	AddDropdown(MountBox, L"指定坐骑", { L"黑马", L"白马", L"棕马", L"小毛驴" });
	AddPanelWithFixedGap(MountPanel, 0.0f, 10.0f);

	auto* StoryPanel = CreateCollapsiblePanel(PC, L"开档与解锁");
	auto* StoryBox = StoryPanel ? StoryPanel->CT_Contents : nullptr;
	AddToggle(StoryBox, L"一周目可选极难");
	AddToggle(StoryBox, L"一周目可选传承");
	AddToggle(StoryBox, L"承君传承包括所有");
	AddToggle(StoryBox, L"未交互驿站可用");
	AddToggle(StoryBox, L"激活GM命令行");
	AddToggle(StoryBox, L"解锁全图鉴");
	AddToggle(StoryBox, L"解锁全成就");
	AddPanelWithFixedGap(StoryPanel, 0.0f, 10.0f);

	auto* ScreenPanel = CreateCollapsiblePanel(PC, L"屏幕设置");
	auto* ScreenBox = ScreenPanel ? ScreenPanel->CT_Contents : nullptr;
	AddDropdown(ScreenBox, L"分辨率", { L"1920x1080", L"2560x1440", L"3840x2160" });
	AddDropdown(ScreenBox, L"窗口模式", { L"全屏", L"无边框", L"窗口" });
	AddToggle(ScreenBox, L"垂直同步");
	AddPanelWithFixedGap(ScreenPanel, 0.0f, 10.0f);

	auto* DiffPanel = CreateCollapsiblePanel(PC, L"开档难度系数");
	auto* DiffBox = DiffPanel ? DiffPanel->CT_Contents : nullptr;
	AddNumeric(DiffBox, L"简单系数", L"100");
	AddNumeric(DiffBox, L"普通系数", L"100");
	AddNumeric(DiffBox, L"困难系数", L"100");
	AddNumeric(DiffBox, L"极难系数", L"100");
	AddNumeric(DiffBox, L"敌人伤害系数", L"100");
	AddNumeric(DiffBox, L"敌人气血系数", L"100");
	AddNumeric(DiffBox, L"资源产出系数", L"100");
	AddNumeric(DiffBox, L"经验获取系数", L"100");
	AddPanelWithFixedGap(DiffPanel, 0.0f, 10.0f);

	auto* TitlePanel = CreateCollapsiblePanel(PC, L"称号战力门槛");
	auto* TitleBox = TitlePanel ? TitlePanel->CT_Contents : nullptr;
	AddNumeric(TitleBox, L"称号门槛1", L"100");
	AddNumeric(TitleBox, L"称号门槛2", L"200");
	AddNumeric(TitleBox, L"称号门槛3", L"300");
	AddNumeric(TitleBox, L"称号门槛4", L"400");
	AddNumeric(TitleBox, L"称号门槛5", L"500");
	AddNumeric(TitleBox, L"称号门槛6", L"600");
	AddNumeric(TitleBox, L"称号门槛7", L"700");
	AddNumeric(TitleBox, L"称号门槛8", L"800");
	AddNumeric(TitleBox, L"称号门槛9", L"900");
	AddNumeric(TitleBox, L"称号门槛10", L"1000");
	AddNumeric(TitleBox, L"称号门槛11", L"1100");
	AddPanelWithFixedGap(TitlePanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab5 (System): " << Count << " widgets added\n";
}


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
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢尧", L"唐婉婉", L"徐小小", L"向天笑" });
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
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢尧", L"唐婉婉", L"徐小小", L"向天笑" });
	if (GTeammateReplaceDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammateReplaceDD);
		else GDynTabContent6->AddChild(GTeammateReplaceDD);
		Count++;
	}
	AddPanelWithFixedGap(OperatePanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab6 (Teammates): " << Count << " widgets added\n";
}


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

	std::cout << "[SDK] Tab7 (Quests): " << Count << " widgets added\n";
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
		std::cout << "[SDK] Tab8 (Controls): showcase button added\n";
	}
	else
	{
		std::cout << "[SDK] Tab8 (Controls): failed to create showcase button\n";
	}
}







