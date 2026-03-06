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

/// PostRender Hook 鍘熷鍑芥暟绛惧悕
using GVCPostRenderFn = void(__fastcall*)(void* /* this */, void* /* Canvas */);

/// 闈㈡澘缂╂斁姣斾緥 (1.1x 鏁翠綋鏀惧ぇ)
inline constexpr float kInternalPanelScale = 1.1f;

/// UBP_JHConfigTabBtn_C 鍐呴儴鎸囧悜鐖剁骇 ConfigView 鐨勫亸绉?(鐢ㄤ簬 PatchTabBtnRuntimeContext)
inline constexpr uintptr_t kConfigTabBtnParentCtxOffset = 0x02E0;

/// 瀛愭ā鍧楅潰鏉?(濡?VideoPanel2) 鍐呴儴鎸囧悜鐖剁骇 ConfigView 鐨勫亸绉?
inline constexpr uintptr_t kConfigModuleView2ParentCtxOffset = 0x03B0;

/// 鐗╁搧缂撳瓨鏉＄洰: 浠?DataTable 瑙ｆ瀽鐨勭墿鍝佷俊鎭?
struct CachedItem {
	int32 DefId;            // 鐗╁搧瀹氫箟 ID
	wchar_t Name[64];       // 鐗╁搧鍚嶇О
	wchar_t Desc[256];      // 鐗╁搧鎻忚堪
	uint8 Quality;          // 鍝佽川绛夌骇
	uint8 SubType;          // 瀛愮被鍨?(鐢ㄤ簬鍒嗙被杩囨护)
	uint8 IconData[0x28];   // TSoftObjectPtr<UTexture2D> 鍘熷鏁版嵁 (40瀛楄妭)
	bool HasIcon;           // 鏄惁鏈夋湁鏁堝浘鏍囨暟鎹?
};

/// 面板重建时用于恢复 UI 状态的进程内缓存（不落盘）。
struct UIRememberState
{
	// 统一按“控件标题”记忆状态
	std::unordered_map<std::wstring, int32> ComboIndexByTitle;
	std::unordered_map<std::wstring, float> SliderValueByTitle;
	std::unordered_map<std::wstring, std::wstring> EditTextByTitle;

	// 物品浏览器附加状态
	int32 ItemCategoryIndex = 0;
	int32 ItemCurrentPage = 0;
	int32 ItemAddQuantity = 1;
	std::wstring ItemSearchText;
};

/// 鐗╁搧缃戞牸甯冨眬甯搁噺
inline constexpr int32 kItemGridCols = 8;
inline constexpr int32 kItemGridRows = 5;
inline constexpr int32 kItemsPerPage = kItemGridCols * kItemGridRows; // 40

// 鈹€鈹€ Hook 涓庢牳蹇冪姸鎬?鈹€鈹€

extern GVCPostRenderFn GOriginalPostRender;   // PostRender 鍘熷鍑芥暟鎸囬拡
extern VTableHook GVCPostRenderHook;             // VTable Hook 瀹炰緥
extern UUserWidget* GInternalWidget;              // 浣滃紛闈㈡澘 Widget 瀹炰緥
extern bool GInternalWidgetVisible;               // 闈㈡澘鍙鎬ф爣璁?
extern std::atomic<bool> GIsUnloading;           // DLL 姝ｅ湪鍗歌浇
extern std::atomic<bool> GUnloadCleanupDone;     // 卸载清理是否已在游戏线程完成
extern std::atomic<bool> GHooksRemoved;          // Hook 宸茬Щ闄?
extern std::atomic<bool> GConsoleClosed;         // 鎺у埗鍙板凡鍏抽棴
extern std::atomic<int32> GPostRenderInFlight;   // HookedGVCPostRender 当前在执行的调用数
extern UIRememberState GUIRememberState;          // 面板重建状态记忆

// 鈹€鈹€ 闈㈡澘 UI 寮曠敤 鈹€鈹€

extern UNeoUIButtonBase* GCachedBtnExit;         // 缂撳瓨鐨勫叧闂寜閽?(姣忓抚妫€娴嬬偣鍑?
extern std::vector<UObject*> GRootedObjects;     // 宸叉爣璁?GC Root 鐨勫璞″垪琛?
extern UWidget* GOriginalLanPanel;               // 淇濈暀鐨勫師鐢熻瑷€闈㈡澘
extern UWidget* GOriginalInputMappingPanel;      // 淇濈暀鐨勫師鐢熼敭浣嶉潰鏉?
extern UJHCommon_Btn_Free_C* GOriginalResetButton; // 淇濈暀鐨勫師鐢熼噸缃寜閽?

// 鈹€鈹€ 鐗╁搧娴忚鍣ㄧ姸鎬?鈹€鈹€

