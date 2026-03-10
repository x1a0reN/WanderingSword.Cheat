// ── 任务下拉列表数据 ──
std::vector<int32> GQuestOptionIds;
std::vector<std::wstring> GQuestOptionLabels;

void BuildQuestDropdownOptions()
{
	GQuestOptionIds.clear();
	GQuestOptionLabels.clear();

	// Get QuestResManager → QuestSettingTable (UDataTable* at +0x120)
	UObject* ResMgr = static_cast<UObject*>(UManagerFuncLib::GetQuestResManager());
	if (!ResMgr || !IsSafeLiveObject(ResMgr))
	{
		LOGE_STREAM("Tab7Quests") << "[SDK] GetQuestResManager failed\n";
		return;
	}
	LOGI_STREAM("Tab7Quests") << "[SDK] ResMgr=" << (void*)ResMgr << "\n";

	UDataTable* QuestTable = *reinterpret_cast<UDataTable**>(
		reinterpret_cast<uint8*>(ResMgr) + 0x120);
	if (!QuestTable || !IsSafeLiveObject(static_cast<UObject*>(QuestTable)))
	{
		LOGE_STREAM("Tab7Quests") << "[SDK] QuestSettingTable is null (ptr="
			<< (void*)QuestTable << ")\n";
		// Dump nearby pointers to find the correct offset
		for (int off = 0x100; off <= 0x180; off += 8)
		{
			void* ptr = *reinterpret_cast<void**>(
				reinterpret_cast<uint8*>(ResMgr) + off);
			LOGI_STREAM("Tab7Quests") << "[SDK]   ResMgr+0x"
				<< std::hex << off << std::dec << " = " << ptr << "\n";
		}
		return;
	}
	LOGI_STREAM("Tab7Quests") << "[SDK] QuestTable=" << (void*)QuestTable << "\n";

	// Iterate RowMap (TMap<FName, uint8*> at +0x30 of UDataTable)
	struct FRowMapPair { FName Key; uint8* Value; };
	auto& RowMap = *reinterpret_cast<TMap<FName, uint8*>*>(
		reinterpret_cast<uint8*>(QuestTable) + 0x30);

	if (!RowMap.IsValid())
	{
		LOGE_STREAM("Tab7Quests") << "[SDK] RowMap.IsValid() = false\n";
		return;
	}
	const int32 NumRows = RowMap.Num();
	LOGI_STREAM("Tab7Quests") << "[SDK] QuestTable rows: " << NumRows << "\n";

	GQuestOptionLabels.push_back(L"请选择任务");
	GQuestOptionIds.push_back(0);

	auto* ResManager = static_cast<UQuestResManager*>(ResMgr);

	for (int32 i = 0; i < NumRows && GQuestOptionIds.size() < 500; ++i)
	{
		uint8* RowData = RowMap[i].Value();
		if (!RowData) continue;

		// FQuestSetting row: first 8 bytes = UScriptStruct*, QuestId (int32) at +0x08
		int32 QuestId = *reinterpret_cast<int32*>(RowData + 0x08);
		if (QuestId <= 0) continue;

		// Get quest name via QuestResManager::GetQuestInfo
		std::wstring Label;
		UQuestInfo* Info = ResManager->GetQuestInfo(QuestId);
		if (Info && IsSafeLiveObject(static_cast<UObject*>(Info)))
		{
			// QuestName is FText at +0x30
			std::string NameUtf8 = Info->QuestName.ToString();
			if (!NameUtf8.empty())
			{
				// Convert UTF-8 to wide string
				int Len = MultiByteToWideChar(CP_UTF8, 0, NameUtf8.c_str(), -1, nullptr, 0);
				if (Len > 0)
				{
					Label.resize(Len - 1);
					MultiByteToWideChar(CP_UTF8, 0, NameUtf8.c_str(), -1, &Label[0], Len);
				}
			}
		}

		// Fallback: show ID if name is empty
		if (Label.empty())
		{
			wchar_t Buf[64];
			swprintf_s(Buf, 64, L"任务#%d", QuestId);
			Label = Buf;
		}

		GQuestOptionIds.push_back(QuestId);
		GQuestOptionLabels.push_back(Label);
	}

	LOGI_STREAM("Tab7Quests") << "[SDK] Built " << GQuestOptionIds.size() << " quest options\n";
}

