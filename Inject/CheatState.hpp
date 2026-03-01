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

/// 鐗╁搧缃戞牸甯冨眬甯搁噺
inline constexpr int32 ITEM_GRID_COLS = 6;
inline constexpr int32 ITEM_GRID_ROWS = 4;
inline constexpr int32 ITEMS_PER_PAGE = ITEM_GRID_COLS * ITEM_GRID_ROWS; // 24

// 鈹€鈹€ Hook 涓庢牳蹇冪姸鎬?鈹€鈹€

extern GVCPostRenderFn OriginalGVCPostRender;   // PostRender 鍘熷鍑芥暟鎸囬拡
extern VTableHook GVCPostRenderHook;             // VTable Hook 瀹炰緥
extern UUserWidget* InternalWidget;              // 浣滃紛闈㈡澘 Widget 瀹炰緥
extern bool InternalWidgetVisible;               // 闈㈡澘鍙鎬ф爣璁?
extern std::atomic<bool> GIsUnloading;           // DLL 姝ｅ湪鍗歌浇
extern std::atomic<bool> GUnloadCleanupDone;     // 卸载清理是否已在游戏线程完成
extern std::atomic<bool> GHooksRemoved;          // Hook 宸茬Щ闄?
extern std::atomic<bool> GConsoleClosed;         // 鎺у埗鍙板凡鍏抽棴
extern std::atomic<int32> GPostRenderInFlight;   // HookedGVCPostRender 当前在执行的调用数

// 鈹€鈹€ 闈㈡澘 UI 寮曠敤 鈹€鈹€

extern UNeoUIButtonBase* GCachedBtnExit;         // 缂撳瓨鐨勫叧闂寜閽?(姣忓抚妫€娴嬬偣鍑?
extern std::vector<UObject*> GRootedObjects;     // 宸叉爣璁?GC Root 鐨勫璞″垪琛?
extern UWidget* GOriginalLanPanel;               // 淇濈暀鐨勫師鐢熻瑷€闈㈡澘
extern UWidget* GOriginalInputMappingPanel;      // 淇濈暀鐨勫師鐢熼敭浣嶉潰鏉?
extern UJHCommon_Btn_Free_C* GOriginalResetButton; // 淇濈暀鐨勫師鐢熼噸缃寜閽?

// 鈹€鈹€ 鐗╁搧娴忚鍣ㄧ姸鎬?鈹€鈹€

extern std::vector<CachedItem> GAllItems;        // 鍏ㄩ噺鐗╁搧缂撳瓨
extern std::vector<int32> GFilteredIndices;       // 褰撳墠鍒嗙被杩囨护鍚庣殑绱㈠紩鍒楄〃
extern bool GItemCacheBuilt;                      // 缂撳瓨鏄惁宸叉瀯寤?
extern int32 GItemCurrentPage;                    // 褰撳墠椤电爜 (0-based)
extern int32 GItemTotalPages;                     // 鎬婚〉鏁?

extern UBPVE_JHConfigVideoItem2_C* GItemCategoryDD; // 鐗╁搧鍒嗙被涓嬫媺妗?
extern int32 GItemLastCatIdx;                     // 涓婃閫変腑鐨勫垎绫荤储寮?
extern UHorizontalBox* GItemPagerRow;             // 缈婚〉鎸夐挳琛屽鍣?
extern UJHCommon_Btn_Free_C* GItemPrevPageBtn;    // 涓婁竴椤垫寜閽?
extern UJHCommon_Btn_Free_C* GItemNextPageBtn;    // 涓嬩竴椤垫寜閽?
extern UTextBlock* GItemPageLabel;                // 椤电爜鏄剧ず鏍囩
extern bool GItemPrevWasPressed;                  // 涓婁竴椤垫寜閽笂甯ф寜涓嬬姸鎬?(杈规部瑙﹀彂)
extern bool GItemNextWasPressed;                  // 涓嬩竴椤垫寜閽笂甯ф寜涓嬬姸鎬?(杈规部瑙﹀彂)
extern UHorizontalBox* GItemQuantityRow;          // 娣诲姞鏁伴噺杈撳叆琛屽鍣?
extern UEditableTextBox* GItemQuantityEdit;       // 娣诲姞鏁伴噺杈撳叆妗?
extern int32 GItemAddQuantity;                    // 褰撳墠娣诲姞鏁伴噺鍊?
extern UUniformGridPanel* GItemGridPanel;         // 6x4 鐗╁搧鍥炬爣缃戞牸
extern UButton* GItemSlotButtons[ITEMS_PER_PAGE]; // 24 涓墿鍝佹Ы鎸夐挳
extern UImage* GItemSlotImages[ITEMS_PER_PAGE];   // 24 涓墿鍝佹Ы鍥炬爣
extern UImage* GItemSlotQualityBorders[ITEMS_PER_PAGE];
extern UUserWidget* GItemSlotEntryWidgets[ITEMS_PER_PAGE];
extern int32 GItemSlotItemIndices[ITEMS_PER_PAGE]; // 姣忎釜妲戒綅瀵瑰簲鐨勭墿鍝佺储寮?(-1=绌?
extern bool GItemSlotWasPressed[ITEMS_PER_PAGE];  // 鐗╁搧妲戒笂甯ф寜涓嬬姸鎬?(杈规部瑙﹀彂)
extern int32 GItemHoveredSlot;
extern UJHNeoUITipsVEBase* GItemHoverTipsWidget;
extern UItemInfoSpec* GItemHoverTempSpec;
extern std::vector<UBPVE_JHConfigVolumeItem2_C*> GVolumeItems; // 婊戝潡鎺т欢缂撳瓨锛堢敤浜庢枃鏈悓姝ワ級
extern std::vector<float> GVolumeLastValues;      // 婊戝潡涓婂抚鏁板€?
extern std::vector<bool> GVolumeMinusWasPressed;  // "-" 鎸夐挳涓婂抚鎸変笅鐘舵€?
extern std::vector<bool> GVolumePlusWasPressed;   // "+" 鎸夐挳涓婂抚鎸変笅鐘舵€?