// ── 物品浏览器状态 ──

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

// ── 滑块控件缓存（跨Tab通用）──

extern std::vector<UBPVE_JHConfigVolumeItem2_C*> GVolumeItems;
extern std::vector<float> GVolumeLastValues;
extern std::vector<bool> GVolumeMinusWasPressed;
extern std::vector<bool> GVolumePlusWasPressed;

// ── Tab1（物品）功能控件 ──

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

// ── Tab2（战斗）功能控件 ──

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
// 鈹€鈹€ 鍔ㄦ€?Tab (6/7/8) 鈹€鈹€

struct DynTabState {
    UBP_JHConfigTabBtn_C* Btn6 = nullptr;
    UBP_JHConfigTabBtn_C* Btn7 = nullptr;
    UBP_JHConfigTabBtn_C* Btn8 = nullptr;
    UVerticalBox* Content6 = nullptr;
    UVerticalBox* Content7 = nullptr;
    UVerticalBox* Content8 = nullptr;
};

extern DynTabState GDynTab;


// ── 队友 Tab 控件 ──

struct TeammateTabControls {
    UBPVE_JHConfigVideoItem2_C* FollowToggle = nullptr;
    UBPVE_JHConfigVolumeItem2_C* FollowCount = nullptr;
    UBPVE_JHConfigVideoItem2_C* AddDD = nullptr;
    UBPVE_JHConfigVideoItem2_C* ReplaceToggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* ReplaceDD = nullptr;
};

extern TeammateTabControls GTeammate;

// ── 任务 Tab 控件 ──

struct QuestTabControls {
    UBPVE_JHConfigVideoItem2_C* Toggle = nullptr;
    UBPVE_JHConfigVideoItem2_C* TypeDD = nullptr;
};

extern QuestTabControls GQuest;

// 物品不减功能开关（供 Inline Hook 读取）
extern std::atomic<bool> GItemNoDecreaseEnabled;

// 物品不减 Inline Hook
extern uintptr_t GChangeItemNumAddr;
extern unsigned char GOriginalChangeItemNumBytes[5];  // 保存原始入口字节(5字节)
extern void* GHookTrampoline;  // Hook 跳板内存
extern bool GInlineHookInstalled;

// 动态启用/禁用物品不减 Hook
void EnableItemNoDecreaseHook();
void DisableItemNoDecreaseHook();

// 物品获得加倍 Inline Hook
void SetItemGainMultiplierHookValue(int32 Value);
void EnableItemGainMultiplierHook();
void DisableItemGainMultiplierHook();

// 锻造制衣效果加倍 Inline Hook
void SetCraftItemIncrementHookValue(float Value);
void SetCraftExtraEffectHookValue(float Value);
void EnableCraftEffectMultiplierHook();
void DisableCraftEffectMultiplierHook();

// 最大额外词条数 Inline Hook（双 Hook）
void EnableMaxExtraAffixesHooks();
void DisableMaxExtraAffixesHooks();

// 所有物品可出售
void EnableAllItemsSellable();
void DisableAllItemsSellable();
void EnableDropRate100Patch();
void DisableDropRate100Patch();
void EnableIgnoreItemUseCountFeature();
void DisableIgnoreItemUseCountFeature();
void EnableIgnoreItemRequirementsPatch();
void DisableIgnoreItemRequirementsPatch();

// Tab2: 招式无视冷却（双 Hook）
void EnableSkillNoCooldownHooks();
void DisableSkillNoCooldownHooks();

// Tab2: 不遇敌（硬编码补丁）
void EnableNoEncounterPatch();
void DisableNoEncounterPatch();

// Tab2: 战败视为胜利（inline hook，复刻 CT）
void EnableDefeatAsVictoryHook();
void DisableDefeatAsVictoryHook();

// Tab2: 全队友参战（完整复刻 CT: allInFight1~5）
void EnableAllTeammatesInFightHooks();
void DisableAllTeammatesInFightHooks();

// Tab2: 所有心法可填装最后一格（1 inline hook + 2 硬编码补丁）
void EnableNeiGongFillLastSlotFeature();
void DisableNeiGongFillLastSlotFeature();

// Tab2: 战斗加速（双 Hook）
void EnableBattleSpeedHooks();
void DisableBattleSpeedHooks();
void SetBattleSpeedHookMultiplier(float Value);

// Tab2: 战斗前自动恢复气血和真气（inline hook）
void EnableAutoRecoverHpMpHook();
void DisableAutoRecoverHpMpHook();

// Tab2: 总移动速度加倍（inline hook）
void EnableTotalMoveSpeedHook();
void DisableTotalMoveSpeedHook();
void SetTotalMoveSpeedMultiplier(float Value);
void SetTotalMoveSpeedFriendlyOnly(bool Enabled);
