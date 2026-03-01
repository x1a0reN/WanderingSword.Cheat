#include <iostream>
#include <algorithm>

#include "PanelManager.hpp"
#include "GCManager.hpp"
#include "ItemBrowser.hpp"
#include "TabContent.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"

namespace
{
	constexpr bool kEnableUIInitLog = false;

	APlayerController* GInternalWidgetOwnerPC = nullptr;
	UGameInstance* GInternalWidgetOwnerGI = nullptr;

	bool IsValidUObject(UObject* Obj)
	{
		return IsSafeLiveObject(Obj);
	}

	bool IsValidPlayerController(APlayerController* PC)
	{
		return IsValidUObject(static_cast<UObject*>(PC));
	}

	bool ShouldRecreateInternalWidget(APlayerController* CurrentPC)
	{
		if (!InternalWidget)
			return false;

		auto* WidgetObj = static_cast<UObject*>(InternalWidget);
		if (!IsValidUObject(WidgetObj))
			return true;

		if (!IsValidPlayerController(CurrentPC))
			return true;

		if (GInternalWidgetOwnerPC && GInternalWidgetOwnerPC != CurrentPC)
			return true;

		UGameInstance* CurrentGI = UGameplayStatics::GetGameInstance(CurrentPC);
		if (GInternalWidgetOwnerGI && GInternalWidgetOwnerGI != CurrentGI)
			return true;

		return false;
	}

	void ReleaseInternalWidgetForRecreate(const char* Reason)
	{
		if (!InternalWidget)
			return;

		std::cout << "[SDK] ShowInternalWidget: recreate old widget, reason=" << (Reason ? Reason : "unknown") << "\n";

		if (IsValidUObject(static_cast<UObject*>(InternalWidget)))
		{
			if (InternalWidget->IsInViewport())
				InternalWidget->RemoveFromParent();
			ClearGCRoot(InternalWidget);
		}
		else
		{
			GRootedObjects.erase(
				std::remove(GRootedObjects.begin(), GRootedObjects.end(), static_cast<UObject*>(InternalWidget)),
				GRootedObjects.end());
		}
		InternalWidget = nullptr;
		InternalWidgetVisible = false;
		GInternalWidgetOwnerPC = nullptr;
		GInternalWidgetOwnerGI = nullptr;
		ClearRuntimeWidgetState();
	}
}
void ClearRuntimeWidgetState()
{
	ClearItemBrowserState();

	GDynTabBtn6 = nullptr;
	GDynTabBtn7 = nullptr;
	GDynTabBtn8 = nullptr;
	GDynTabContent6 = nullptr;
	GDynTabContent7 = nullptr;
	GDynTabContent8 = nullptr;

	GTeammateFollowToggle = nullptr;
	GTeammateFollowCount = nullptr;
	GTeammateAddDD = nullptr;
	GTeammateReplaceToggle = nullptr;
	GTeammateReplaceDD = nullptr;

	GQuestToggle = nullptr;
	GQuestTypeDD = nullptr;
	GOriginalLanPanel = nullptr;
	GOriginalInputMappingPanel = nullptr;
	GOriginalResetButton = nullptr;
	GVolumeItems.clear();
	GVolumeLastValues.clear();
	GVolumeMinusWasPressed.clear();
	GVolumePlusWasPressed.clear();
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

	GCachedBtnExit = nullptr;
}

