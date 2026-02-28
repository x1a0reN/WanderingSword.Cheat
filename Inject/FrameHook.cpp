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

		bool PrevPressed = GItemPrevPageBtn && GItemPrevPageBtn->IsPressed();
		if (GItemPrevWasPressed && !PrevPressed && GItemCurrentPage > 0)
		{
			GItemCurrentPage--;
			RefreshItemPage();
		}
		GItemPrevWasPressed = PrevPressed;

		bool NextPressed = GItemNextPageBtn && GItemNextPageBtn->IsPressed();
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
		auto* Switcher = CV->CT_Contents;

		if (Switcher)
		{
			auto DeactivateOriginalTabs = [&]()
			{
				if (CV->BTN_Sound)   CV->BTN_Sound->EVT_UpdateActiveStatus(false);
				if (CV->BTN_Video)   CV->BTN_Video->EVT_UpdateActiveStatus(false);
				if (CV->BTN_Keys)    CV->BTN_Keys->EVT_UpdateActiveStatus(false);
				if (CV->BTN_Lan)     CV->BTN_Lan->EVT_UpdateActiveStatus(false);
				if (CV->BTN_Others)  CV->BTN_Others->EVT_UpdateActiveStatus(false);
				if (CV->BTN_Gamepad) CV->BTN_Gamepad->EVT_UpdateActiveStatus(false);
			};
			auto SetDynamicActive = [&](int32 ActiveIdx)
			{
				auto SafeSetActive = [](UBP_JHConfigTabBtn_C* Btn, bool Active) {
					if (!Btn) return;
					if (Btn->JHGPCBtn_ActiveBG)
						Btn->JHGPCBtn_ActiveBG->SetVisibility(
							Active ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
				};
				SafeSetActive(GDynTabBtn6, ActiveIdx == 6);
				SafeSetActive(GDynTabBtn7, ActiveIdx == 7);
				SafeSetActive(GDynTabBtn8, ActiveIdx == 8);
			};
			// Detect if user clicked an original tab (0-5) via switcher index change
			int32 curIdx = Switcher->GetActiveWidgetIndex();
			if (curIdx != GLastSwitcherIndex && curIdx >= 0 && curIdx <= 5)
			{
				ShowOriginalTab(CV);
				SetDynamicActive(-1);
			}
			GLastSwitcherIndex = curIdx;

			// Dynamic tab click detection via IsHovered() + Win32 mouse state.
			// BtnMain->IsPressed() is broken because SanitizeWidgetTree clears
			// button bindings and sets IsFocusable=false, preventing Slate press
			// state. IsHovered() is geometry-based and unaffected by sanitization.
			static bool sDynMouseWasDown = false;
			bool DynMouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			bool DynMouseReleased = sDynMouseWasDown && !DynMouseDown;

			if (DynMouseReleased)
			{
				if (GDynTabBtn6 && GDynTabBtn6->IsHovered())
				{
					ShowDynamicTab(CV, 6);
					DeactivateOriginalTabs();
					SetDynamicActive(6);
					std::cout << "[SDK] Switched to Tab6 (Teammates)\n";
				}
				else if (GDynTabBtn7 && GDynTabBtn7->IsHovered())
				{
					ShowDynamicTab(CV, 7);
					DeactivateOriginalTabs();
					SetDynamicActive(7);
					std::cout << "[SDK] Switched to Tab7 (Quests)\n";
				}
				else if (GDynTabBtn8 && GDynTabBtn8->IsHovered())
				{
					ShowDynamicTab(CV, 8);
					DeactivateOriginalTabs();
					SetDynamicActive(8);
					std::cout << "[SDK] Switched to Tab8 (Controls)\n";
				}
			}
			sDynMouseWasDown = DynMouseDown;
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
