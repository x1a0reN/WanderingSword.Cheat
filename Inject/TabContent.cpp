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

	// ── 基础数值 (滑块) ──
	auto AddSlider = [&](const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	// ── 选项 (下拉框) ──
	auto AddDropdown = [&](const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	AddSlider(L"\u91D1\u94B1");               // 金钱
	AddSlider(L"\u6B66\u5B66\u70B9");         // 武学点
	AddSlider(L"\u7ECF\u8109\u70B9");         // 经脉点
	AddSlider(L"\u95E8\u6D3E\u8D21\u732E");   // 门派贡献
	AddSlider(L"\u7EE7\u627F\u70B9");         // 继承点
	AddSlider(L"\u7B49\u7EA7");               // 等级
	AddSlider(L"\u9493\u9C7C\u7B49\u7EA7");   // 钓鱼等级

	AddDropdown(L"\u989D\u5916\u5FC3\u6CD5\u680F",  // 额外心法栏
		{ L"0", L"1", L"2" });

	AddDropdown(L"\u95E8\u6D3E",                     // 门派
		{ L"\u65E0\u95E8\u6D3E",                     // 无门派
		  L"\u5C11\u6797",                           // 少林
		  L"\u6B66\u5F53",                           // 武当
		  L"\u5CE8\u7709",                           // 峨眉
		  L"\u660E\u6559",                           // 明教
		  L"\u4E10\u5E2E",                           // 丐帮
		  L"\u5510\u95E8",                           // 唐门
		  L"\u5929\u5C71" });                        // 天山

	// ── 属性 (滑块) ──
	AddSlider(L"\u6C14\u8840");               // 气血
	AddSlider(L"\u6C14\u8840\u4E0A\u9650");   // 气血上限
	AddSlider(L"\u771F\u6C14");               // 真气
	AddSlider(L"\u771F\u6C14\u4E0A\u9650");   // 真气上限
	AddSlider(L"\u7CBE\u529B");               // 精力
	AddSlider(L"\u7CBE\u529B\u4E0A\u9650");   // 精力上限
	AddSlider(L"\u529B\u9053");               // 力道
	AddSlider(L"\u6839\u9AA8");               // 根骨
	AddSlider(L"\u8EAB\u6CD5");               // 身法
	AddSlider(L"\u5185\u529F");               // 内功
	AddSlider(L"\u653B\u51FB");               // 攻击
	AddSlider(L"\u9632\u5FA1");               // 防御
	AddSlider(L"\u66B4\u51FB");               // 暴击
	AddSlider(L"\u95EA\u907F");               // 闪避
	AddSlider(L"\u547D\u4E2D");               // 命中

	// ── 精通经验 (滑块) ──
	AddSlider(L"\u62F3\u638C\u7CBE\u901A");   // 拳掌精通
	AddSlider(L"\u5251\u6CD5\u7CBE\u901A");   // 剑法精通
	AddSlider(L"\u5200\u6CD5\u7CBE\u901A");   // 刀法精通
	AddSlider(L"\u67AA\u68CD\u7CBE\u901A");   // 枪棍精通
	AddSlider(L"\u6697\u5668\u7CBE\u901A");   // 暗器精通

	std::cout << "[SDK] Tab0 (Character): " << Count << " items added\n";
}

// ── Populate other tabs (stub functions for structure) ──
void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VideoSlot, "Tab1(VideoSlot)");
	if (!Container) return;

	// Temporary diagnostic route: render the Controls showcase in Tab1.
	// This helps verify whether crashes are tied to Tab8 slot/switching logic.
	if (kTempMoveControlsShowcaseToTab1)
	{
		ClearItemBrowserState();
		auto* Tab1VBox = Container->IsA(UVerticalBox::StaticClass())
			? static_cast<UVerticalBox*>(Container)
			: nullptr;
		if (!Tab1VBox)
		{
			std::cout << "[SDK] Tab1 fallback failed: container is not UVerticalBox\n";
			return;
		}

		std::cout << "[SDK] Tab1: temporary Controls showcase mode enabled\n";
		UVerticalBox* SavedTab8 = GDynTabContent8;
		GDynTabContent8 = Tab1VBox;
		PopulateTab_Controls(CV, PC);
		GDynTabContent8 = SavedTab8;
		std::cout << "[SDK] Tab1: Controls showcase populated for crash test\n";
		return;
	}

	Container->ClearChildren();
	ClearItemBrowserState();
	int Count = 0;

	auto AddToggle = [&](const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};
	auto AddSlider = [&](const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item) { Container->AddChild(Item); Count++; }
	};

	// ── Zone 1: Quick toggles ──
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

	BuildItemCache();

	GItemCategoryDD = CreateVideoItemWithOptions(PC,
		L"\u2550\u2550 \u7269\u54C1\u7BA1\u7406 \u2550\u2550",
		{ L"\u5168\u90E8", L"\u6B66\u5668", L"\u9632\u5177", L"\u6D88\u8017\u54C1", L"\u5176\u4ED6" });
	if (GItemCategoryDD) {
		Container->AddChild(GItemCategoryDD);
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

		Container->AddChild(GItemQuantityRow);
		Count++;
	}
	GItemPagerRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
	if (GItemPagerRow)
	{
		GItemPrevPageBtn = static_cast<UButton*>(CreateRawWidget(UButton::StaticClass(), Outer));
		if (GItemPrevPageBtn)
		{
			ClearButtonBindings(GItemPrevPageBtn);
			GItemPrevPageBtn->IsFocusable = false;
			auto* PrevTxt = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
			if (PrevTxt)
			{
				PrevTxt->SetText(MakeText(L"\u4E0A\u4E00\u9875"));
				GItemPrevPageBtn->SetContent(PrevTxt);
			}
			GItemPagerRow->AddChildToHorizontalBox(GItemPrevPageBtn);
		}

		GItemPageLabel = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
		if (GItemPageLabel)
		{
			GItemPageLabel->SetText(MakeText(L"1/1"));
			GItemPagerRow->AddChildToHorizontalBox(GItemPageLabel);
		}

		GItemNextPageBtn = static_cast<UButton*>(CreateRawWidget(UButton::StaticClass(), Outer));
		if (GItemNextPageBtn)
		{
			ClearButtonBindings(GItemNextPageBtn);
			GItemNextPageBtn->IsFocusable = false;
			auto* NextTxt = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
			if (NextTxt)
			{
				NextTxt->SetText(MakeText(L"\u4E0B\u4E00\u9875"));
				GItemNextPageBtn->SetContent(NextTxt);
			}
			GItemPagerRow->AddChildToHorizontalBox(GItemNextPageBtn);
		}

		Container->AddChild(GItemPagerRow);
		Count++;
	}

	GItemGridPanel = static_cast<UUniformGridPanel*>(CreateRawWidget(UUniformGridPanel::StaticClass(), Outer));
	if (GItemGridPanel)
	{
		GItemGridPanel->SetMinDesiredSlotWidth(64.0f);
		GItemGridPanel->SetMinDesiredSlotHeight(64.0f);
		Container->AddChild(GItemGridPanel);
		Count++;

		for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
		{
			auto* Btn = static_cast<UButton*>(CreateRawWidget(UButton::StaticClass(), Outer));
			if (!Btn)
				continue;
			ClearButtonBindings(Btn);
			Btn->IsFocusable = false;

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

	AddToggle(L"\u4F24\u5BB3\u52A0\u500D");           // 伤害加倍
	AddSlider(L"\u4F24\u5BB3\u500D\u7387");            // 伤害倍率
	AddToggle(L"\u62DB\u5F0F\u65E0\u89C6\u51B7\u5374"); // 招式无视冷却
	AddToggle(L"\u6218\u6597\u52A0\u901F");            // 战斗加速
	AddSlider(L"\u6218\u6597\u52A0\u901F\u500D\u6570"); // 战斗加速倍数
	AddToggle(L"\u4E0D\u9047\u654C");                  // 不遇敌
	AddToggle(L"\u5168\u961F\u53CB\u53C2\u6218");      // 全队友参战
	AddToggle(L"\u6218\u8D25\u89C6\u4E3A\u80DC\u5229"); // 战败视为胜利
	AddToggle(L"\u5FC3\u6CD5\u586B\u88C5\u6700\u540E\u4E00\u683C"); // 心法填装最后一格
	AddToggle(L"\u6218\u6597\u524D\u81EA\u52A8\u6062\u590D"); // 战斗前自动恢复
	AddToggle(L"\u79FB\u52A8\u901F\u5EA6\u52A0\u500D"); // 移动速度加倍
	AddSlider(L"\u79FB\u52A8\u500D\u7387");            // 移动倍率
	AddToggle(L"\u53EA\u5BF9\u672C\u65B9\u751F\u6548"); // 只对本方生效

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

	AddToggle(L"\u953B\u9020/\u5236\u8863/\u70BC\u4E39/\u70F9\u996A\u65E0\u89C6\u8981\u6C42"); // 锻造/制衣/炼丹/烹饪无视要求
	AddToggle(L"\u8BBE\u7F6E\u4EA7\u51FA\u6570\u91CF"); // 设置产出数量
	AddSlider(L"\u4EA7\u51FA\u6570\u91CF");            // 产出数量
	AddToggle(L"\u91C7\u96C6\u4E00\u79D2\u51B7\u5374"); // 采集一秒冷却
	AddToggle(L"\u9493\u9C7C\u53EA\u9493\u7A00\u6709\u7269"); // 钓鱼只钓稀有物
	AddToggle(L"\u9493\u9C7C\u6536\u7B3F\u5FC5\u6709\u6536\u83B7"); // 钓鱼收杆必有收获
	AddToggle(L"\u5BB6\u56ED\u968F\u65F6\u6536\u83B7"); // 家园随时收获

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

	AddToggle(L"\u9001\u793C\u5FC5\u5B9A\u559C\u6B22"); // 送礼必定喜欢
	AddToggle(L"\u9080\u8BF7\u65E0\u89C6\u6761\u4EF6"); // 邀请无视条件
	AddToggle(L"\u5207\u78CB\u65E0\u89C6\u597D\u611F"); // 切磋无视好感
	AddToggle(L"\u8BF7\u6559\u65E0\u89C6\u8981\u6C42"); // 请教无视要求
	AddToggle(L"\u5207\u78CB\u83B7\u5F97\u5BF9\u624B\u80CC\u5305"); // 切磋获得对手背包
	AddToggle(L"NPC\u88C5\u5907\u53EF\u8131");         // NPC装备可脱
	AddToggle(L"NPC\u65E0\u89C6\u6B66\u5668\u529F\u6CD5\u9650\u5236"); // NPC无视武器功法限制
	AddToggle(L"\u5F3A\u5236\u663E\u793ANPC\u4E92\u52A8"); // 强制显示NPC互动

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

	AddToggle(L"\u7A7A\u683C\u8DF3\u8DC3");           // 空格跳跃
	AddSlider(L"\u8DF3\u8DC3\u901F\u5EA6");            // 跳跃速度
	AddToggle(L"\u65E0\u9650\u8DF3\u8DC3");            // 无限跳跃
	AddToggle(L"\u5954\u8DD1/\u9A91\u9A6C\u52A0\u901F"); // 奔跑/骑马加速
	AddSlider(L"\u52A0\u901F\u500D\u7387");            // 加速倍率
	AddToggle(L"\u5750\u9A91\u66FF\u6362");            // 坐骑替换
	AddDropdown(L"\u6307\u5B9A\u5750\u9A91",           // 指定坐骑
		{ L"\u9ED1\u9A6C", L"\u767D\u9A6C",           // 黑马, 白马
		  L"\u68D5\u9A6C", L"\u5C0F\u6BDB\u9A74" });  // 棕马, 小毛驴
	AddToggle(L"\u4E00\u5468\u76EE\u53EF\u9009\u6781\u96BE"); // 一周目可选极难
	AddToggle(L"\u4E00\u5468\u76EE\u53EF\u9009\u4F20\u627F"); // 一周目可选传承
	AddToggle(L"\u627F\u541B\u4F20\u627F\u5305\u62EC\u6240\u6709"); // 承君传承包括所有
	AddToggle(L"\u672A\u4EA4\u4E92\u9A7F\u7AD9\u53EF\u7528"); // 未交互驿站可用
	AddToggle(L"\u6FC0\u6D3BGM\u547D\u4EE4\u884C");   // 激活GM命令行
	AddToggle(L"\u89E3\u9501\u5168\u56FE\u9274");      // 解锁全图鉴
	AddToggle(L"\u89E3\u9501\u5168\u6210\u5C31");      // 解锁全成就

	std::cout << "[SDK] Tab5 (System): " << Count << " items added\n";
}

// ═══ Dynamic Tab 6 - 队友 (Teammates) ═══
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

	GTeammateFollowToggle = CreateToggleItem(PC, L"\u8BBE\u7F6E\u961F\u53CB\u8DDF\u968F\u6570\u91CF"); // 设置队友跟随数量
	if (GTeammateFollowToggle) { GDynTabContent6->AddChild(GTeammateFollowToggle); Count++; }

	GTeammateFollowCount = CreateVolumeItem(PC, L"\u8DDF\u968F\u6570\u91CF"); // 跟随数量
	if (GTeammateFollowCount) { GDynTabContent6->AddChild(GTeammateFollowCount); Count++; }

	GTeammateAddDD = CreateVideoItemWithOptions(PC, L"\u6DFB\u52A0\u961F\u53CB", // 添加队友
		{ L"\u8BF7\u9009\u62E9",                     // 请选择
		  L"\u767E\u91CC\u4E1C\u98CE",               // 百里东风
		  L"\u5C1A\u4E91\u6EAA",                     // 尚云溪
		  L"\u53F6\u5343\u79CB",                     // 叶千秋
		  L"\u8C22\u5C27",                           // 谢尧
		  L"\u5510\u5A49\u5A49",                     // 唐婉婉
		  L"\u5F90\u5C0F\u5C0F",                     // 徐小小
		  L"\u5411\u5929\u7B11" });                  // 向天笑
	if (GTeammateAddDD) { GDynTabContent6->AddChild(GTeammateAddDD); Count++; }

	GTeammateReplaceToggle = CreateToggleItem(PC, L"\u66FF\u6362\u6307\u5B9A\u961F\u53CB"); // 替换指定队友
	if (GTeammateReplaceToggle) { GDynTabContent6->AddChild(GTeammateReplaceToggle); Count++; }

	GTeammateReplaceDD = CreateVideoItemWithOptions(PC, L"\u6307\u5B9A\u961F\u53CB", // 指定队友
		{ L"\u8BF7\u9009\u62E9",                     // 请选择
		  L"\u767E\u91CC\u4E1C\u98CE",               // 百里东风
		  L"\u5C1A\u4E91\u6EAA",                     // 尚云溪
		  L"\u53F6\u5343\u79CB",                     // 叶千秋
		  L"\u8C22\u5C27",                           // 谢尧
		  L"\u5510\u5A49\u5A49",                     // 唐婉婉
		  L"\u5F90\u5C0F\u5C0F",                     // 徐小小
		  L"\u5411\u5929\u7B11" });                  // 向天笑
	if (GTeammateReplaceDD) { GDynTabContent6->AddChild(GTeammateReplaceDD); Count++; }

	std::cout << "[SDK] Tab6 (Teammates): " << Count << " items added\n";
}

