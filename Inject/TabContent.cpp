#include <iostream>
#include "TabContent.hpp"
#include "GCManager.hpp"
#include "ItemBrowser.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"
UPanelWidget* GetOrCreateSlotContainer(UBPMV_ConfigView2_C* CV, UNeoUINamedSlot* Slot, const char* SlotName)
{
	if (!Slot)
	{
		std::cout << "[SDK] " << SlotName << ": slot pointer is null\n";
		return nullptr;
	}

	// Remove any existing children (game's sub-module panels) from the slot.
	// This detaches the whole sub-module panel so its blueprint tick logic
	// won't run and access stale child pointers.
	int childCount = Slot->GetChildrenCount();
	std::cout << "[SDK] " << SlotName << ": ptr=" << (void*)Slot << " children=" << childCount << "\n";
	while (Slot->GetChildrenCount() > 0)
	{
		UWidget* Child = Slot->GetChildAt(0);
		if (Child)
		{
			// Keep original game-built panels with valid VM context for stable Tab8 showcase.
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

	// Create a fresh UVerticalBox for our cheat UI content
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

	AddSlider(L"\u91D1\u94B1");
	AddEditBox(L"\u6B66\u5B66\u70B9", L"\u8F93\u5165\u6570\u503C", L"100");
	AddSlider(L"\u7ECF\u8109\u70B9");
	AddSlider(L"\u95E8\u6D3E\u8D21\u732E");
	AddSlider(L"\u7EE7\u627F\u70B9");
	AddSlider(L"\u7B49\u7EA7");
	AddSlider(L"\u9493\u9C7C\u7B49\u7EA7");

	AddDropdown(L"\u989D\u5916\u5FC3\u6CD5\u680F", { L"0", L"1", L"2" });
	AddDropdown(L"\u95E8\u6D3E",
		{ L"\u65E0\u95E8\u6D3E", L"\u5C11\u6797", L"\u6B66\u5F53", L"\u5CE8\u7709",
		  L"\u660E\u6559", L"\u4E10\u5E2E", L"\u5510\u95E8", L"\u5929\u5C71" });

	AddSlider(L"\u6C14\u8840");
	AddSlider(L"\u6C14\u8840\u4E0A\u9650");
	AddSlider(L"\u771F\u6C14");
	AddSlider(L"\u771F\u6C14\u4E0A\u9650");
	AddSlider(L"\u7CBE\u529B");
	AddSlider(L"\u7CBE\u529B\u4E0A\u9650");
	AddSlider(L"\u529B\u9053");
	AddSlider(L"\u6839\u9AA8");
	AddSlider(L"\u8EAB\u6CD5");
	AddSlider(L"\u5185\u529F");
	AddSlider(L"\u653B\u51FB");
	AddSlider(L"\u9632\u5FA1");
	AddSlider(L"\u66B4\u51FB");
	AddSlider(L"\u95EA\u907F");
	AddSlider(L"\u547D\u4E2D");

	AddSlider(L"\u62F3\u638C\u7CBE\u901A");
	AddSlider(L"\u5251\u6CD5\u7CBE\u901A");
	AddSlider(L"\u5200\u6CD5\u7CBE\u901A");
	AddSlider(L"\u67AA\u68CD\u7CBE\u901A");
	AddSlider(L"\u6697\u5668\u7CBE\u901A");

	std::cout << "[SDK] Tab0 (Character): " << Count << " items added\n";
}

// 鈹€鈹€ Populate other tabs (stub functions for structure) 鈹€鈹€
void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VideoSlot, "Tab1(VideoSlot)");
	if (!Container) return;

	Container->ClearChildren();
	ClearItemBrowserState();
	int Count = 0;

	// 鈺愨晲鈺?鎶樺彔闈㈡澘 1: 鐗╁搧閫夐」 鈺愨晲鈺?
	auto* OptionsPanel = CreateCollapsiblePanel(PC, L"\u7269\u54C1\u9009\u9879");
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

	AddToggle(L"\u7269\u54C1\u4E0D\u51CF");
	AddToggle(L"\u7269\u54C1\u83B7\u5F97\u52A0\u500D");
	AddSlider(L"\u52A0\u500D\u500D\u6570");
	AddToggle(L"\u6240\u6709\u7269\u54C1\u53EF\u51FA\u552E");
	AddToggle(L"\u5305\u62EC\u4EFB\u52A1\u7269\u54C1");
	AddToggle(L"\u6389\u843D\u7387100%");
	AddToggle(L"\u953B\u9020\u5236\u8863\u6548\u679C\u52A0\u500D");
	AddSlider(L"\u9053\u5177\u589E\u91CF\u6548\u679C\u500D\u7387");
	AddSlider(L"\u989D\u5916\u6548\u679C\u500D\u7387");
	AddToggle(L"\u6700\u5927\u989D\u5916\u8BCD\u6761\u6570");
	AddToggle(L"\u65E0\u89C6\u7269\u54C1\u4F7F\u7528\u6B21\u6570");
	AddToggle(L"\u65E0\u89C6\u7269\u54C1\u4F7F\u7528\u8981\u6C42");

	if (OptionsPanel) { Container->AddChild(OptionsPanel); Count++; }

	// 鈺愨晲鈺?鎶樺彔闈㈡澘 2: 鐗╁搧娴忚鍣?鈺愨晲鈺?
	auto* BrowserPanel = CreateCollapsiblePanel(PC, L"\u7269\u54C1\u6D4F\u89C8\u5668");
	UPanelWidget* BrowserBox = BrowserPanel ? BrowserPanel->CT_Contents : nullptr;

	BuildItemCache();

	GItemCategoryDD = CreateVideoItemWithOptions(PC,
		L"\u2550\u2550 \u7269\u54C1\u7BA1\u7406 \u2550\u2550",
		{ L"\u5168\u90E8", L"\u6B66\u5668", L"\u9632\u5177", L"\u6D88\u8017\u54C1", L"\u5176\u4ED6" });
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
			QtyLabel->SetText(MakeText(L"\u6DFB\u52A0\u6570\u91CF"));
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
		GItemPrevPageBtn = CreateGameStyleButton(PC, L"\u4E0A\u4E00\u9875", "ItemPrevPage",
			0.0f, 0.0f, &PrevLayout);
		if (PrevLayout)
			GItemPagerRow->AddChildToHorizontalBox(PrevLayout);

		GItemPageLabel = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
		if (GItemPageLabel)
		{
			GItemPageLabel->SetText(MakeText(L"1/1"));
			GItemPagerRow->AddChildToHorizontalBox(GItemPageLabel);
		}

		UWidget* NextLayout = nullptr;
		GItemNextPageBtn = CreateGameStyleButton(PC, L"\u4E0B\u4E00\u9875", "ItemNextPage",
			0.0f, 0.0f, &NextLayout);
		if (NextLayout)
			GItemPagerRow->AddChildToHorizontalBox(NextLayout);

		if (BrowserBox) BrowserBox->AddChild(GItemPagerRow);
		else Container->AddChild(GItemPagerRow);
		Count++;
	}

	GItemGridPanel = static_cast<UUniformGridPanel*>(CreateRawWidget(UUniformGridPanel::StaticClass(), Outer));
	if (GItemGridPanel)
	{
		GItemGridPanel->SetMinDesiredSlotWidth(64.0f);
		GItemGridPanel->SetMinDesiredSlotHeight(64.0f);
		if (BrowserBox) BrowserBox->AddChild(GItemGridPanel);
		else Container->AddChild(GItemGridPanel);
		Count++;

		for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
		{
			auto* Btn = static_cast<UButton*>(CreateRawWidget(UButton::StaticClass(), Outer));
			if (!Btn)
				continue;

			auto* Img = static_cast<UImage*>(CreateRawWidget(UImage::StaticClass(), Outer));
			if (Img)
			{
				Img->SetVisibility(ESlateVisibility::Collapsed);
				Btn->SetContent(Img);
			}

			int32 Row = i / ITEM_GRID_COLS;
			int32 Col = i % ITEM_GRID_COLS;
			GItemGridPanel->AddChildToUniformGrid(Btn, Row, Col);

			GItemSlotButtons[i] = Btn;
			GItemSlotImages[i] = Img;
			GItemSlotItemIndices[i] = -1;
			GItemSlotWasPressed[i] = false;
		}
	}

	if (BrowserPanel) { Container->AddChild(BrowserPanel); Count++; }

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

	AddToggle(L"\u4F24\u5BB3\u52A0\u500D");           // 浼ゅ鍔犲€?
	AddSlider(L"\u4F24\u5BB3\u500D\u7387");            // 浼ゅ鍊嶇巼
	AddToggle(L"\u62DB\u5F0F\u65E0\u89C6\u51B7\u5374"); // 鎷涘紡鏃犺鍐峰嵈
	AddToggle(L"\u6218\u6597\u52A0\u901F");            // 鎴樻枟鍔犻€?
	AddSlider(L"\u6218\u6597\u52A0\u901F\u500D\u6570"); // 鎴樻枟鍔犻€熷€嶆暟
	AddToggle(L"\u4E0D\u9047\u654C");                  // 涓嶉亣鏁?
	AddToggle(L"\u5168\u961F\u53CB\u53C2\u6218");      // 鍏ㄩ槦鍙嬪弬鎴?
	AddToggle(L"\u6218\u8D25\u89C6\u4E3A\u80DC\u5229"); // 鎴樿触瑙嗕负鑳滃埄
	AddToggle(L"\u5FC3\u6CD5\u586B\u88C5\u6700\u540E\u4E00\u683C"); // 蹇冩硶濉鏈€鍚庝竴鏍?
	AddToggle(L"\u6218\u6597\u524D\u81EA\u52A8\u6062\u590D"); // 鎴樻枟鍓嶈嚜鍔ㄦ仮澶?
	AddToggle(L"\u79FB\u52A8\u901F\u5EA6\u52A0\u500D"); // 绉诲姩閫熷害鍔犲€?
	AddSlider(L"\u79FB\u52A8\u500D\u7387");            // 绉诲姩鍊嶇巼
	AddToggle(L"\u53EA\u5BF9\u672C\u65B9\u751F\u6548"); // 鍙鏈柟鐢熸晥

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

	AddToggle(L"\u953B\u9020/\u5236\u8863/\u70BC\u4E39/\u70F9\u996A\u65E0\u89C6\u8981\u6C42"); // 閿婚€?鍒惰。/鐐间腹/鐑归オ鏃犺瑕佹眰
	AddToggle(L"\u8BBE\u7F6E\u4EA7\u51FA\u6570\u91CF"); // 璁剧疆浜у嚭鏁伴噺
	AddSlider(L"\u4EA7\u51FA\u6570\u91CF");            // 浜у嚭鏁伴噺
	AddToggle(L"\u91C7\u96C6\u4E00\u79D2\u51B7\u5374"); // 閲囬泦涓€绉掑喎鍗?
	AddToggle(L"\u9493\u9C7C\u53EA\u9493\u7A00\u6709\u7269"); // 閽撻奔鍙挀绋€鏈夌墿
	AddToggle(L"\u9493\u9C7C\u6536\u7B3F\u5FC5\u6709\u6536\u83B7"); // 閽撻奔鏀舵潌蹇呮湁鏀惰幏
	AddToggle(L"\u5BB6\u56ED\u968F\u65F6\u6536\u83B7"); // 瀹跺洯闅忔椂鏀惰幏

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

	AddToggle(L"\u9001\u793C\u5FC5\u5B9A\u559C\u6B22"); // 閫佺ぜ蹇呭畾鍠滄
	AddToggle(L"\u9080\u8BF7\u65E0\u89C6\u6761\u4EF6"); // 閭€璇锋棤瑙嗘潯浠?
	AddToggle(L"\u5207\u78CB\u65E0\u89C6\u597D\u611F"); // 鍒囩鏃犺濂芥劅
	AddToggle(L"\u8BF7\u6559\u65E0\u89C6\u8981\u6C42"); // 璇锋暀鏃犺瑕佹眰
	AddToggle(L"\u5207\u78CB\u83B7\u5F97\u5BF9\u624B\u80CC\u5305"); // 鍒囩鑾峰緱瀵规墜鑳屽寘
	AddToggle(L"NPC\u88C5\u5907\u53EF\u8131");         // NPC瑁呭鍙劚
	AddToggle(L"NPC\u65E0\u89C6\u6B66\u5668\u529F\u6CD5\u9650\u5236"); // NPC鏃犺姝﹀櫒鍔熸硶闄愬埗
	AddToggle(L"\u5F3A\u5236\u663E\u793ANPC\u4E92\u52A8"); // 寮哄埗鏄剧ずNPC浜掑姩

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

	AddToggle(L"\u7A7A\u683C\u8DF3\u8DC3");           // 绌烘牸璺宠穬
	AddSlider(L"\u8DF3\u8DC3\u901F\u5EA6");            // 璺宠穬閫熷害
	AddToggle(L"\u65E0\u9650\u8DF3\u8DC3");            // 鏃犻檺璺宠穬
	AddToggle(L"\u5954\u8DD1/\u9A91\u9A6C\u52A0\u901F"); // 濂旇窇/楠戦┈鍔犻€?
	AddSlider(L"\u52A0\u901F\u500D\u7387");            // 鍔犻€熷€嶇巼
	AddToggle(L"\u5750\u9A91\u66FF\u6362");            // 鍧愰獞鏇挎崲
	AddDropdown(L"\u6307\u5B9A\u5750\u9A91",           // 鎸囧畾鍧愰獞
		{ L"\u9ED1\u9A6C", L"\u767D\u9A6C",           // 榛戦┈, 鐧介┈
		  L"\u68D5\u9A6C", L"\u5C0F\u6BDB\u9A74" });  // 妫曢┈, 灏忔瘺椹?
	AddToggle(L"\u4E00\u5468\u76EE\u53EF\u9009\u6781\u96BE"); // 涓€鍛ㄧ洰鍙€夋瀬闅?
	AddToggle(L"\u4E00\u5468\u76EE\u53EF\u9009\u4F20\u627F"); // 涓€鍛ㄧ洰鍙€変紶鎵?
	AddToggle(L"\u627F\u541B\u4F20\u627F\u5305\u62EC\u6240\u6709"); // 鎵垮悰浼犳壙鍖呮嫭鎵€鏈?
	AddToggle(L"\u672A\u4EA4\u4E92\u9A7F\u7AD9\u53EF\u7528"); // 鏈氦浜掗┛绔欏彲鐢?
	AddToggle(L"\u6FC0\u6D3BGM\u547D\u4EE4\u884C");   // 婵€娲籊M鍛戒护琛?
	AddToggle(L"\u89E3\u9501\u5168\u56FE\u9274");      // 瑙ｉ攣鍏ㄥ浘閴?
	AddToggle(L"\u89E3\u9501\u5168\u6210\u5C31");      // 瑙ｉ攣鍏ㄦ垚灏?

	std::cout << "[SDK] Tab5 (System): " << Count << " items added\n";
}

