#include <algorithm>

#include "PanelManager.hpp"
#include "GCManager.hpp"
#include "ItemBrowser.hpp"
#include "TabContent.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"
#include "Logging.hpp"

namespace
{
	constexpr bool kEnableUIInitLog = false;
	int32 GLastClosedTabIndex = -1;

	APlayerController* GInternalWidgetOwnerPC = nullptr;
	UGameInstance* GInternalWidgetOwnerGI = nullptr;

	bool IsValidPlayerController(APlayerController* PC)
	{
		return IsSafeLiveObject(static_cast<UObject*>(PC));
	}

	bool ShouldRecreateInternalWidget(APlayerController* CurrentPC)
	{
		if (!GInternalWidget)
			return false;

		auto* WidgetObj = static_cast<UObject*>(GInternalWidget);
		if (!IsSafeLiveObject(WidgetObj))
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
		if (!GInternalWidget)
			return;

		RememberUIControlStatesFromLiveWidgets();

		LOGI_STREAM("PanelManager") << "[SDK] ShowInternalWidget: recreate old widget, reason=" << (Reason ? Reason : "unknown") << "\n";

		if (IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)))
		{
			if (GInternalWidget->IsInViewport())
				GInternalWidget->RemoveFromParent();
			ClearGCRoot(GInternalWidget);
		}
		else
		{
			GRootedObjects.erase(
				std::remove(GRootedObjects.begin(), GRootedObjects.end(), static_cast<UObject*>(GInternalWidget)),
				GRootedObjects.end());
		}
		GInternalWidget = nullptr;
		GInternalWidgetVisible = false;
		GInternalWidgetOwnerPC = nullptr;
		GInternalWidgetOwnerGI = nullptr;
		ClearRuntimeWidgetState();
	}

	int32 GetCurrentTabOnClose(UBPMV_ConfigView2_C* CV)
	{
		if (!CV || !IsSafeLiveObject(static_cast<UObject*>(CV)))
			return -1;

		auto ReadVisibilityDirect = [](UWidget* W) -> ESlateVisibility
		{
			if (!W) return ESlateVisibility::Collapsed;
			return W->Visibility;
		};

		auto IsDynContentVisible = [&](UVerticalBox* Box) -> bool
		{
			if (!Box) return false;
			auto* Obj = static_cast<UObject*>(Box);
			if (!IsSafeLiveObject(Obj)) return false;
			if (Obj->Flags & EObjectFlags::BeginDestroyed) return false;
			if (Obj->Flags & EObjectFlags::FinishDestroyed) return false;
			return ReadVisibilityDirect(static_cast<UWidget*>(Box)) != ESlateVisibility::Collapsed;
		};

		if (IsDynContentVisible(GDynTab.Content6)) return 6;
		if (IsDynContentVisible(GDynTab.Content7)) return 7;
		if (IsDynContentVisible(GDynTab.Content8)) return 8;

		if (CV->CT_Contents &&
			IsSafeLiveObject(static_cast<UObject*>(CV->CT_Contents)))
		{
			const int32 ActiveNative = CV->CT_Contents->GetActiveWidgetIndex();
			if (ActiveNative >= 0 && ActiveNative <= 5)
				return ActiveNative;
		}

		return -1;
	}

}

int32 GetLastClosedTabIndex()
{
	return GLastClosedTabIndex;
}

