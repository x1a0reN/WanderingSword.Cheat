#include "CheatState.hpp"

GVCPostRenderFn GOriginalPostRender = nullptr;
VTableHook GVCPostRenderHook;
UUserWidget* GInternalWidget = nullptr;
bool GInternalWidgetVisible = false;
std::atomic<bool> GIsUnloading = false;
std::atomic<bool> GUnloadCleanupDone = false;
std::atomic<bool> GHooksRemoved = false;
std::atomic<bool> GConsoleClosed = false;
std::atomic<int32> GPostRenderInFlight = 0;
UIRememberState GUIRememberState{};

std::atomic<bool> GItemNoDecreaseEnabled = false;

uintptr_t GChangeItemNumAddr = 0;
unsigned char GOriginalChangeItemNumBytes[5] = {};
void* GHookTrampoline = nullptr;
bool GInlineHookInstalled = false;

UNeoUIButtonBase* GCachedBtnExit = nullptr;
std::vector<UObject*> GRootedObjects;
UWidget* GOriginalLanPanel = nullptr;
UWidget* GOriginalInputMappingPanel = nullptr;
UJHCommon_Btn_Free_C* GOriginalResetButton = nullptr;

ItemBrowserState GItemBrowser{};

std::vector<UBPVE_JHConfigVolumeItem2_C*> GVolumeItems;
std::vector<float> GVolumeLastValues;
std::vector<bool> GVolumeMinusWasPressed;
std::vector<bool> GVolumePlusWasPressed;

Tab1Controls GTab1{};
Tab2Controls GTab2{};
DynTabState GDynTab{};
TeammateTabControls GTeammate{};
QuestTabControls GQuest{};
