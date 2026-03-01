#include <Windows.h>
#include <iostream>
#include <cmath>

#include "CheatState.hpp"
#include "FrameHook.hpp"
#include "ItemBrowser.hpp"
#include "PanelManager.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"

namespace
{
	struct PostRenderInFlightScope final
	{
		PostRenderInFlightScope()
		{
			GPostRenderInFlight.fetch_add(1, std::memory_order_acq_rel);
		}

		~PostRenderInFlightScope()
		{
			GPostRenderInFlight.fetch_sub(1, std::memory_order_acq_rel);
		}
	};

	bool IsPointerInLiveObjectArray(UObject* Obj)
	{
		if (!Obj)
			return false;

		auto* ObjArray = UObject::GObjects.GetTypedPtr();
		if (!ObjArray)
			return false;

		const int32 Num = ObjArray->Num();
		for (int32 i = 0; i < Num; ++i)
		{
			if (ObjArray->GetByIndex(i) == Obj)
				return true;
		}
		return false;
	}

	bool EnsureLiveInternalWidgetForFrame()
	{
		if (!InternalWidget)
			return false;

		auto* Obj = static_cast<UObject*>(InternalWidget);
		if (Obj && IsPointerInLiveObjectArray(Obj) && UKismetSystemLibrary::IsValid(Obj))
			return true;

		std::cout << "[SDK] FrameHook: stale internal widget pointer detected, reset state\n";
		InternalWidget = nullptr;
		InternalWidgetVisible = false;
		GCachedBtnExit = nullptr;
		ClearRuntimeWidgetState();
		return false;
	}
}