void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTab.Content7) return;
	GDynTab.Content7->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GQuest.ExecuteBtn = nullptr;
	GQuest.BtnWasPressed = false;
	GQuest.QuestDD = nullptr;
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

	// Build quest list from DataTable
	BuildQuestDropdownOptions();

	auto* MainPanel = CreateCollapsiblePanel(PC, L"任务执行");
	auto* MainBox = MainPanel ? MainPanel->CT_Contents : nullptr;

	// 任务下拉列表
	GQuest.QuestDD = CreateVideoItemWithOptions(PC, L"选择任务", {});
	if (GQuest.QuestDD && GQuest.QuestDD->CB_Main)
	{
		GQuest.QuestDD->CB_Main->ClearOptions();
		for (const auto& Label : GQuestOptionLabels)
			GQuest.QuestDD->CB_Main->AddOption(FString(Label.c_str()));
		GQuest.QuestDD->CB_Main->SetSelectedIndex(0);

		if (MainBox) MainBox->AddChild(GQuest.QuestDD);
		else GDynTab.Content7->AddChild(GQuest.QuestDD);
		Count++;
	}

	// 执行类型下拉列表
	GQuest.TypeDD = CreateVideoItemWithOptions(PC, L"执行类型", { L"接取", L"完成" });
	if (GQuest.TypeDD)
	{
		if (MainBox) MainBox->AddChild(GQuest.TypeDD);
		else GDynTab.Content7->AddChild(GQuest.TypeDD);
		Count++;
	}

	// 执行按钮
	UWidget* BtnLayout = nullptr;
	GQuest.ExecuteBtn = CreateGameStyleButton(PC, L"立刻执行任务", "QuestExecute",
		0.0f, 0.0f, &BtnLayout);
	if (BtnLayout)
	{
		if (MainBox) MainBox->AddChild(BtnLayout);
		else GDynTab.Content7->AddChild(BtnLayout);
		Count++;
	}

	AddPanelWithFixedGap(MainPanel, 0.0f, 10.0f);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  任务 SDK 执行
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void ExecuteQuestSDK(int32 QuestId, bool bAccept)
{
	UObject* QMObj = static_cast<UObject*>(UManagerFuncLib::GetQuestManager());
	if (!QMObj || !IsSafeLiveObject(QMObj))
	{
		LOGE_STREAM("Tab7Quests") << "[SDK] GetQuestManager failed\n";
		return;
	}

	auto* QM = static_cast<UQuestManager*>(QMObj);

	if (bAccept)
	{
		QM->RequestQuest(QuestId);
		LOGI_STREAM("Tab7Quests") << "[SDK] RequestQuest(" << QuestId << ")\n";
	}
	else
	{
		QM->FinishQuest(QuestId);
		LOGI_STREAM("Tab7Quests") << "[SDK] FinishQuest(" << QuestId << ")\n";
	}
}

void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTab.Content8 || !CV || !PC) return;
	GDynTab.Content8->ClearChildren();

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
		: static_cast<UObject*>(CV);

	auto AddAboutText = [&](UVerticalBox* Box, const wchar_t* Text, float BottomPadding)
	{
		if (!Box || !Text || !*Text)
			return;
		auto* Label = CreateRawTextLabel(Outer, Text);
		if (!Label)
			return;
		Label->Font.Size = 18;
		Label->SetJustification(ETextJustify::Left);
		Label->SetAutoWrapText(true);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

		if (UPanelSlot* Slot = Box->AddChild(Label))
		{
			if (Slot->IsA(UVerticalBoxSlot::StaticClass()))
			{
				auto* VBoxSlot = static_cast<UVerticalBoxSlot*>(Slot);
				VBoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
				VBoxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
				FMargin Pad{};
				Pad.Bottom = BottomPadding;
				VBoxSlot->SetPadding(Pad);
			}
		}
	};

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		if (UPanelSlot* Slot = GDynTab.Content8->AddChild(Panel))
		{
			if (Slot->IsA(UVerticalBoxSlot::StaticClass()))
			{
				auto* VSlot = static_cast<UVerticalBoxSlot*>(Slot);
				FMargin Pad{};
				Pad.Top = TopGap;
				Pad.Bottom = BottomGap;
				VSlot->SetPadding(Pad);
			}
		}
	};

	auto* AboutPanel = CreateCollapsiblePanel(PC, L"关于本修改器");
	auto* AboutBox = AboutPanel ? AboutPanel->CT_Contents : nullptr;
	if (AboutBox)
	{
		AddAboutText(AboutBox,
			L"WanderingSword.Cheat 是一个面向《逸剑风云决》的游戏内修改器，目标是把常用能力直接塞进游戏原生 UI 流程里，少折腾、少跳窗、少来回切工具。",
			12.0f);
		AddAboutText(AboutBox,
			L"当前界面主要覆盖角色、物品、队友、任务等常见功能；像 NPC 浏览器这类页面也会记住上次搜索词和页码，避免每次重建后又从第一页重新翻，纯属对重复劳动下死手。",
			12.0f);
		AddAboutText(AboutBox,
			L"面板通过 HOME 呼出或隐藏。隐藏后如果某个功能没有立刻响应，优先重新呼出面板确认当前页状态，再看是不是游戏版本更新把偏移、蓝图结构或者事件链偷偷改了。",
			12.0f);
		AddAboutText(AboutBox,
			L"这一页只负责说明，不执行任何游戏逻辑；真正的功能入口请切到对应页签使用。遇到异常时，优先看日志，比盲猜靠谱得多。",
			0.0f);
	}

	AddPanelWithFixedGap(AboutPanel, 0.0f, 10.0f);
}
