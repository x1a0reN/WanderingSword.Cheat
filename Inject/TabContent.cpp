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

	auto AddSlider = [&](const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	auto AddEditBox = [&](const wchar_t* Title, const wchar_t* Hint, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeEditBoxItem(PC, Outer, Container, Title, Hint, DefaultValue);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	auto AddDropdown = [&](const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	AddSlider(L"金钱");
	AddEditBox(L"武学点", L"输入数值", L"100");
	AddSlider(L"经脉点");
	AddSlider(L"门派贡献");
	AddSlider(L"继承点");
	AddSlider(L"等级");
	AddSlider(L"钓鱼等级");

	AddDropdown(L"额外心法栏", { L"0", L"1", L"2" });
	AddDropdown(L"门派",
		{ L"无门派", L"少林", L"武当", L"峨眉",
		  L"明教", L"丐帮", L"唐门", L"天山" });

	AddSlider(L"气血");
	AddSlider(L"气血上限");
	AddSlider(L"真气");
	AddSlider(L"真气上限");
	AddSlider(L"精力");
	AddSlider(L"精力上限");
	AddSlider(L"力道");
	AddSlider(L"根骨");
	AddSlider(L"身法");
	AddSlider(L"内功");
	AddSlider(L"攻击");
	AddSlider(L"防御");
	AddSlider(L"暴击");
	AddSlider(L"闪避");
	AddSlider(L"命中");

	AddSlider(L"拳掌精通");
	AddSlider(L"剑法精通");
	AddSlider(L"刀法精通");
	AddSlider(L"枪棍精通");
	AddSlider(L"暗器精通");

	std::cout << "[SDK] Tab0 (Character): " << Count << " items added\n";
}


void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VideoSlot, "Tab1(VideoSlot)");
	if (!Container) return;

	Container->ClearChildren();
	ClearItemBrowserState();
	int Count = 0;

	
	auto* OptionsPanel = CreateCollapsiblePanel(PC, L"物品选项");
	UPanelWidget* OptionsBox = OptionsPanel ? OptionsPanel->CT_Contents : nullptr;

	auto AddToggle = [&](const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item && OptionsBox) { OptionsBox->AddChild(Item); Count++; }
		else if (Item) { Container->AddChild(Item); Count++; }
	};
	auto AddSlider = [&](const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item && OptionsBox) { OptionsBox->AddChild(Item); Count++; }
		else if (Item) { Container->AddChild(Item); Count++; }
	};

	AddToggle(L"物品不减");
	AddToggle(L"物品获得加倍");
	AddSlider(L"加倍倍数");
	AddToggle(L"所有物品可出售");
	AddToggle(L"包括任务物品");
	AddToggle(L"掉落率100%");
	AddToggle(L"锻造制衣效果加倍");
	AddSlider(L"道具增量效果倍率");
	AddSlider(L"额外效果倍率");
	AddToggle(L"最大额外词条数");
	AddToggle(L"无视物品使用次数");
	AddToggle(L"无视物品使用要求");

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

	AddPanelWithFixedGap(OptionsPanel, 0.0f, 14.0f);

	
	auto* BrowserPanel = CreateCollapsiblePanel(PC, L"物品浏览器");
	UPanelWidget* BrowserBox = BrowserPanel ? BrowserPanel->CT_Contents : nullptr;

	BuildItemCache();

	GItemCategoryDD = CreateVideoItemWithOptions(PC,
		L"══ 物品管理 ══",
		{ L"全部", L"武器", L"防具", L"消耗品", L"其他" });
	if (GItemCategoryDD) {
		if (BrowserBox) BrowserBox->AddChild(GItemCategoryDD);
		else Container->AddChild(GItemCategoryDD);
		Count++;
	}
	GItemLastCatIdx = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);
	GItemQuantityRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
	if (GItemQuantityRow)
	{
		auto* QtyLabel = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
		if (QtyLabel)
		{
			QtyLabel->SetText(MakeText(L"添加数量"));
			GItemQuantityRow->AddChildToHorizontalBox(QtyLabel);
		}

		GItemQuantityEdit = static_cast<UEditableTextBox*>(CreateRawWidget(UEditableTextBox::StaticClass(), Outer));
		if (GItemQuantityEdit)
		{
			GItemQuantityEdit->SetHintText(MakeText(L"1"));
			wchar_t QtyBuf[16] = {};
			swprintf_s(QtyBuf, 16, L"%d", GItemAddQuantity);
			GItemQuantityEdit->SetText(MakeText(QtyBuf));
			GItemQuantityRow->AddChildToHorizontalBox(GItemQuantityEdit);
		}

		if (BrowserBox) BrowserBox->AddChild(GItemQuantityRow);
		else Container->AddChild(GItemQuantityRow);
		Count++;
	}
	GItemPagerRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
	if (GItemPagerRow)
	{
		UWidget* PrevLayout = nullptr;
		GItemPrevPageBtn = CreateGameStyleButton(PC, L"上一页", "ItemPrevPage",
			0.0f, 0.0f, &PrevLayout);
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
			0.0f, 0.0f, &NextLayout);
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
		GItemGridPanel->SetMinDesiredSlotHeight(68.0f);
		GItemGridPanel->SetSlotPadding(FMargin{ 3.0f, 3.0f, 3.0f, 3.0f });
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
			Pad.Top = 4.0f;
			Pad.Right = 0.0f;
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

	auto AddToggle = [&](const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};
	auto AddSlider = [&](const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	AddToggle(L"伤害加倍");           
	AddSlider(L"伤害倍率");            
	AddToggle(L"招式无视冷却"); 
	AddToggle(L"战斗加速");            
	AddSlider(L"战斗加速倍数"); 
	AddToggle(L"不遇敌");                  
	AddToggle(L"全队友参战");      
	AddToggle(L"战败视为胜利"); 
	AddToggle(L"心法填装最后一格"); 
	AddToggle(L"战斗前自动恢复"); 
	AddToggle(L"移动速度加倍"); 
	AddSlider(L"移动倍率");            
	AddToggle(L"只对本方生效"); 

	std::cout << "[SDK] Tab2 (Battle): " << Count << " items added\n";
}
void PopulateTab_Life(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->LanSlot, "Tab3(LanSlot)");
	if (!Container) return;
	Container->ClearChildren();
	int Count = 0;

	auto AddToggle = [&](const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};
	auto AddSlider = [&](const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	AddToggle(L"锻造/制衣/炼丹/烹饪无视要求"); 
	AddToggle(L"设置产出数量"); 
	AddSlider(L"产出数量");            
	AddToggle(L"采集一秒冷却"); 
	AddToggle(L"钓鱼只钓稀有物"); 
	AddToggle(L"钓鱼收笿必有收获"); 
	AddToggle(L"家园随时收获"); 

	std::cout << "[SDK] Tab3 (Life): " << Count << " items added\n";
}
void PopulateTab_Social(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->OthersSlot, "Tab4(OthersSlot)");
	if (!Container) return;
	Container->ClearChildren();
	int Count = 0;

	auto AddToggle = [&](const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	AddToggle(L"送礼必定喜欢"); 
	AddToggle(L"邀请无视条件"); 
	AddToggle(L"切磋无视好感"); 
	AddToggle(L"请教无视要求"); 
	AddToggle(L"切磋获得对手背包"); 
	AddToggle(L"NPC装备可脱");         
	AddToggle(L"NPC无视武器功法限制"); 
	AddToggle(L"强制显示NPC互动"); 

	std::cout << "[SDK] Tab4 (Social): " << Count << " items added\n";
}
void PopulateTab_System(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->GamepadSlot, "Tab5(GamepadSlot)");
	if (!Container) return;
	Container->ClearChildren();
	int Count = 0;

	auto AddToggle = [&](const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};
	auto AddSlider = [&](const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};
	auto AddDropdown = [&](const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	AddToggle(L"空格跳跃");           
	AddSlider(L"跳跃速度");            
	AddToggle(L"无限跳跃");            
	AddToggle(L"奔跑/骑马加速"); 
	AddSlider(L"加速倍率");            
	AddToggle(L"坐骑替换");            
	AddDropdown(L"指定坐骑",           
		{ L"黑马", L"白马",           
		  L"棕马", L"小毛驴" });  
	AddToggle(L"一周目可选极难"); 
	AddToggle(L"一周目可选传承"); 
	AddToggle(L"承君传承包括所有"); 
	AddToggle(L"未交互驿站可用"); 
	AddToggle(L"激活GM命令行");   
	AddToggle(L"解锁全图鉴");      
	AddToggle(L"解锁全成就");      

	std::cout << "[SDK] Tab5 (System): " << Count << " items added\n";
}


