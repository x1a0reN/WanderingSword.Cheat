#pragma once

#include <Windows.h>
#include <atomic>
#include <vector>

#include "SDK/Engine_classes.hpp"
#include "SDK/UMG_classes.hpp"
#include "SDK/NeoUI_classes.hpp"
#include "SDK/BPMV_ConfigView2_classes.hpp"
#include "SDK/BP_JHConfigTabBtn_classes.hpp"
#include "SDK/JHGPCBtn_classes.hpp"
#include "SDK/JHGPCBtn_ActiveBG_classes.hpp"
#include "SDK/JHCommon_Btn_Free_classes.hpp"
#include "SDK/JH_classes.hpp"
#include "SDK/BPVE_JHConfigVideoItem2_classes.hpp"
#include "SDK/BPVE_JHConfigVolumeItem2_classes.hpp"
#include "VTHook.hpp"

using namespace SDK;

using GVCPostRenderFn = void(__fastcall*)(void* /* this */, void* /* Canvas */);

inline constexpr float kInternalPanelScale = 1.1f;
inline constexpr bool kTempMoveControlsShowcaseToTab1 = true;
inline constexpr uintptr_t kConfigTabBtnParentCtxOffset = 0x02E0;
inline constexpr uintptr_t kConfigModuleView2ParentCtxOffset = 0x03B0;

struct CachedItem {
	int32 DefId;
	wchar_t Name[64];
	uint8 Quality;
	uint8 SubType;
	uint8 IconData[0x28];
	bool HasIcon;
};

inline constexpr int32 ITEM_GRID_COLS = 6;
inline constexpr int32 ITEM_GRID_ROWS = 4;
inline constexpr int32 ITEMS_PER_PAGE = ITEM_GRID_COLS * ITEM_GRID_ROWS;

extern GVCPostRenderFn OriginalGVCPostRender;
extern VTableHook GVCPostRenderHook;
extern UUserWidget* InternalWidget;
extern bool InternalWidgetVisible;
extern std::atomic<bool> GIsUnloading;
extern std::atomic<bool> GHooksRemoved;
extern std::atomic<bool> GConsoleClosed;

extern UButton* GCachedBtnExit;
extern std::vector<UObject*> GRootedObjects;
extern UWidget* GOriginalLanPanel;
extern UWidget* GOriginalInputMappingPanel;
extern UJHCommon_Btn_Free_C* GOriginalResetButton;
extern UJHCommon_Btn_Free_C* GNativeGameResetButton;

extern std::vector<CachedItem> GAllItems;
extern std::vector<int32> GFilteredIndices;
extern bool GItemCacheBuilt;
extern int32 GItemCurrentPage;
extern int32 GItemTotalPages;

extern UBPVE_JHConfigVideoItem2_C* GItemCategoryDD;
extern int32 GItemLastCatIdx;
extern UHorizontalBox* GItemPagerRow;
extern UButton* GItemPrevPageBtn;
extern UButton* GItemNextPageBtn;
extern UTextBlock* GItemPageLabel;
extern bool GItemPrevWasPressed;
extern bool GItemNextWasPressed;
extern UHorizontalBox* GItemQuantityRow;
extern UEditableTextBox* GItemQuantityEdit;
extern int32 GItemAddQuantity;
extern UUniformGridPanel* GItemGridPanel;
extern UButton* GItemSlotButtons[ITEMS_PER_PAGE];
extern UImage* GItemSlotImages[ITEMS_PER_PAGE];
extern int32 GItemSlotItemIndices[ITEMS_PER_PAGE];
extern bool GItemSlotWasPressed[ITEMS_PER_PAGE];

extern UBP_JHConfigTabBtn_C* GDynTabBtn6;
extern UBP_JHConfigTabBtn_C* GDynTabBtn7;
extern UBP_JHConfigTabBtn_C* GDynTabBtn8;
extern UVerticalBox* GDynTabContent6;
extern UVerticalBox* GDynTabContent7;
extern UVerticalBox* GDynTabContent8;

extern int32 GLastSwitcherIndex;

extern UBPVE_JHConfigVideoItem2_C* GTeammateFollowToggle;
extern UBPVE_JHConfigVolumeItem2_C* GTeammateFollowCount;
extern UBPVE_JHConfigVideoItem2_C* GTeammateAddDD;
extern UBPVE_JHConfigVideoItem2_C* GTeammateReplaceToggle;
extern UBPVE_JHConfigVideoItem2_C* GTeammateReplaceDD;

extern UBPVE_JHConfigVideoItem2_C* GQuestToggle;
extern UBPVE_JHConfigVideoItem2_C* GQuestTypeDD;