void ClearRuntimeWidgetState()
{
	ResetRuntimeControlStateBindings();
	ClearItemBrowserState();

	GDynTab.Btn6 = nullptr;
	GDynTab.Btn7 = nullptr;
	GDynTab.Btn8 = nullptr;
	GDynTab.Content6 = nullptr;
	GDynTab.Content7 = nullptr;
	GDynTab.Content8 = nullptr;

	GTeammate.FollowToggle = nullptr;
	GTeammate.FollowCount = nullptr;
	GTeammate.AddDD = nullptr;
	GTeammate.ReplaceToggle = nullptr;
	GTeammate.ReplaceDD = nullptr;

	GQuest.ExecuteBtn = nullptr;
	GQuest.BtnWasPressed = false;
	GQuest.QuestDD = nullptr;
	GQuest.TypeDD = nullptr;
	GOriginalLanPanel = nullptr;
	GOriginalInputMappingPanel = nullptr;
	GOriginalResetButton = nullptr;
	GVolumeItems.clear();
	GVolumeLastValues.clear();
	GVolumeMinusWasPressed.clear();
	GVolumePlusWasPressed.clear();
	GTab1.ItemNoDecreaseToggle = nullptr;
	GTab1.ItemGainMultiplierToggle = nullptr;
	GTab1.ItemGainMultiplierSlider = nullptr;
	GTab1.AllItemsSellableToggle = nullptr;
	GTab1.DropRate100Toggle = nullptr;
	GTab1.CraftEffectMultiplierToggle = nullptr;
	GTab1.CraftItemIncrementSlider = nullptr;
	GTab1.CraftExtraEffectSlider = nullptr;
	GTab1.MaxExtraAffixesToggle = nullptr;
	GTab1.IgnoreItemUseCountToggle = nullptr;
	GTab1.IgnoreItemRequirementsToggle = nullptr;

	GTab3.CraftIgnoreRequirementsToggle = nullptr;

	GTab3.CraftOutputQuantityEdit = nullptr;
	GTab3.GatherCooldownToggle = nullptr;
	GTab3.FishRareOnlyToggle = nullptr;
	GTab3.FishAlwaysCatchToggle = nullptr;
	GTab3.HomelandHarvestToggle = nullptr;

	GTab4 = {};
	GTab5 = {};

	GCachedBtnExit = nullptr;
}