// 鈺愨晲鈺?Dynamic Tab 6 - 闃熷弸 (Teammates) 鈺愨晲鈺?
void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent6) return;
	GDynTabContent6->ClearChildren();
	int Count = 0;

	// Reset cached widget pointers
	GTeammateFollowToggle = nullptr;
	GTeammateFollowCount = nullptr;
	GTeammateAddDD = nullptr;
	GTeammateReplaceToggle = nullptr;
	GTeammateReplaceDD = nullptr;

	GTeammateFollowToggle = CreateToggleItem(PC, L"\u8BBE\u7F6E\u961F\u53CB\u8DDF\u968F\u6570\u91CF"); // 璁剧疆闃熷弸璺熼殢鏁伴噺
	if (GTeammateFollowToggle) { GDynTabContent6->AddChild(GTeammateFollowToggle); Count++; }

	GTeammateFollowCount = CreateVolumeItem(PC, L"\u8DDF\u968F\u6570\u91CF"); // 璺熼殢鏁伴噺
	if (GTeammateFollowCount) { GDynTabContent6->AddChild(GTeammateFollowCount); Count++; }

	GTeammateAddDD = CreateVideoItemWithOptions(PC, L"\u6DFB\u52A0\u961F\u53CB", // 娣诲姞闃熷弸
		{ L"\u8BF7\u9009\u62E9",                     // 璇烽€夋嫨
		  L"\u767E\u91CC\u4E1C\u98CE",               // 鐧鹃噷涓滈
		  L"\u5C1A\u4E91\u6EAA",                     // 灏氫簯婧?
		  L"\u53F6\u5343\u79CB",                     // 鍙跺崈绉?
		  L"\u8C22\u5C27",                           // 璋㈠哀
		  L"\u5510\u5A49\u5A49",                     // 鍞愬濠?
		  L"\u5F90\u5C0F\u5C0F",                     // 寰愬皬灏?
		  L"\u5411\u5929\u7B11" });                  // 鍚戝ぉ绗?
	if (GTeammateAddDD) { GDynTabContent6->AddChild(GTeammateAddDD); Count++; }

	GTeammateReplaceToggle = CreateToggleItem(PC, L"\u66FF\u6362\u6307\u5B9A\u961F\u53CB"); // 鏇挎崲鎸囧畾闃熷弸
	if (GTeammateReplaceToggle) { GDynTabContent6->AddChild(GTeammateReplaceToggle); Count++; }

	GTeammateReplaceDD = CreateVideoItemWithOptions(PC, L"\u6307\u5B9A\u961F\u53CB", // 鎸囧畾闃熷弸
		{ L"\u8BF7\u9009\u62E9",                     // 璇烽€夋嫨
		  L"\u767E\u91CC\u4E1C\u98CE",               // 鐧鹃噷涓滈
		  L"\u5C1A\u4E91\u6EAA",                     // 灏氫簯婧?
		  L"\u53F6\u5343\u79CB",                     // 鍙跺崈绉?
		  L"\u8C22\u5C27",                           // 璋㈠哀
		  L"\u5510\u5A49\u5A49",                     // 鍞愬濠?
		  L"\u5F90\u5C0F\u5C0F",                     // 寰愬皬灏?
		  L"\u5411\u5929\u7B11" });                  // 鍚戝ぉ绗?
	if (GTeammateReplaceDD) { GDynTabContent6->AddChild(GTeammateReplaceDD); Count++; }

	std::cout << "[SDK] Tab6 (Teammates): " << Count << " items added\n";
}