// ── Raw widget creation via StaticConstructObject_Internal ──
// Found by decompiling UWidgetBlueprintLibrary::Create → CreateWidget → StaticConstructObject_Internal
// RVA 0x17C6140 in JH-Win64-Shipping.exe
void InitializeConfigView2BySDK(UBPMV_ConfigView2_C* ConfigView)
{
	if (!ConfigView)
		return;

	if (kEnableUIInitLog)
		std::cout << "[SDK] Init: calling EVT_VisualConstructOnce...\n";
	ConfigView->EVT_VisualConstructOnce();

	if (kEnableUIInitLog)
		std::cout << "[SDK] Init: calling EVT_SetupSubModuleSlots...\n";
	ConfigView->EVT_SetupSubModuleSlots();

	// Diagnostic: check slot state after setup
	if (kEnableUIInitLog)
	{
		std::cout << "[SDK] Init: VolumeSlot=" << (void*)ConfigView->VolumeSlot
		          << " children=" << (ConfigView->VolumeSlot ? ConfigView->VolumeSlot->GetChildrenCount() : -1) << "\n";
		std::cout << "[SDK] Init: VideoSlot=" << (void*)ConfigView->VideoSlot
		          << " children=" << (ConfigView->VideoSlot ? ConfigView->VideoSlot->GetChildrenCount() : -1) << "\n";
		std::cout << "[SDK] Init: CT_Contents=" << (void*)ConfigView->CT_Contents << "\n";
		std::cout << "[SDK] Init: SB_Global=" << (void*)ConfigView->SB_Global
		          << " VB_Global=" << (void*)ConfigView->VB_Global << "\n";
	}

	if (kEnableUIInitLog)
		std::cout << "[SDK] Init: calling EVT_SyncTabIndex(0)...\n";
	ConfigView->EVT_SyncTabIndex(0);

	if (kEnableUIInitLog)
		std::cout << "[SDK] Init: calling EVT_SyncWithGlobalInputMode...\n";
	ConfigView->EVT_SyncWithGlobalInputMode();

	if (kEnableUIInitLog)
		std::cout << "[SDK] Init: calling EVT_VisualShow...\n";
	ConfigView->EVT_VisualShow();

	if (kEnableUIInitLog)
		std::cout << "[SDK] BPMV_ConfigView2_C initialized by SDK events (safe mode)\n";
}
void ApplyConfigView2TextPatch(UUserWidget* Widget, APlayerController* PC)
{
	if (!Widget || !Widget->IsA(UBPMV_ConfigView2_C::StaticClass()))
		return;

	auto* CV = static_cast<UBPMV_ConfigView2_C*>(Widget);

	// ── Rename existing 6 tabs + hide icons ──
	// [角色] 主角数据/属性/战斗属性/精通经验/队伍数据
	// [物品] 物品不减/加倍/全可售/装备编辑/掉落率/添加物品(列表)
	// [战斗] 伤害加倍/无视冷却/战斗加速/不遇敌/全员参战/心法/战败=胜利
	// [生活] 锻造/制衣/炼丹/烹饪/采集/钓鱼/家园
	// [社交] 好感/送礼/邀请/切磋/请教/NPC装备/强制NPC互动
	// [系统] 跳跃/移动/坐骑(下拉框)/难度/GM命令/解锁图鉴成就/屏幕
	// [队友] 添加队友(列表)/移除队友/替换指定队友/跟随数量 (动态新增)
	// [任务] 立刻接到或完成未做过的任务(列表)              (动态新增)
	SetupTab(CV->BTN_Sound,   0, L"\u89D2\u8272");   // 角色
	SetupTab(CV->BTN_Video,   1, L"\u7269\u54C1");   // 物品
	SetupTab(CV->BTN_Keys,    2, L"\u6218\u6597");   // 战斗
	SetupTab(CV->BTN_Lan,     3, L"\u751F\u6D3B");   // 生活
	SetupTab(CV->BTN_Others,  4, L"\u793E\u4EA4");   // 社交
	SetupTab(CV->BTN_Gamepad, 5, L"\u7CFB\u7EDF");   // 系统

	// NOTE: Do NOT call EstablishTabBtns here — it resets internal click bindings
	// that EVT_VisualConstructOnce() already established, breaking tab switching.

	// ── Hide title image ("游戏设置" is baked into a texture) ──
	if (CV->IMG_Title)
		CV->IMG_Title->SetVisibility(ESlateVisibility::Collapsed);

	// ── Hide/remove refresh/reset button from injected panel (restore original behavior) ──
	if (CV->Btn_Revert2)
	{
		GOriginalResetButton = CV->Btn_Revert2;
		if (GOriginalResetButton)
			MarkAsGCRoot(GOriginalResetButton);
		CV->Btn_Revert2->RemoveFromParent();
		std::cout << "[SDK] Btn_Revert2 removed from injected panel: " << (void*)CV->Btn_Revert2 << "\n";
	}

	// ── Remove tip text (SetVisibility gets overridden by blueprint) ──
	if (CV->TXT_EnterTip)
		CV->TXT_EnterTip->RemoveFromParent();

	// ── Cache close button for per-frame click detection ──
	GCachedBtnExit = CV->BTN_Exit;

	// ── Populate tab contents ──
	PopulateTab_Character(CV, PC);
	PopulateTab_Items(CV, PC);
	PopulateTab_Battle(CV, PC);
	PopulateTab_Life(CV, PC);
	PopulateTab_Social(CV, PC);
	PopulateTab_System(CV, PC);

	// ── Create dynamic tabs 6/7/8 ──
	CreateDynamicTabs(CV, PC);

	// Default first tab active
	if (CV->BTN_Sound)
		CV->BTN_Sound->EVT_UpdateActiveStatus(true);
	if (GDynTabBtn6)
		GDynTabBtn6->EVT_UpdateActiveStatus(false);
	if (GDynTabBtn7)
		GDynTabBtn7->EVT_UpdateActiveStatus(false);
	if (GDynTabBtn8)
		GDynTabBtn8->EVT_UpdateActiveStatus(false);

	if (kEnableUIInitLog)
		std::cout << "[SDK] ConfigView2 patched: 9 tabs populated\n";
}