void __fastcall HookedGVCPostRender(void* This, void* Canvas)
{
	PostRenderInFlightScope InFlightScope;

	// Call original first to preserve normal rendering
	if (OriginalGVCPostRender)
		OriginalGVCPostRender(This, Canvas);

	if (GIsUnloading.load(std::memory_order_relaxed))
	{
		if (!GUnloadCleanupDone.exchange(true, std::memory_order_acq_rel))
		{
			APlayerController* PC = GetFirstLocalPlayerController();
			DestroyInternalWidget(PC);
			std::cout << "[SDK] UnloadCleanup: runtime UI cleanup done on game thread\n";
		}
		return;
	}

	// World/level transition guard:
	// avoid DestroyInternalWidget/ShowInternalWidget in PostRender during unstable frames.
	static UWorld* LastWorld = nullptr;
	static ULevel* LastLevel = nullptr;
	static int32 TransitionGuardFrames = 0;
	UWorld* CurrentWorld = UWorld::GetWorld();
	ULevel* CurrentLevel = CurrentWorld ? CurrentWorld->PersistentLevel : nullptr;
	if (CurrentWorld != LastWorld || CurrentLevel != LastLevel)
	{
		if (LastWorld != nullptr)
		{
			const bool HadWidget = (InternalWidget != nullptr) || InternalWidgetVisible;
			std::cout << "[SDK] WorldTransition: oldWorld=" << (void*)LastWorld
				<< " newWorld=" << (void*)CurrentWorld
				<< " oldLevel=" << (void*)LastLevel
				<< " newLevel=" << (void*)CurrentLevel
				<< " hadWidget=" << (HadWidget ? 1 : 0)
				<< " -> invalidate runtime state only\n";

			InternalWidget = nullptr;
			InternalWidgetVisible = false;
			GCachedBtnExit = nullptr;
			ClearRuntimeWidgetState();
			TransitionGuardFrames = 120;
		}
		LastWorld = CurrentWorld;
		LastLevel = CurrentLevel;
	}
	const bool InTransitionGuard = (TransitionGuardFrames > 0);
	if (TransitionGuardFrames > 0)
		--TransitionGuardFrames;

	EnsureMouseCursorVisible();

	// Edge-trigger HOME so one press toggles once
	static bool HomeWasDown = false;
	const bool HomeDown = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
	if (!InTransitionGuard && HomeDown && !HomeWasDown)
		ToggleInternalWidget();
	HomeWasDown = HomeDown;

	if (InTransitionGuard)
		return;

	const bool HasLiveInternalWidget = EnsureLiveInternalWidgetForFrame();
	UUserWidget* LiveInternalWidget = HasLiveInternalWidget ? InternalWidget : nullptr;
	UBPMV_ConfigView2_C* LiveConfigView = nullptr;
	int32 ActiveNativeTabIndex = -1;
	if (InternalWidgetVisible &&
		LiveInternalWidget &&
		LiveInternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass()))
	{
		LiveConfigView = static_cast<UBPMV_ConfigView2_C*>(LiveInternalWidget);
		if (LiveConfigView->CT_Contents &&
			UKismetSystemLibrary::IsValid(static_cast<UObject*>(LiveConfigView->CT_Contents)))
		{
			ActiveNativeTabIndex = LiveConfigView->CT_Contents->GetActiveWidgetIndex();
		}
	}
	const bool IsItemsTabActive = (ActiveNativeTabIndex == 1);

	// Detect BTN_Exit click (edge-triggered).
	// Only poll while panel is visible and pointer is valid to avoid unreachable UObject asserts.
	static bool ExitWasPressed = false;
	bool ExitPressed = false;
	const bool CanCheckExit =
		InternalWidgetVisible &&
		LiveInternalWidget &&
		LiveInternalWidget->IsInViewport() &&
		LiveInternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass());

	if (CanCheckExit)
	{
		auto* CV = static_cast<UBPMV_ConfigView2_C*>(LiveInternalWidget);
		GCachedBtnExit = CV ? CV->BTN_Exit : nullptr;

		if (GCachedBtnExit)
		{
			auto* ExitObj = static_cast<UObject*>(GCachedBtnExit);
			if (UKismetSystemLibrary::IsValid(ExitObj))
				ExitPressed = GCachedBtnExit->IsPressed();
			else
				GCachedBtnExit = nullptr;
		}
	}
	else
	{
		GCachedBtnExit = nullptr;
		ExitWasPressed = false;
	}

	if (ExitWasPressed && !ExitPressed && CanCheckExit)
	{
		APlayerController* PC = GetFirstLocalPlayerController();
		if (PC)
			HideInternalWidget(PC);
	}
	ExitWasPressed = ExitPressed;

	// 鈹€鈹€ Item browser per-frame polling 鈹€鈹€
	static DWORD sLastItemUiPollTick = 0;
	if (InternalWidgetVisible && LiveInternalWidget && IsItemsTabActive)
	{
		const DWORD ItemUiNow = GetTickCount();
		const bool RunItemUiPoll = (sLastItemUiPollTick == 0) || ((ItemUiNow - sLastItemUiPollTick) >= 16);
		if (RunItemUiPoll)
		{
			sLastItemUiPollTick = ItemUiNow;
			PollCollapsiblePanelsInput();

			if (GVolumeLastValues.size() != GVolumeItems.size())
				GVolumeLastValues.resize(GVolumeItems.size(), 0.0f);
			if (GVolumeMinusWasPressed.size() != GVolumeItems.size())
				GVolumeMinusWasPressed.resize(GVolumeItems.size(), false);
			if (GVolumePlusWasPressed.size() != GVolumeItems.size())
				GVolumePlusWasPressed.resize(GVolumeItems.size(), false);

			for (size_t i = 0; i < GVolumeItems.size(); ++i)
			{
				auto* Item = GVolumeItems[i];
				if (!Item || !UKismetSystemLibrary::IsValid(static_cast<UObject*>(Item)))
					continue;

				auto* Slider = Item->VolumeSlider;
				if (!Slider || !UKismetSystemLibrary::IsValid(static_cast<UObject*>(Slider)))
					continue;

				bool MinusPressed = false;
				bool PlusPressed = false;
				if (Item->BTN_Minus && UKismetSystemLibrary::IsValid(static_cast<UObject*>(Item->BTN_Minus)))
					MinusPressed = Item->BTN_Minus->IsPressed();
				if (Item->BTN_Plus && UKismetSystemLibrary::IsValid(static_cast<UObject*>(Item->BTN_Plus)))
					PlusPressed = Item->BTN_Plus->IsPressed();

				const bool MinusClicked = GVolumeMinusWasPressed[i] && !MinusPressed;
				const bool PlusClicked = GVolumePlusWasPressed[i] && !PlusPressed;

				float CurValue = Slider->GetValue();
				bool bValueChanged = std::fabs(CurValue - GVolumeLastValues[i]) > 0.0001f;

				if (MinusClicked || PlusClicked)
				{
					float Step = Slider->StepSize;
					if (Step <= 0.0001f)
						Step = 0.01f;

					float NewValue = CurValue + (PlusClicked ? Step : 0.0f) - (MinusClicked ? Step : 0.0f);
					float MinValue = Slider->MinValue;
					float MaxValue = Slider->MaxValue;
					if (MaxValue < MinValue)
					{
						const float Tmp = MinValue;
						MinValue = MaxValue;
						MaxValue = Tmp;
					}

					if (NewValue < MinValue) NewValue = MinValue;
					if (NewValue > MaxValue) NewValue = MaxValue;

					if (std::fabs(NewValue - CurValue) > 0.0001f)
					{
						Slider->SetValue(NewValue);
						CurValue = NewValue;
						bValueChanged = true;
					}
				}

				if (bValueChanged)
				{
					CurValue = Slider->GetValue();
					if (Item->TXT_CurrentValue)
					{
						float MinValue = Slider->MinValue;
						float MaxValue = Slider->MaxValue;
						float Norm = CurValue;
						if (MaxValue > MinValue)
							Norm = (CurValue - MinValue) / (MaxValue - MinValue);
						if (Norm < 0.0f) Norm = 0.0f;
						if (Norm > 1.0f) Norm = 1.0f;
						const int32 DisplayValue = static_cast<int32>(Norm * 100.0f + 0.5f);
						wchar_t Buf[16] = {};
						swprintf_s(Buf, 16, L"%d", DisplayValue);
						Item->TXT_CurrentValue->SetText(MakeText(Buf));
					}
				}

				GVolumeLastValues[i] = CurValue;
				GVolumeMinusWasPressed[i] = MinusPressed;
				GVolumePlusWasPressed[i] = PlusPressed;
			}

			static DWORD LastItemCacheRetryTick = 0;
			if (!GItemCacheBuilt && (GItemCategoryDD || GItemGridPanel))
			{
				DWORD NowTick = GetTickCount();
				if (NowTick - LastItemCacheRetryTick > 1000)
				{
					LastItemCacheRetryTick = NowTick;
					BuildItemCache();
					if (GItemCacheBuilt)
					{
						int32 CurrentCat = 0;
						if (GItemCategoryDD && GItemCategoryDD->CB_Main)
						{
							int32 SelectedCat = GItemCategoryDD->CB_Main->GetSelectedIndex();
							if (SelectedCat >= 0)
								CurrentCat = SelectedCat;
						}
						GItemCurrentPage = 0;
						GItemLastCatIdx = CurrentCat;
						FilterItems(CurrentCat);
						RefreshItemPage();
					}
				}
			}

			if (GItemCategoryDD && GItemCategoryDD->CB_Main)
			{
				int32 catIdx = GItemCategoryDD->CB_Main->GetSelectedIndex();
				if (catIdx != GItemLastCatIdx && catIdx >= 0)
				{
					GItemLastCatIdx = catIdx;
					GItemCurrentPage = 0;
					FilterItems(catIdx);
					RefreshItemPage();
				}
			}

			GItemAddQuantity = GetItemAddQuantityFromEdit();

			auto GetClickableButton = [](UJHCommon_Btn_Free_C* W) -> UButton* {
				if (!W) return nullptr;
				if (W->Btn) return W->Btn;
				if (W->JHGPCBtn)
					return static_cast<UJHNeoUIGamepadConfirmButton*>(W->JHGPCBtn)->BtnMain;
				return nullptr;
			};

			UButton* PrevInner = GetClickableButton(GItemPrevPageBtn);
			bool PrevPressed = PrevInner &&
				UKismetSystemLibrary::IsValid(static_cast<UObject*>(PrevInner)) &&
				PrevInner->IsPressed();
			if (GItemPrevWasPressed && !PrevPressed && GItemCurrentPage > 0)
			{
				GItemCurrentPage--;
				RefreshItemPage();
			}
			GItemPrevWasPressed = PrevPressed;

			UButton* NextInner = GetClickableButton(GItemNextPageBtn);
			bool NextPressed = NextInner &&
				UKismetSystemLibrary::IsValid(static_cast<UObject*>(NextInner)) &&
				NextInner->IsPressed();
			if (GItemNextWasPressed && !NextPressed && (GItemCurrentPage + 1) < GItemTotalPages)
			{
				GItemCurrentPage++;
				RefreshItemPage();
			}
			GItemNextWasPressed = NextPressed;

			for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
			{
				auto* Btn = GItemSlotButtons[i];
				if (!Btn || !UKismetSystemLibrary::IsValid(static_cast<UObject*>(Btn)))
					continue;

				bool Pressed = Btn->IsPressed();
				if (GItemSlotWasPressed[i] && !Pressed)
				{
					int32 itemIdx = GItemSlotItemIndices[i];
					if (itemIdx >= 0 && itemIdx < (int32)GAllItems.size())
					{
						CachedItem& item = GAllItems[itemIdx];
						UItemFuncLib::AddItem(item.DefId, GItemAddQuantity);
						std::cout << "[SDK] AddItem(click): " << item.DefId << " x" << GItemAddQuantity << "\n";
					}
				}
				GItemSlotWasPressed[i] = Pressed;
			}
		}
	}
	else
	{
		sLastItemUiPollTick = 0;
		GItemPrevWasPressed = false;
		GItemNextWasPressed = false;
		for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
			GItemSlotWasPressed[i] = false;
	}

	// Hover tips polling:
	// 1) 物品 Tab 内高频轮询（节流到约 60Hz）
	// 2) 非物品 Tab 仅在已有 tips 残留时低频轮询，用于快速收口隐藏
	bool HoverGridValid = false;
	if (GItemGridPanel)
	{
		auto* GridObj = static_cast<UObject*>(GItemGridPanel);
		HoverGridValid = IsPointerInLiveObjectArray(GridObj) && UKismetSystemLibrary::IsValid(GridObj);
	}
	const bool HasActiveHoverTips =
		(GItemHoveredSlot >= 0) ||
		(GItemHoverTipsWidget && UKismetSystemLibrary::IsValid(static_cast<UObject*>(GItemHoverTipsWidget)));
	const bool CanPollHover =
		InternalWidgetVisible &&
		LiveInternalWidget &&
		HoverGridValid &&
		(IsItemsTabActive || HasActiveHoverTips);

	if (CanPollHover)
	{
		static DWORD sLastHoverPollTick = 0;
		const DWORD HoverPollNow = GetTickCount();
		const DWORD PollIntervalMs = IsItemsTabActive ? (HasActiveHoverTips ? 33 : 20) : 80;
		if (sLastHoverPollTick == 0 || (HoverPollNow - sLastHoverPollTick) >= PollIntervalMs)
		{
			sLastHoverPollTick = HoverPollNow;
			// 临时禁用：用于验证物品 Tab 悬浮 Tip 轮询是否为卡顿主因
			// PollItemBrowserHoverTips();
		}
	}

	// 鈹€鈹€ Dynamic tab button click detection 鈹€鈹€
	if (InternalWidgetVisible && LiveConfigView)
	{
		auto* CV = LiveConfigView;
		auto IsLiveTabBtn = [](UBP_JHConfigTabBtn_C* Btn) -> bool
		{
			return Btn && UKismetSystemLibrary::IsValid(static_cast<UObject*>(Btn));
		};

		// Native tabs fully handle themselves via AutoFocusForMouseEntering 鈫?
		// HandleMainBtn() 鈫?EVT_SyncTabIndex(). We only manage dynamic tab
		// content visibility (VBoxes outside the Switcher) and active state.
		static int32 sActiveDynTab = -1;

		int32 dynHoverIdx = -1;
		if      (IsLiveTabBtn(GDynTabBtn6) && GDynTabBtn6->IsHovered()) dynHoverIdx = 6;
		else if (IsLiveTabBtn(GDynTabBtn7) && GDynTabBtn7->IsHovered()) dynHoverIdx = 7;
		else if (IsLiveTabBtn(GDynTabBtn8) && GDynTabBtn8->IsHovered()) dynHoverIdx = 8;

		if (dynHoverIdx >= 6 && dynHoverIdx != sActiveDynTab)
		{
			// Entering a (different) dynamic tab 鈥?show its content
			ShowDynamicTab(CV, dynHoverIdx);
			if (IsLiveTabBtn(GDynTabBtn6)) GDynTabBtn6->EVT_UpdateActiveStatus(dynHoverIdx == 6);
			if (IsLiveTabBtn(GDynTabBtn7)) GDynTabBtn7->EVT_UpdateActiveStatus(dynHoverIdx == 7);
			if (IsLiveTabBtn(GDynTabBtn8)) GDynTabBtn8->EVT_UpdateActiveStatus(dynHoverIdx == 8);
			sActiveDynTab = dynHoverIdx;
		}
		else if (sActiveDynTab >= 6 && dynHoverIdx == -1)
		{
			// Only restore original content when a NATIVE tab is hovered.
			// Moving mouse to content area or empty space keeps dynamic tab active
			// 鈥?same behavior as native tabs.
			bool nativeHovered =
				(CV->BTN_Sound && UKismetSystemLibrary::IsValid(static_cast<UObject*>(CV->BTN_Sound)) && CV->BTN_Sound->IsHovered()) ||
				(CV->BTN_Video && UKismetSystemLibrary::IsValid(static_cast<UObject*>(CV->BTN_Video)) && CV->BTN_Video->IsHovered()) ||
				(CV->BTN_Keys && UKismetSystemLibrary::IsValid(static_cast<UObject*>(CV->BTN_Keys)) && CV->BTN_Keys->IsHovered()) ||
				(CV->BTN_Lan && UKismetSystemLibrary::IsValid(static_cast<UObject*>(CV->BTN_Lan)) && CV->BTN_Lan->IsHovered()) ||
				(CV->BTN_Others && UKismetSystemLibrary::IsValid(static_cast<UObject*>(CV->BTN_Others)) && CV->BTN_Others->IsHovered()) ||
				(CV->BTN_Gamepad && UKismetSystemLibrary::IsValid(static_cast<UObject*>(CV->BTN_Gamepad)) && CV->BTN_Gamepad->IsHovered());
			if (nativeHovered)
			{
				ShowOriginalTab(CV);
				if (IsLiveTabBtn(GDynTabBtn6)) GDynTabBtn6->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTabBtn7)) GDynTabBtn7->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTabBtn8)) GDynTabBtn8->EVT_UpdateActiveStatus(false);
				sActiveDynTab = -1;
			}
		}
	}

	// Detect if blueprint logic closed the widget externally
	if (InternalWidgetVisible && LiveInternalWidget && !LiveInternalWidget->IsInViewport())
	{
		APlayerController* PC = GetFirstLocalPlayerController();
		HideInternalWidget(PC);
		std::cout << "[SDK] Widget closed externally, cached instance kept\n";
	}
}

// -- Main Thread --