void InitializeConfigView2BySDK(UBPMV_ConfigView2_C* ConfigView)
{
	if (!ConfigView)
		return;

	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] Init: calling EVT_VisualConstructOnce...\n";
	ConfigView->EVT_VisualConstructOnce();

	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] Init: calling EVT_SetupSubModuleSlots...\n";
	ConfigView->EVT_SetupSubModuleSlots();

	if (kEnableUIInitLog)
	{
		LOGI_STREAM("PanelManager") << "[SDK] Init: VolumeSlot=" << (void*)ConfigView->VolumeSlot
		          << " children=" << (ConfigView->VolumeSlot ? ConfigView->VolumeSlot->GetChildrenCount() : -1) << "\n";
		LOGI_STREAM("PanelManager") << "[SDK] Init: VideoSlot=" << (void*)ConfigView->VideoSlot
		          << " children=" << (ConfigView->VideoSlot ? ConfigView->VideoSlot->GetChildrenCount() : -1) << "\n";
		LOGI_STREAM("PanelManager") << "[SDK] Init: CT_Contents=" << (void*)ConfigView->CT_Contents << "\n";
		LOGI_STREAM("PanelManager") << "[SDK] Init: SB_Global=" << (void*)ConfigView->SB_Global
		          << " VB_Global=" << (void*)ConfigView->VB_Global << "\n";
	}

	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] Init: calling EVT_SyncTabIndex(0)...\n";

	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] Init: calling EVT_SyncWithGlobalInputMode...\n";
	ConfigView->EVT_SyncWithGlobalInputMode();

	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] Init: calling EVT_VisualShow...\n";
	ConfigView->EVT_VisualShow();

	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] BPMV_ConfigView2_C initialized by SDK events (safe mode)\n";
}
void ApplyConfigView2TextPatch(UUserWidget* Widget, APlayerController* PC)
{
	if (!Widget || !Widget->IsA(UBPMV_ConfigView2_C::StaticClass()))
		return;

	auto* CV = static_cast<UBPMV_ConfigView2_C*>(Widget);

	SetupTab(CV->BTN_Sound,   0, L"角色");
	SetupTab(CV->BTN_Video,   1, L"物品");
	SetupTab(CV->BTN_Keys,    2, L"战斗");
	SetupTab(CV->BTN_Lan,     3, L"生活");
	SetupTab(CV->BTN_Others,  4, L"社交");
	SetupTab(CV->BTN_Gamepad, 5, L"系统");


	if (CV->IMG_Title)
		CV->IMG_Title->SetVisibility(ESlateVisibility::Collapsed);

	if (CV->Btn_Revert2)
	{
		GOriginalResetButton = CV->Btn_Revert2;
		if (GOriginalResetButton)
			MarkAsGCRoot(GOriginalResetButton);
		CV->Btn_Revert2->RemoveFromParent();
		LOGI_STREAM("PanelManager") << "[SDK] Btn_Revert2 removed from injected panel: " << (void*)CV->Btn_Revert2 << "\n";
	}

	if (CV->TXT_EnterTip)
		CV->TXT_EnterTip->RemoveFromParent();

	GCachedBtnExit = CV->BTN_Exit;

	PopulateTab_Character(CV, PC);
	PopulateTab_Items(CV, PC);
	PopulateTab_Battle(CV, PC);
	PopulateTab_Life(CV, PC);
	PopulateTab_Social(CV, PC);
	PopulateTab_System(CV, PC);

	CreateDynamicTabs(CV, PC);

	if (CV->BTN_Sound)   CV->BTN_Sound->EVT_UpdateActiveStatus(false);
	if (CV->BTN_Video)   CV->BTN_Video->EVT_UpdateActiveStatus(false);
	if (CV->BTN_Keys)    CV->BTN_Keys->EVT_UpdateActiveStatus(false);
	if (CV->BTN_Lan)     CV->BTN_Lan->EVT_UpdateActiveStatus(false);
	if (CV->BTN_Others)  CV->BTN_Others->EVT_UpdateActiveStatus(false);
	if (CV->BTN_Gamepad) CV->BTN_Gamepad->EVT_UpdateActiveStatus(false);
	auto ResetDynamicTabButtonVisual = [](UBP_JHConfigTabBtn_C* Btn)
	{
		if (!Btn || !IsSafeLiveObject(static_cast<UObject*>(Btn)))
			return;
		if (Btn->IMG_Active &&
			IsSafeLiveObject(static_cast<UObject*>(Btn->IMG_Active)))
		{
			Btn->IMG_Active->SetRenderOpacity(0.0f);
			Btn->IMG_Active->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Btn->JHGPCBtn_ActiveBG &&
			IsSafeLiveObject(static_cast<UObject*>(Btn->JHGPCBtn_ActiveBG)))
		{
			Btn->JHGPCBtn_ActiveBG->SetRenderOpacity(0.0f);
			Btn->JHGPCBtn_ActiveBG->SetVisibility(ESlateVisibility::Collapsed);
		}
	};
	ResetDynamicTabButtonVisual(GDynTab.Btn6);
	ResetDynamicTabButtonVisual(GDynTab.Btn7);
	ResetDynamicTabButtonVisual(GDynTab.Btn8);

	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] ConfigView2 patched: 9 tabs populated\n";
}


