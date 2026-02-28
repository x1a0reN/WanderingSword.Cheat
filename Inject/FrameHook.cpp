#include <Windows.h>
#include <iostream>

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

	// Detect BTN_Exit click (edge-triggered)
	static bool ExitWasPressed = false;
	bool ExitPressed = GCachedBtnExit && GCachedBtnExit->IsPressed();
	if (ExitWasPressed && !ExitPressed && InternalWidgetVisible)
	{
		APlayerController* PC = GetFirstLocalPlayerController();
		if (PC)
			HideInternalWidget(PC);
	}
	ExitWasPressed = ExitPressed;

		// ── Item browser per-frame polling ──
	if (InternalWidgetVisible)
	{
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

		auto GetInnerButton = [](UJHCommon_Btn_Free_C* Btn) -> UButton* {
			if (!Btn || !Btn->JHGPCBtn) return nullptr;
			return static_cast<UJHNeoUIGamepadConfirmButton*>(Btn->JHGPCBtn)->BtnMain;
		};

		UButton* PrevInner = GetInnerButton(GItemPrevPageBtn);
		bool PrevPressed = PrevInner && PrevInner->IsPressed();
		if (GItemPrevWasPressed && !PrevPressed && GItemCurrentPage > 0)
		{
			GItemCurrentPage--;
			RefreshItemPage();
		}
		GItemPrevWasPressed = PrevPressed;

		UButton* NextInner = GetInnerButton(GItemNextPageBtn);
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