// 鈺愨晲鈺?Dynamic Tab 7 - 浠诲姟 (Quests) 鈺愨晲鈺?
void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent7) return;
	GDynTabContent7->ClearChildren();
	int Count = 0;

	GQuestToggle = nullptr;
	GQuestTypeDD = nullptr;

	GQuestToggle = CreateToggleItem(PC, L"\u63A5\u5230/\u5B8C\u6210\u4EFB\u52A1"); // 鎺ュ埌/瀹屾垚浠诲姟
	if (GQuestToggle) { GDynTabContent7->AddChild(GQuestToggle); Count++; }

	GQuestTypeDD = CreateVideoItemWithOptions(PC, L"\u6267\u884C\u7C7B\u578B", // 鎵ц绫诲瀷
		{ L"\u63A5\u5230", L"\u5B8C\u6210" }); // 鎺ュ埌, 瀹屾垚
	if (GQuestTypeDD) { GDynTabContent7->AddChild(GQuestTypeDD); Count++; }

	std::cout << "[SDK] Tab7 (Quests): " << Count << " items added\n";
}

// 鈺愨晲鈺?Dynamic Tab 8 - 鎺т欢灞曠ず (Controls Showcase) 鈺愨晲鈺?
void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent8 || !CV || !PC) return;
	GDynTabContent8->ClearChildren();

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
		: static_cast<UObject*>(CV);
	UWidget* BtnLayout = nullptr;
	auto* ResetBtn = CreateGameStyleButton(PC, L"\u4E0B\u4E00\u9875", "Tab8Showcase",
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

// 鈺愨晲鈺?Create Dynamic Tabs 6/7/8 鈺愨晲鈺?
// Tab6 uses switcher index 6.
// Tab7/Tab8 share switcher index 7 by hot-swapping the mounted panel,
// avoiding SetActiveWidgetIndex(8) crashes in the original module logic.