void CreateDynamicTabs(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!CV || !PC) return;

	GDynTab.Btn6 = nullptr;
	GDynTab.Btn7 = nullptr;
	GDynTab.Btn8 = nullptr;
	GDynTab.Content6 = nullptr;
	GDynTab.Content7 = nullptr;
	GDynTab.Content8 = nullptr;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
	                            : static_cast<UObject*>(CV);

	GDynTab.Btn6 = CreateTabButton(PC);
	if (GDynTab.Btn6)
	{
		SetupTab(GDynTab.Btn6, 6, L"\u961F\u53CB");
		PatchTabBtnRuntimeContext(GDynTab.Btn6, CV, "DynTab6");
		if (CV->CT_TabBtns)
			CV->CT_TabBtns->AddChild(GDynTab.Btn6);
		if (kEnableUIInitLog)
			LOGI_STREAM("PanelManager") << "[SDK] DynTab6 button created\n";
	}

	GDynTab.Btn7 = CreateTabButton(PC);
	if (GDynTab.Btn7)
	{
		SetupTab(GDynTab.Btn7, 7, L"\u4EFB\u52A1");
		PatchTabBtnRuntimeContext(GDynTab.Btn7, CV, "DynTab7");
		if (CV->CT_TabBtns)
			CV->CT_TabBtns->AddChild(GDynTab.Btn7);
		if (kEnableUIInitLog)
			LOGI_STREAM("PanelManager") << "[SDK] DynTab7 button created\n";
	}

	GDynTab.Btn8 = CreateTabButton(PC);
	if (GDynTab.Btn8)
	{
		SetupTab(GDynTab.Btn8, 8, L"\u5173\u4E8E");
		PatchTabBtnRuntimeContext(GDynTab.Btn8, CV, "DynTab8");
		if (CV->CT_TabBtns)
			CV->CT_TabBtns->AddChild(GDynTab.Btn8);
		if (kEnableUIInitLog)
			LOGI_STREAM("PanelManager") << "[SDK] DynTab8 button created\n";
	}

	UPanelWidget* SwitcherParent = CV->CT_Contents ? CV->CT_Contents->GetParent() : nullptr;
	if (kEnableUIInitLog)
		LOGI_STREAM("PanelManager") << "[SDK] DynTab: SwitcherParent=" << (void*)SwitcherParent << "\n";

	GDynTab.Content6 = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (GDynTab.Content6 && SwitcherParent)
	{
		SwitcherParent->AddChild(GDynTab.Content6);
		GDynTab.Content6->SetVisibility(ESlateVisibility::Collapsed);
		if (kEnableUIInitLog)
			LOGI_STREAM("PanelManager") << "[SDK] DynTab6 content added to SwitcherParent (Collapsed)\n";
	}

	GDynTab.Content7 = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (GDynTab.Content7 && SwitcherParent)
	{
		SwitcherParent->AddChild(GDynTab.Content7);
		GDynTab.Content7->SetVisibility(ESlateVisibility::Collapsed);
		if (kEnableUIInitLog)
			LOGI_STREAM("PanelManager") << "[SDK] DynTab7 content added to SwitcherParent (Collapsed)\n";
	}

	GDynTab.Content8 = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (GDynTab.Content8 && SwitcherParent)
	{
		SwitcherParent->AddChild(GDynTab.Content8);
		GDynTab.Content8->SetVisibility(ESlateVisibility::Collapsed);
		if (kEnableUIInitLog)
			LOGI_STREAM("PanelManager") << "[SDK] DynTab8 content added to SwitcherParent (Collapsed)\n";
	}

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
			LOGI_STREAM("PanelManager") << "[SDK] Class exists but create failed: " << ClassName << "\n";
			return nullptr;
		}

		CachedWorkingClass = WidgetClass;
		CachedWorkingClassName = ClassName;
		LOGI_STREAM("PanelManager") << "[SDK] Internal widget created from class: " << ClassName << "\n";
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

	LOGI_STREAM("PanelManager") << "[SDK] Failed to create internal widget from all candidates\n";
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
	if (GInternalWidget && GInternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass()))
	{
		GLastClosedTabIndex = GetCurrentTabOnClose(static_cast<UBPMV_ConfigView2_C*>(GInternalWidget));
		LOGI_STREAM("PanelManager") << "[SDK] Remember tab on hide: idx=" << GLastClosedTabIndex << "\n";
	}

	RememberUIControlStatesFromLiveWidgets();

	if (GInternalWidget && IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)))
	{
		if (GInternalWidget->IsInViewport())
			GInternalWidget->RemoveFromParent();
	}
	else if (GInternalWidget)
	{
		GRootedObjects.erase(
			std::remove(GRootedObjects.begin(), GRootedObjects.end(), static_cast<UObject*>(GInternalWidget)),
			GRootedObjects.end());
		GInternalWidget = nullptr;
		GInternalWidgetOwnerPC = nullptr;
		GInternalWidgetOwnerGI = nullptr;
	}
	GCachedBtnExit = nullptr;

	if (IsValidPlayerController(PlayerController))
	{
		UGameplayStatics::SetGamePaused(PlayerController, false);
		PlayerController->bShowMouseCursor = true;
	}
	GInternalWidgetVisible = false;

	LOGI_STREAM("PanelManager") << "[SDK] Home: internal widget hidden (cached), game resumed\n";
}
void DestroyInternalWidget(APlayerController* PlayerController)
{
	if (GInternalWidget && GInternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass()))
	{
		GLastClosedTabIndex = GetCurrentTabOnClose(static_cast<UBPMV_ConfigView2_C*>(GInternalWidget));
		LOGI_STREAM("PanelManager") << "[SDK] Remember tab on destroy: idx=" << GLastClosedTabIndex << "\n";
	}

	RememberUIControlStatesFromLiveWidgets();

	if (GInternalWidget && IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)) && GInternalWidget->IsInViewport())
		GInternalWidget->RemoveFromParent();
	GCachedBtnExit = nullptr;

	ClearAllGCRoots();
	GInternalWidget = nullptr;
	ClearRuntimeWidgetState();
	GInternalWidgetOwnerPC = nullptr;
	GInternalWidgetOwnerGI = nullptr;

	if (IsValidPlayerController(PlayerController))
	{
		UGameplayStatics::SetGamePaused(PlayerController, false);
		PlayerController->bShowMouseCursor = true;
	}
	GInternalWidgetVisible = false;

	LOGI_STREAM("PanelManager") << "[SDK] Internal widget destroyed and fully cleaned\n";
}
void ShowInternalWidget(APlayerController* PlayerController)
{
	if (!IsValidPlayerController(PlayerController))
	{
		LOGI_STREAM("PanelManager") << "[SDK] ShowInternalWidget: invalid player controller\n";
		return;
	}

	if (ShouldRecreateInternalWidget(PlayerController))
	{
		const char* reason = "owner/world mismatch or stale pointer";
		if (GInternalWidget && !IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)))
			reason = "internal widget invalid";
		ReleaseInternalWidgetForRecreate(reason);
	}

	bool bCreatedNow = false;
	if (!GInternalWidget)
	{
		GInternalWidget = CreateInternalWidgetInstance(PlayerController);
		if (!GInternalWidget)
			return;
		MarkAsGCRoot(GInternalWidget);
		bCreatedNow = true;
	}

	if (!IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)))
	{
		LOGI_STREAM("PanelManager") << "[SDK] ShowInternalWidget: internal widget invalid after create\n";
		ReleaseInternalWidgetForRecreate("invalid after create");
		return;
	}

	GInternalWidgetOwnerPC = PlayerController;
	GInternalWidgetOwnerGI = UGameplayStatics::GetGameInstance(PlayerController);
	GInternalWidget->SetOwningPlayer(PlayerController);

	if (!GInternalWidget->IsInViewport())
		GInternalWidget->AddToViewport(10000);

	if (bCreatedNow)
	{
		if (GInternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass()))
			InitializeConfigView2BySDK(static_cast<UBPMV_ConfigView2_C*>(GInternalWidget));

		ApplyConfigView2TextPatch(GInternalWidget, PlayerController);
		GInternalWidget->SetRenderTransformPivot(FVector2D{ 0.5f, 0.5f });
		GInternalWidget->SetRenderScale(FVector2D{ kInternalPanelScale, kInternalPanelScale });
	}

	if (!IsValidPlayerController(PlayerController) ||
		!IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)))
	{
		LOGI_STREAM("PanelManager") << "[SDK] ShowInternalWidget: aborted before SetInputMode, invalid runtime objects\n";
		return;
	}

	GInternalWidget->SetKeyboardFocus();
	UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PlayerController, GInternalWidget, EMouseLockMode::DoNotLock, false);
	PlayerController->bShowMouseCursor = true;
	UGameplayStatics::SetGamePaused(PlayerController, true);
	GInternalWidgetVisible = true;

	LOGI_STREAM("PanelManager") << "[SDK] Home: internal widget shown, game paused\n";
}
void ToggleInternalWidget()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		LOGI_STREAM("PanelManager") << "[SDK] Home: player controller not ready\n";
		return;
	}

	if (GInternalWidget && !IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)))
	{
		LOGI_STREAM("PanelManager") << "[SDK] ToggleInternalWidget: stale internal widget detected, force recreate\n";
		GRootedObjects.erase(
			std::remove(GRootedObjects.begin(), GRootedObjects.end(), static_cast<UObject*>(GInternalWidget)),
			GRootedObjects.end());
		GInternalWidget = nullptr;
		GInternalWidgetVisible = false;
		GCachedBtnExit = nullptr;
		GInternalWidgetOwnerPC = nullptr;
		GInternalWidgetOwnerGI = nullptr;
		ClearRuntimeWidgetState();
	}

	bool isShowing = GInternalWidget && IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)) && GInternalWidget->IsInViewport();
	if (isShowing)
		HideInternalWidget(PlayerController);
	else
		ShowInternalWidget(PlayerController);
}

