#pragma once

#include <Windows.h>
#include <atomic>
#include <string>
#include <unordered_map>
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

using GVCPostRenderFn = void(__fastcall*)(void* , void* );

inline constexpr float kInternalPanelScale = 1.1f;

inline constexpr uintptr_t kConfigTabBtnParentCtxOffset = 0x02E0;

inline constexpr uintptr_t kConfigModuleView2ParentCtxOffset = 0x03B0;

struct CachedItem {
	int32 DefId;
	wchar_t Name[64];
	wchar_t Desc[256];
	uint8 Quality;
	uint8 SubType;
	uint8 IconData[0x28];
	bool HasIcon;
};

struct UIRememberState
{
	std::unordered_map<std::wstring, int32> ComboIndexByTitle;
	std::unordered_map<std::wstring, float> SliderValueByTitle;
	std::unordered_map<std::wstring, std::wstring> EditTextByTitle;

	int32 ItemCategoryIndex = 0;
	int32 ItemCurrentPage = 0;
	int32 ItemAddQuantity = 1;
	std::wstring ItemSearchText;
};

inline constexpr int32 kItemGridCols = 8;
inline constexpr int32 kItemGridRows = 5;
inline constexpr int32 kItemsPerPage = kItemGridCols * kItemGridRows;


extern GVCPostRenderFn GOriginalPostRender;
extern VTableHook GVCPostRenderHook;
extern UUserWidget* GInternalWidget;
extern bool GInternalWidgetVisible;
extern std::atomic<bool> GIsUnloading;
extern std::atomic<bool> GUnloadCleanupDone;
extern std::atomic<bool> GHooksRemoved;
extern std::atomic<bool> GConsoleClosed;
extern std::atomic<int32> GPostRenderInFlight;
extern UIRememberState GUIRememberState;


extern UNeoUIButtonBase* GCachedBtnExit;
extern std::vector<UObject*> GRootedObjects;
extern UWidget* GOriginalLanPanel;
extern UWidget* GOriginalInputMappingPanel;
extern UJHCommon_Btn_Free_C* GOriginalResetButton;


struct ItemBrowserState {
    std::vector<CachedItem> AllItems;
    std::vector<int32> FilteredIndices;
    bool CacheBuilt = false;
    int32 CurrentPage = 0;
    int32 TotalPages = 0;

    UBPVE_JHConfigVideoItem2_C* CategoryDD = nullptr;
    int32 LastCatIdx = -1;
    UHorizontalBox* PagerRow = nullptr;
    UJHCommon_Btn_Free_C* PrevPageBtn = nullptr;
    UJHCommon_Btn_Free_C* NextPageBtn = nullptr;
    UTextBlock* PageLabel = nullptr;
    bool PrevWasPressed = false;
    bool NextWasPressed = false;
    UHorizontalBox* QuantityRow = nullptr;
    UEditableTextBox* QuantityEdit = nullptr;
    int32 AddQuantity = 1;
    UWidget* GridPanel = nullptr;
    UListView* ListView = nullptr;
    UButton* SlotButtons[kItemsPerPage] = {};
    UImage* SlotImages[kItemsPerPage] = {};
    UImage* SlotQualityBorders[kItemsPerPage] = {};
    UUserWidget* SlotEntryWidgets[kItemsPerPage] = {};
    int32 SlotItemIndices[kItemsPerPage] = {};
    bool SlotWasPressed[kItemsPerPage] = {};
    int32 HoveredSlot = -1;
    UJHNeoUITipsVEBase* HoverTipsWidget = nullptr;
    UItemInfoSpec* HoverTempSpec = nullptr;
};

extern ItemBrowserState GItemBrowser;


extern std::vector<UBPVE_JHConfigVolumeItem2_C*> GVolumeItems;
extern std::vector<float> GVolumeLastValues;
extern std::vector<bool> GVolumeMinusWasPressed;
extern std::vector<bool> GVolumePlusWasPressed;