// ── Tab content population ──

// Replace game's sub-module panel in a NamedSlot with our own clean VBox.
// The original sub-module panels have blueprint Tick/animations that crash
// when we remove their children — so we detach the entire panel instead.
void CreateDynamicTabs(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!CV || !PC) return;

	// Reset dynamic tab state
	GDynTabBtn6 = nullptr;
	GDynTabBtn7 = nullptr;
	GDynTabBtn8 = nullptr;
	GDynTabContent6 = nullptr;
	GDynTabContent7 = nullptr;
	GDynTabContent8 = nullptr;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
	                            : static_cast<UObject*>(CV);

	// ── Create tab buttons ──
	GDynTabBtn6 = CreateTabButton(PC);
	if (GDynTabBtn6)
	{
		SetupTab(GDynTabBtn6, 6, L"\u961F\u53CB"); // 队友
		PatchTabBtnRuntimeContext(GDynTabBtn6, CV, "DynTab6");
		if (CV->CT_TabBtns)
			CV->CT_TabBtns->AddChild(GDynTabBtn6);
		if (kEnableUIInitLog)
			std::cout << "[SDK] DynTab6 button created\n";
	}

	GDynTabBtn7 = CreateTabButton(PC);
	if (GDynTabBtn7)
	{
		SetupTab(GDynTabBtn7, 7, L"\u4EFB\u52A1"); // 任务
		PatchTabBtnRuntimeContext(GDynTabBtn7, CV, "DynTab7");
		if (CV->CT_TabBtns)
			CV->CT_TabBtns->AddChild(GDynTabBtn7);
		if (kEnableUIInitLog)
			std::cout << "[SDK] DynTab7 button created\n";
	}

	GDynTabBtn8 = CreateTabButton(PC);
	if (GDynTabBtn8)
	{
		SetupTab(GDynTabBtn8, 8, L"\u63A7\u4EF6"); // 控件
		PatchTabBtnRuntimeContext(GDynTabBtn8, CV, "DynTab8");
		if (CV->CT_TabBtns)
			CV->CT_TabBtns->AddChild(GDynTabBtn8);
		if (kEnableUIInitLog)
			std::cout << "[SDK] DynTab8 button created\n";
	}

	// ── Create content containers (mounted to Switcher's parent, not Switcher itself) ──
	UPanelWidget* SwitcherParent = CV->CT_Contents ? CV->CT_Contents->GetParent() : nullptr;
	if (kEnableUIInitLog)
		std::cout << "[SDK] DynTab: SwitcherParent=" << (void*)SwitcherParent << "\n";

	GDynTabContent6 = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (GDynTabContent6 && SwitcherParent)
	{
		SwitcherParent->AddChild(GDynTabContent6);
		GDynTabContent6->SetVisibility(ESlateVisibility::Collapsed);
		if (kEnableUIInitLog)
			std::cout << "[SDK] DynTab6 content added to SwitcherParent (Collapsed)\n";
	}

	GDynTabContent7 = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (GDynTabContent7 && SwitcherParent)
	{
		SwitcherParent->AddChild(GDynTabContent7);
		GDynTabContent7->SetVisibility(ESlateVisibility::Collapsed);
		if (kEnableUIInitLog)
			std::cout << "[SDK] DynTab7 content added to SwitcherParent (Collapsed)\n";
	}

	GDynTabContent8 = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (GDynTabContent8 && SwitcherParent)
	{
		SwitcherParent->AddChild(GDynTabContent8);
		GDynTabContent8->SetVisibility(ESlateVisibility::Collapsed);
		if (kEnableUIInitLog)
			std::cout << "[SDK] DynTab8 content added to SwitcherParent (Collapsed)\n";
	}

	// ── Populate content ──
	PopulateTab_Teammates(CV, PC);
	PopulateTab_Quests(CV, PC);
	PopulateTab_Controls(CV, PC);
}
APlayerController* GetFirstLocalPlayerController()
{
	UWorld* World = UWorld::GetWorld();
	if (!World)
		return nullptr;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (IsValidPlayerController(PC))
			return PC;
	}

	if (World->OwningGameInstance && World->OwningGameInstance->LocalPlayers.Num() > 0)
	{
		ULocalPlayer* LP = World->OwningGameInstance->LocalPlayers[0];
		if (LP && IsValidPlayerController(LP->PlayerController))
			return LP->PlayerController;
	}

	if (World->PersistentLevel && World->PersistentLevel->OwningWorld)
	{
		UWorld* OW = World->PersistentLevel->OwningWorld;
		if (OW && OW->OwningGameInstance && OW->OwningGameInstance->LocalPlayers.Num() > 0)
		{
			ULocalPlayer* LP = OW->OwningGameInstance->LocalPlayers[0];
			if (LP && IsValidPlayerController(LP->PlayerController))
				return LP->PlayerController;
		}
	}

	return nullptr;
}
UUserWidget* CreateInternalWidgetInstance(APlayerController* PlayerController)
{
	static UClass* CachedWorkingClass = nullptr;
	static const char* CachedWorkingClassName = nullptr;

	auto TryCreate = [&](UClass* WidgetClass, const char* ClassName) -> UUserWidget*
	{
		if (!WidgetClass)
			return nullptr;

		UUserWidget* Widget = UWidgetBlueprintLibrary::Create(PlayerController, WidgetClass, PlayerController);
		if (!Widget)
		{
			std::cout << "[SDK] Class exists but create failed: " << ClassName << "\n";
			return nullptr;
		}

		CachedWorkingClass = WidgetClass;
		CachedWorkingClassName = ClassName;
		std::cout << "[SDK] Internal widget created from class: " << ClassName << "\n";
		return Widget;
	};

	if (CachedWorkingClass)
	{
		if (UUserWidget* Widget = TryCreate(CachedWorkingClass, CachedWorkingClassName ? CachedWorkingClassName : "CachedWorkingClass"))
			return Widget;

		CachedWorkingClass = nullptr;
		CachedWorkingClassName = nullptr;
	}

	constexpr const char* CandidateWidgetClasses[] =
	{
		"BPMV_ConfigView2_C",
		"BPMV_Prime_C",
		"BPMV_PrimeQuest_C",
		"BPMV_PrimeMeridian_C",
		"BPMV_JHNeoUISkill_C",
		"BPMV_JHSystem_C",
		"BPMV_GMCommand_C",
		"WBP_GM0_C",
	};

	for (const char* ClassName : CandidateWidgetClasses)
	{
		UClass* WidgetClass = UObject::FindClassFast(ClassName);
		if (!WidgetClass)
			WidgetClass = UObject::FindClass(ClassName);

		if (UUserWidget* Widget = TryCreate(WidgetClass, ClassName))
			return Widget;
	}

	std::cout << "[SDK] Failed to create internal widget from all candidates\n";
	return nullptr;
}
void EnsureMouseCursorVisible()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (IsValidPlayerController(PlayerController) && !PlayerController->bShowMouseCursor)
		PlayerController->bShowMouseCursor = true;
}
void HideInternalWidget(APlayerController* PlayerController)
{
	if (InternalWidget && IsValidUObject(static_cast<UObject*>(InternalWidget)))
	{
		if (InternalWidget->IsInViewport())
			InternalWidget->RemoveFromParent();
	}
	else if (InternalWidget)
	{
		GRootedObjects.erase(
			std::remove(GRootedObjects.begin(), GRootedObjects.end(), static_cast<UObject*>(InternalWidget)),
			GRootedObjects.end());
		InternalWidget = nullptr;
		GInternalWidgetOwnerPC = nullptr;
		GInternalWidgetOwnerGI = nullptr;
	}
	GCachedBtnExit = nullptr;

	if (IsValidPlayerController(PlayerController))
	{
		UGameplayStatics::SetGamePaused(PlayerController, false);
		PlayerController->bShowMouseCursor = true;
	}
	InternalWidgetVisible = false;

	std::cout << "[SDK] Home: internal widget hidden (cached), game resumed\n";
}
void DestroyInternalWidget(APlayerController* PlayerController)
{
	if (InternalWidget && IsValidUObject(static_cast<UObject*>(InternalWidget)) && InternalWidget->IsInViewport())
		InternalWidget->RemoveFromParent();
	GCachedBtnExit = nullptr;

	ClearAllGCRoots();
	InternalWidget = nullptr;
	ClearRuntimeWidgetState();
	GInternalWidgetOwnerPC = nullptr;
	GInternalWidgetOwnerGI = nullptr;

	if (IsValidPlayerController(PlayerController))
	{
		UGameplayStatics::SetGamePaused(PlayerController, false);
		PlayerController->bShowMouseCursor = true;
	}
	InternalWidgetVisible = false;

	std::cout << "[SDK] Internal widget destroyed and fully cleaned\n";
}
void ShowInternalWidget(APlayerController* PlayerController)
{
	if (!IsValidPlayerController(PlayerController))
	{
		std::cout << "[SDK] ShowInternalWidget: invalid player controller\n";
		return;
	}

	if (ShouldRecreateInternalWidget(PlayerController))
	{
		const char* reason = "owner/world mismatch or stale pointer";
		if (InternalWidget && !IsValidUObject(static_cast<UObject*>(InternalWidget)))
			reason = "internal widget invalid";
		ReleaseInternalWidgetForRecreate(reason);
	}

	bool bCreatedNow = false;
	if (!InternalWidget)
	{
		InternalWidget = CreateInternalWidgetInstance(PlayerController);
		if (!InternalWidget)
			return;
		MarkAsGCRoot(InternalWidget);
		bCreatedNow = true;
	}

	if (!IsValidUObject(static_cast<UObject*>(InternalWidget)))
	{
		std::cout << "[SDK] ShowInternalWidget: internal widget invalid after create\n";
		ReleaseInternalWidgetForRecreate("invalid after create");
		return;
	}

	GInternalWidgetOwnerPC = PlayerController;
	GInternalWidgetOwnerGI = UGameplayStatics::GetGameInstance(PlayerController);
	InternalWidget->SetOwningPlayer(PlayerController);

	if (!InternalWidget->IsInViewport())
		InternalWidget->AddToViewport(10000);

	if (bCreatedNow)
	{
		if (InternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass()))
			InitializeConfigView2BySDK(static_cast<UBPMV_ConfigView2_C*>(InternalWidget));

		ApplyConfigView2TextPatch(InternalWidget, PlayerController);
		InternalWidget->SetRenderTransformPivot(FVector2D{ 0.5f, 0.5f });
		InternalWidget->SetRenderScale(FVector2D{ kInternalPanelScale, kInternalPanelScale });
	}

	if (!IsValidPlayerController(PlayerController) ||
		!IsValidUObject(static_cast<UObject*>(InternalWidget)))
	{
		std::cout << "[SDK] ShowInternalWidget: aborted before SetInputMode, invalid runtime objects\n";
		return;
	}

	InternalWidget->SetKeyboardFocus();
	UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PlayerController, InternalWidget, EMouseLockMode::DoNotLock, false);
	PlayerController->bShowMouseCursor = true;
	UGameplayStatics::SetGamePaused(PlayerController, true);
	InternalWidgetVisible = true;

	std::cout << "[SDK] Home: internal widget shown, game paused\n";
}
void ToggleInternalWidget()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		std::cout << "[SDK] Home: player controller not ready\n";
		return;
	}

	if (InternalWidget && !IsValidUObject(static_cast<UObject*>(InternalWidget)))
	{
		std::cout << "[SDK] ToggleInternalWidget: stale internal widget detected, force recreate\n";
		GRootedObjects.erase(
			std::remove(GRootedObjects.begin(), GRootedObjects.end(), static_cast<UObject*>(InternalWidget)),
			GRootedObjects.end());
		InternalWidget = nullptr;
		InternalWidgetVisible = false;
		GCachedBtnExit = nullptr;
		GInternalWidgetOwnerPC = nullptr;
		GInternalWidgetOwnerGI = nullptr;
		ClearRuntimeWidgetState();
	}

	// Use real widget state instead of our boolean flag
	bool isShowing = InternalWidget && IsValidUObject(static_cast<UObject*>(InternalWidget)) && InternalWidget->IsInViewport();
	if (isShowing)
		HideInternalWidget(PlayerController);
	else
		ShowInternalWidget(PlayerController);
}