// Tab1（物品）功能控件引用
extern UBPVE_JHConfigVideoItem2_C* GTab1ItemNoDecreaseToggle;
extern UBPVE_JHConfigVideoItem2_C* GTab1ItemGainMultiplierToggle;
extern UBPVE_JHConfigVolumeItem2_C* GTab1ItemGainMultiplierSlider;
extern UBPVE_JHConfigVideoItem2_C* GTab1AllItemsSellableToggle;
extern UBPVE_JHConfigVideoItem2_C* GTab1IncludeQuestItemsToggle;
extern UBPVE_JHConfigVideoItem2_C* GTab1DropRate100Toggle;
extern UBPVE_JHConfigVideoItem2_C* GTab1CraftEffectMultiplierToggle;
extern UBPVE_JHConfigVolumeItem2_C* GTab1CraftItemIncrementSlider;
extern UBPVE_JHConfigVolumeItem2_C* GTab1CraftExtraEffectSlider;
extern UEditableTextBox* GTab1MaxExtraAffixesEdit;
extern UBPVE_JHConfigVideoItem2_C* GTab1IgnoreItemUseCountToggle;
extern UBPVE_JHConfigVideoItem2_C* GTab1IgnoreItemRequirementsToggle;
// 鈹€鈹€ 鍔ㄦ€?Tab (6/7/8) 鈹€鈹€

extern UBP_JHConfigTabBtn_C* GDynTabBtn6;         // Tab 6 (闃熷弸) 鎸夐挳
extern UBP_JHConfigTabBtn_C* GDynTabBtn7;         // Tab 7 (浠诲姟) 鎸夐挳
extern UBP_JHConfigTabBtn_C* GDynTabBtn8;         // Tab 8 (鎺т欢) 鎸夐挳
extern UVerticalBox* GDynTabContent6;              // Tab 6 鍐呭瀹瑰櫒
extern UVerticalBox* GDynTabContent7;              // Tab 7 鍐呭瀹瑰櫒
extern UVerticalBox* GDynTabContent8;              // Tab 8 鍐呭瀹瑰櫒

// 鈹€鈹€ 闃熷弸 Tab 鎺т欢 鈹€鈹€

extern UBPVE_JHConfigVideoItem2_C* GTeammateFollowToggle;  // 璁剧疆闃熷弸璺熼殢鏁伴噺寮€鍏?
extern UBPVE_JHConfigVolumeItem2_C* GTeammateFollowCount;   // 璺熼殢鏁伴噺婊戝潡
extern UBPVE_JHConfigVideoItem2_C* GTeammateAddDD;          // 娣诲姞闃熷弸涓嬫媺妗?
extern UBPVE_JHConfigVideoItem2_C* GTeammateReplaceToggle;  // 鏇挎崲鎸囧畾闃熷弸寮€鍏?
extern UBPVE_JHConfigVideoItem2_C* GTeammateReplaceDD;      // 鎸囧畾闃熷弸涓嬫媺妗?

// 鈹€鈹€ 浠诲姟 Tab 鎺т欢 鈹€鈹€

extern UBPVE_JHConfigVideoItem2_C* GQuestToggle;   // 鎺ュ埌/瀹屾垚浠诲姟寮€鍏?
extern UBPVE_JHConfigVideoItem2_C* GQuestTypeDD;   // 鎵ц绫诲瀷涓嬫媺妗?(鎺ュ埌/瀹屾垚)