struct Tab1Controls {
    UBPVE_JHConfigVideoItem2_C* ItemNoDecreaseToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* ItemGainMultiplierToggle = nullptr;
    UBPVE_JHConfigVolumeItem2_C* ItemGainMultiplierSlider = nullptr;
    UBPVE_JHConfigVideoItem2_C* AllItemsSellableToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* DropRate100Toggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* CraftEffectMultiplierToggle = nullptr;
    UBPVE_JHConfigVolumeItem2_C* CraftItemIncrementSlider = nullptr;
    UBPVE_JHConfigVolumeItem2_C* CraftExtraEffectSlider = nullptr;
    UBPVE_JHConfigVideoItem2_C* MaxExtraAffixesToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* IgnoreItemUseCountToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* IgnoreItemRequirementsToggle = nullptr;
};

extern Tab1Controls GTab1;


struct Tab2Controls {
    UBPVE_JHConfigVideoItem2_C* DamageBoostToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* SkillNoCooldownToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* NoEncounterToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* AllTeammatesInFightToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* DefeatAsVictoryToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* NeiGongFillLastSlotToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* AutoRecoverHpMpToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* TotalMoveSpeedToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* DamageFriendlyOnlyToggle = nullptr;
    UBPVE_JHConfigVolumeItem2_C* DamageMultiplierSlider = nullptr;
    UBPVE_JHConfigVolumeItem2_C* MoveSpeedMultiplierSlider = nullptr;
};

extern Tab2Controls GTab2;

struct DynTabState {
    UBP_JHConfigTabBtn_C* Btn6 = nullptr;
    UBP_JHConfigTabBtn_C* Btn7 = nullptr;
    UBP_JHConfigTabBtn_C* Btn8 = nullptr;
    UVerticalBox* Content6 = nullptr;
    UVerticalBox* Content7 = nullptr;
    UVerticalBox* Content8 = nullptr;
};

extern DynTabState GDynTab;


struct TeammateTabControls {
    UBPVE_JHConfigVideoItem2_C* FollowToggle = nullptr;
    UBPVE_JHConfigVolumeItem2_C* FollowCount = nullptr;
    UBPVE_JHConfigVideoItem2_C* AddDD = nullptr;
    UBPVE_JHConfigVideoItem2_C* ReplaceToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* ReplaceDD = nullptr;
};

extern TeammateTabControls GTeammate;


struct QuestTabControls {
    UBPVE_JHConfigVideoItem2_C* Toggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* TypeDD = nullptr;
};

extern QuestTabControls GQuest;

extern std::atomic<bool> GItemNoDecreaseEnabled;

extern uintptr_t GChangeItemNumAddr;
extern unsigned char GOriginalChangeItemNumBytes[5];
extern void* GHookTrampoline;
extern bool GInlineHookInstalled;

void EnableItemNoDecreaseHook();
void DisableItemNoDecreaseHook();

void SetItemGainMultiplierHookValue(int32 Value);
void EnableItemGainMultiplierHook();
void DisableItemGainMultiplierHook();

void SetCraftItemIncrementHookValue(float Value);
void SetCraftExtraEffectHookValue(float Value);
void EnableCraftEffectMultiplierHook();
void DisableCraftEffectMultiplierHook();

void EnableMaxExtraAffixesHooks();
void DisableMaxExtraAffixesHooks();

void EnableAllItemsSellable();
void DisableAllItemsSellable();
void EnableDropRate100Patch();
void DisableDropRate100Patch();
void EnableIgnoreItemUseCountFeature();
void DisableIgnoreItemUseCountFeature();
void EnableIgnoreItemRequirementsPatch();
void DisableIgnoreItemRequirementsPatch();

void EnableSkillNoCooldownHooks();
void DisableSkillNoCooldownHooks();

void EnableNoEncounterPatch();
void DisableNoEncounterPatch();

void EnableDefeatAsVictoryHook();
void DisableDefeatAsVictoryHook();

void EnableAllTeammatesInFightHooks();
void DisableAllTeammatesInFightHooks();

void EnableNeiGongFillLastSlotFeature();
void DisableNeiGongFillLastSlotFeature();

void EnableBattleSpeedHooks();
void DisableBattleSpeedHooks();
void SetBattleSpeedHookMultiplier(float Value);

void EnableAutoRecoverHpMpHook();
void DisableAutoRecoverHpMpHook();

void EnableTotalMoveSpeedHook();
void DisableTotalMoveSpeedHook();
void SetTotalMoveSpeedMultiplier(float Value);
void SetTotalMoveSpeedFriendlyOnly(bool Enabled);