void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent6) return;
	GDynTabContent6->ClearChildren();
	int Count = 0;

	
	GTeammateFollowToggle = nullptr;
	GTeammateFollowCount = nullptr;
	GTeammateAddDD = nullptr;
	GTeammateReplaceToggle = nullptr;
	GTeammateReplaceDD = nullptr;

	GTeammateFollowToggle = CreateToggleItem(PC, L"设置队友跟随数量"); 
	if (GTeammateFollowToggle) { GDynTabContent6->AddChild(GTeammateFollowToggle); Count++; }

	GTeammateFollowCount = CreateVolumeItem(PC, L"跟随数量"); 
	if (GTeammateFollowCount) { GDynTabContent6->AddChild(GTeammateFollowCount); Count++; }

	GTeammateAddDD = CreateVideoItemWithOptions(PC, L"添加队友", 
		{ L"请选择",                     
		  L"百里东风",               
		  L"尚云溪",                     
		  L"叶千秋",                     
		  L"谢尧",                           
		  L"唐婉婉",                     
		  L"徐小小",                     
		  L"向天笑" });                  
	if (GTeammateAddDD) { GDynTabContent6->AddChild(GTeammateAddDD); Count++; }

	GTeammateReplaceToggle = CreateToggleItem(PC, L"替换指定队友"); 
	if (GTeammateReplaceToggle) { GDynTabContent6->AddChild(GTeammateReplaceToggle); Count++; }

	GTeammateReplaceDD = CreateVideoItemWithOptions(PC, L"指定队友", 
		{ L"请选择",                     
		  L"百里东风",               
		  L"尚云溪",                     
		  L"叶千秋",                     
		  L"谢尧",                           
		  L"唐婉婉",                     
		  L"徐小小",                     
		  L"向天笑" });                  
	if (GTeammateReplaceDD) { GDynTabContent6->AddChild(GTeammateReplaceDD); Count++; }

	std::cout << "[SDK] Tab6 (Teammates): " << Count << " items added\n";
}


void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent7) return;
	GDynTabContent7->ClearChildren();
	int Count = 0;

	GQuestToggle = nullptr;
	GQuestTypeDD = nullptr;

	GQuestToggle = CreateToggleItem(PC, L"接到/完成任务"); 
	if (GQuestToggle) { GDynTabContent7->AddChild(GQuestToggle); Count++; }

	GQuestTypeDD = CreateVideoItemWithOptions(PC, L"执行类型", 
		{ L"接到", L"完成" }); 
	if (GQuestTypeDD) { GDynTabContent7->AddChild(GQuestTypeDD); Count++; }

	std::cout << "[SDK] Tab7 (Quests): " << Count << " items added\n";
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