// ── Dynamic tab visibility helpers (avoid SetActiveWidgetIndex which triggers blueprint crash) ──
void ShowDynamicTab(UBPMV_ConfigView2_C* CV, int32 DynIdx)
{
	if (CV->CT_Contents)
		CV->CT_Contents->SetVisibility(ESlateVisibility::Collapsed);
	if (GDynTabContent6) GDynTabContent6->SetVisibility(
		DynIdx == 6 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (GDynTabContent7) GDynTabContent7->SetVisibility(
		DynIdx == 7 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (GDynTabContent8) GDynTabContent8->SetVisibility(
		DynIdx == 8 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (DynIdx == 8 && GDynTabContent8)
	{
		int32 Cnt = GDynTabContent8->GetChildrenCount();
		UWidget* Child0 = (Cnt > 0) ? GDynTabContent8->GetChildAt(0) : nullptr;
		std::cout << "[SDK] ShowDynamicTab8: content=" << (void*)GDynTabContent8
		          << " children=" << Cnt
		          << " vis=" << ToVisName(GDynTabContent8->GetVisibility())
		          << " child0=" << (void*)Child0
		          << " child0Vis=" << (Child0 ? ToVisName(Child0->GetVisibility()) : "null")
		          << "\n";
	}
}
void ShowOriginalTab(UBPMV_ConfigView2_C* CV)
{
	if (CV->CT_Contents)
		CV->CT_Contents->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (GDynTabContent6) GDynTabContent6->SetVisibility(ESlateVisibility::Collapsed);
	if (GDynTabContent7) GDynTabContent7->SetVisibility(ESlateVisibility::Collapsed);
	if (GDynTabContent8) GDynTabContent8->SetVisibility(ESlateVisibility::Collapsed);
}