// ═══ Dynamic Tab 7 - 任务 (Quests) ═══
void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent7) return;
	GDynTabContent7->ClearChildren();
	int Count = 0;

	GQuestToggle = nullptr;
	GQuestTypeDD = nullptr;

	GQuestToggle = CreateToggleItem(PC, L"\u63A5\u5230/\u5B8C\u6210\u4EFB\u52A1"); // 接到/完成任务
	if (GQuestToggle) { GDynTabContent7->AddChild(GQuestToggle); Count++; }

	GQuestTypeDD = CreateVideoItemWithOptions(PC, L"\u6267\u884C\u7C7B\u578B", // 执行类型
		{ L"\u63A5\u5230", L"\u5B8C\u6210" }); // 接到, 完成
	if (GQuestTypeDD) { GDynTabContent7->AddChild(GQuestTypeDD); Count++; }

	std::cout << "[SDK] Tab7 (Quests): " << Count << " items added\n";
}

// ═══ Dynamic Tab 8 - 控件展示 (Controls Showcase) ═══
void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent8 || !CV || !PC) return;
	GDynTabContent8->ClearChildren();

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
		: static_cast<UObject*>(CV);
	UWidget* ResetBtn = CreateShowcaseResetButton(CV, Outer, PC);
	if (ResetBtn)
	{
		auto* SizeHost = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
		if (SizeHost)
		{
			SizeHost->SetWidthOverride(160.0f);
			SizeHost->SetHeightOverride(56.0f);
			SizeHost->SetMinDesiredWidth(160.0f);
			SizeHost->SetMinDesiredHeight(56.0f);
			UPanelSlot* InnerSlot = SizeHost->AddChild(ResetBtn);
			UPanelSlot* OuterSlot = GDynTabContent8->AddChild(SizeHost);
			ResetBtn->ForceLayoutPrepass();
			SizeHost->ForceLayoutPrepass();
			GDynTabContent8->ForceLayoutPrepass();
			std::cout << "[SDK] Tab8 attach(sizehost): innerSlot=" << (void*)InnerSlot
			          << " outerSlot=" << (void*)OuterSlot
			          << " btnParent=" << (void*)ResetBtn->GetParent()
			          << " sizeHostParent=" << (void*)SizeHost->GetParent()
			          << " btnVis=" << ToVisName(ResetBtn->GetVisibility())
			          << "\n";
		}
		else
		{
			UPanelSlot* Slot = GDynTabContent8->AddChild(ResetBtn);
			ResetBtn->ForceLayoutPrepass();
			GDynTabContent8->ForceLayoutPrepass();
			std::cout << "[SDK] Tab8 attach(direct): slot=" << (void*)Slot
			          << " btnParent=" << (void*)ResetBtn->GetParent()
			          << " btnVis=" << ToVisName(ResetBtn->GetVisibility())
			          << "\n";
		}
		std::cout << "[SDK] Tab8 (Controls): single reused reset button added\n";
	}
	else
	{
		std::cout << "[SDK] Tab8 (Controls): failed to reuse reset button\n";
	}
}

// ═══ Create Dynamic Tabs 6/7/8 ═══
// Tab6 uses switcher index 6.
// Tab7/Tab8 share switcher index 7 by hot-swapping the mounted panel,
// avoiding SetActiveWidgetIndex(8) crashes in the original module logic.

