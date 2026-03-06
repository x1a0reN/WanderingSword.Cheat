// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  CheatState.cpp  —  全局变量定义 (对应 CheatState.hpp 声明)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#include "CheatState.hpp"

// ── Hook 与核心运行状态 ──
GVCPostRenderFn          GOriginalPostRender    = nullptr;
VTableHook               GVCPostRenderHook;
UUserWidget*             GInternalWidget        = nullptr;
bool                     GInternalWidgetVisible = false;
std::atomic<bool>        GIsUnloading           = false;
std::atomic<bool>        GUnloadCleanupDone     = false;
std::atomic<bool>        GHooksRemoved          = false;
std::atomic<bool>        GConsoleClosed         = false;
std::atomic<int32>       GPostRenderInFlight    = 0;
UIRememberState          GUIRememberState{};

// ── 劫持的原生面板引用 ──
UNeoUIButtonBase*        GCachedBtnExit         = nullptr;
std::vector<UObject*>    GRootedObjects;
UWidget*                 GOriginalLanPanel      = nullptr;
UWidget*                 GOriginalInputMappingPanel = nullptr;
UJHCommon_Btn_Free_C*    GOriginalResetButton   = nullptr;

// ── 物品浏览器 ──
ItemBrowserState         GItemBrowser{};

// ── 滑块控件缓存 ──
std::vector<UBPVE_JHConfigVolumeItem2_C*> GVolumeItems;
std::vector<float>       GVolumeLastValues;
std::vector<bool>        GVolumeMinusWasPressed;
std::vector<bool>        GVolumePlusWasPressed;

// ── Tab 页面控件实例 ──
Tab1Controls             GTab1{};
Tab2Controls             GTab2{};
Tab3Controls             GTab3{};
Tab4Controls             GTab4{};
Tab5Controls             GTab5{};
DynTabState              GDynTab{};
TeammateTabControls      GTeammate{};
QuestTabControls         GQuest{};

// ── 物品不减 Inline Hook 底层变量 ──
std::atomic<bool>        GItemNoDecreaseEnabled = false;
uintptr_t                GChangeItemNumAddr     = 0;
unsigned char            GOriginalChangeItemNumBytes[5] = {};
void*                    GHookTrampoline        = nullptr;
bool                     GInlineHookInstalled   = false;