void ShowDynamicTab(UBPMV_ConfigView2_C* CV, int32 DynIdx)
{
	if (CV->CT_Contents &&
		IsSafeLiveObject(static_cast<UObject*>(CV->CT_Contents)))
		CV->CT_Contents->SetVisibility(ESlateVisibility::Collapsed);
	if (GDynTab.Content6 &&
		IsSafeLiveObject(static_cast<UObject*>(GDynTab.Content6))) GDynTab.Content6->SetVisibility(
		DynIdx == 6 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (GDynTab.Content7 &&
		IsSafeLiveObject(static_cast<UObject*>(GDynTab.Content7))) GDynTab.Content7->SetVisibility(
		DynIdx == 7 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (GDynTab.Content8 &&
		IsSafeLiveObject(static_cast<UObject*>(GDynTab.Content8))) GDynTab.Content8->SetVisibility(
		DynIdx == 8 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (DynIdx == 8 &&
		GDynTab.Content8 &&
		IsSafeLiveObject(static_cast<UObject*>(GDynTab.Content8)))
	{
		int32 Cnt = GDynTab.Content8->GetChildrenCount();
		UWidget* Child0 = (Cnt > 0) ? GDynTab.Content8->GetChildAt(0) : nullptr;
		LOGI_STREAM("PanelManager") << "[SDK] ShowDynamicTab8: content=" << (void*)GDynTab.Content8
		          << " children=" << Cnt
		          << " vis=" << ToVisName(GDynTab.Content8->GetVisibility())
		          << " child0=" << (void*)Child0
		          << " child0Vis=" << (Child0 ? ToVisName(Child0->GetVisibility()) : "null")
		          << "\n";
	}
}
void ShowOriginalTab(UBPMV_ConfigView2_C* CV)
{
	if (CV->CT_Contents &&
		IsSafeLiveObject(static_cast<UObject*>(CV->CT_Contents)))
		CV->CT_Contents->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (GDynTab.Content6 &&
		IsSafeLiveObject(static_cast<UObject*>(GDynTab.Content6))) GDynTab.Content6->SetVisibility(ESlateVisibility::Collapsed);
	if (GDynTab.Content7 &&
		IsSafeLiveObject(static_cast<UObject*>(GDynTab.Content7))) GDynTab.Content7->SetVisibility(ESlateVisibility::Collapsed);
	if (GDynTab.Content8 &&
		IsSafeLiveObject(static_cast<UObject*>(GDynTab.Content8))) GDynTab.Content8->SetVisibility(ESlateVisibility::Collapsed);
}
