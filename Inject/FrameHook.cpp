#include <Windows.h>
#include <iostream>
#include <cmath>

#include "CheatState.hpp"
#include "FrameHook.hpp"
#include "ItemBrowser.hpp"
#include "PanelManager.hpp"
#include "WidgetUtils.hpp"

void __fastcall HookedGVCPostRender(void* This, void* Canvas)
{
	// Call original first to preserve normal rendering
	if (OriginalGVCPostRender)
		OriginalGVCPostRender(This, Canvas);

	if (GIsUnloading.load(std::memory_order_relaxed))
		return;

	EnsureMouseCursorVisible();

	// Edge-trigger HOME so one press toggles once
	static bool HomeWasDown = false;
	const bool HomeDown = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
	if (HomeDown && !HomeWasDown)
		ToggleInternalWidget();
	HomeWasDown = HomeDown;

	// Detect BTN_Exit click (edge-triggered).
	// Only poll while panel is visible and pointer is valid to avoid unreachable UObject asserts.
	static bool ExitWasPressed = false;
	bool ExitPressed = false;
	const bool CanCheckExit =
		InternalWidgetVisible &&
		InternalWidget &&
		InternalWidget->IsInViewport() &&
		InternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass());

	if (CanCheckExit)
	{
		auto* CV = static_cast<UBPMV_ConfigView2_C*>(InternalWidget);
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

	// ── Item browser per-frame polling ──
	if (InternalWidgetVisible)
	{
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
		bool PrevPressed = PrevInner && PrevInner->IsPressed();
		if (GItemPrevWasPressed && !PrevPressed && GItemCurrentPage > 0)
		{
			GItemCurrentPage--;
			RefreshItemPage();
		}
		GItemPrevWasPressed = PrevPressed;

		UButton* NextInner = GetClickableButton(GItemNextPageBtn);
		bool NextPressed = NextInner && NextInner->IsPressed();
		if (GItemNextWasPressed && !NextPressed && (GItemCurrentPage + 1) < GItemTotalPages)
		{
			GItemCurrentPage++;
			RefreshItemPage();
		}
		GItemNextWasPressed = NextPressed;

		for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
		{
			auto* Btn = GItemSlotButtons[i];
			if (!Btn)
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

	// ── Dynamic tab button click detection ──
	if (InternalWidgetVisible && InternalWidget && InternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass()))
	{
		auto* CV = static_cast<UBPMV_ConfigView2_C*>(InternalWidget);

		// Native tabs fully handle themselves via AutoFocusForMouseEntering →
		// HandleMainBtn() → EVT_SyncTabIndex(). We only manage dynamic tab
		// content visibility (VBoxes outside the Switcher) and active state.
		static int32 sActiveDynTab = -1;

		int32 dynHoverIdx = -1;
		if      (GDynTabBtn6 && GDynTabBtn6->IsHovered()) dynHoverIdx = 6;
		else if (GDynTabBtn7 && GDynTabBtn7->IsHovered()) dynHoverIdx = 7;
		else if (GDynTabBtn8 && GDynTabBtn8->IsHovered()) dynHoverIdx = 8;

		if (dynHoverIdx >= 6 && dynHoverIdx != sActiveDynTab)
		{
			// Entering a (different) dynamic tab — show its content
			ShowDynamicTab(CV, dynHoverIdx);
			if (GDynTabBtn6) GDynTabBtn6->EVT_UpdateActiveStatus(dynHoverIdx == 6);
			if (GDynTabBtn7) GDynTabBtn7->EVT_UpdateActiveStatus(dynHoverIdx == 7);
			if (GDynTabBtn8) GDynTabBtn8->EVT_UpdateActiveStatus(dynHoverIdx == 8);
			sActiveDynTab = dynHoverIdx;
		}
		else if (sActiveDynTab >= 6 && dynHoverIdx == -1)
		{
			// Only restore original content when a NATIVE tab is hovered.
			// Moving mouse to content area or empty space keeps dynamic tab active
			// — same behavior as native tabs.
			bool nativeHovered = (CV->BTN_Sound   && CV->BTN_Sound->IsHovered())
			                  || (CV->BTN_Video   && CV->BTN_Video->IsHovered())
			                  || (CV->BTN_Keys    && CV->BTN_Keys->IsHovered())
			                  || (CV->BTN_Lan     && CV->BTN_Lan->IsHovered())
			                  || (CV->BTN_Others  && CV->BTN_Others->IsHovered())
			                  || (CV->BTN_Gamepad && CV->BTN_Gamepad->IsHovered());
			if (nativeHovered)
			{
				ShowOriginalTab(CV);
				if (GDynTabBtn6) GDynTabBtn6->EVT_UpdateActiveStatus(false);
				if (GDynTabBtn7) GDynTabBtn7->EVT_UpdateActiveStatus(false);
				if (GDynTabBtn8) GDynTabBtn8->EVT_UpdateActiveStatus(false);
				sActiveDynTab = -1;
			}
		}
	}

	// Detect if blueprint logic closed the widget externally
	if (InternalWidgetVisible && InternalWidget && !InternalWidget->IsInViewport())
	{
		APlayerController* PC = GetFirstLocalPlayerController();
		HideInternalWidget(PC);
		std::cout << "[SDK] Widget closed externally, cached instance kept\n";
	}

	// Throttle output to once per second to avoid flooding the console
	static DWORD LastPrint = 0;
	DWORD Now = GetTickCount();
	if (Now - LastPrint > 1000)
	{
		std::cout << "[SDK] GVC PostRender called\n";
		LastPrint = Now;
	}
}

// -- Main Thread --
