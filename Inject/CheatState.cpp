#include "CheatState.hpp"

GVCPostRenderFn OriginalGVCPostRender = nullptr;
VTableHook GVCPostRenderHook;
UUserWidget* InternalWidget = nullptr;
bool InternalWidgetVisible = false;
std::atomic<bool> GIsUnloading = false;
std::atomic<bool> GUnloadCleanupDone = false;
std::atomic<bool> GHooksRemoved = false;
std::atomic<bool> GConsoleClosed = false;
std::atomic<int32> GPostRenderInFlight = 0;

// 物品不减功能开关
std::atomic<bool> GItemNoDecreaseEnabled = false;

// 物品不减 Inline Hook 变量
uintptr_t GChangeItemNumAddr = 0;
unsigned char GOriginalChangeItemNumBytes[14] = {};
bool GInlineHookInstalled = false;

UNeoUIButtonBase* GCachedBtnExit = nullptr;
std::vector<UObject*> GRootedObjects;
UWidget* GOriginalLanPanel = nullptr;
UWidget* GOriginalInputMappingPanel = nullptr;
UJHCommon_Btn_Free_C* GOriginalResetButton = nullptr;

std::vector<CachedItem> GAllItems;
std::vector<int32> GFilteredIndices;
bool GItemCacheBuilt = false;
int32 GItemCurrentPage = 0;
int32 GItemTotalPages = 0;

UBPVE_JHConfigVideoItem2_C* GItemCategoryDD = nullptr;
int32 GItemLastCatIdx = -1;
UHorizontalBox* GItemPagerRow = nullptr;
UJHCommon_Btn_Free_C* GItemPrevPageBtn = nullptr;
UJHCommon_Btn_Free_C* GItemNextPageBtn = nullptr;
UTextBlock* GItemPageLabel = nullptr;
bool GItemPrevWasPressed = false;
bool GItemNextWasPressed = false;
UHorizontalBox* GItemQuantityRow = nullptr;
UEditableTextBox* GItemQuantityEdit = nullptr;
int32 GItemAddQuantity = 1;
UUniformGridPanel* GItemGridPanel = nullptr;
UButton* GItemSlotButtons[ITEMS_PER_PAGE] = {};
UImage* GItemSlotImages[ITEMS_PER_PAGE] = {};
UImage* GItemSlotQualityBorders[ITEMS_PER_PAGE] = {};
UUserWidget* GItemSlotEntryWidgets[ITEMS_PER_PAGE] = {};
int32 GItemSlotItemIndices[ITEMS_PER_PAGE] = {};
bool GItemSlotWasPressed[ITEMS_PER_PAGE] = {};
int32 GItemHoveredSlot = -1;
UJHNeoUITipsVEBase* GItemHoverTipsWidget = nullptr;
UItemInfoSpec* GItemHoverTempSpec = nullptr;
std::vector<UBPVE_JHConfigVolumeItem2_C*> GVolumeItems;
std::vector<float> GVolumeLastValues;
std::vector<bool> GVolumeMinusWasPressed;
std::vector<bool> GVolumePlusWasPressed;

UBPVE_JHConfigVideoItem2_C* GTab1ItemNoDecreaseToggle = nullptr;
UBPVE_JHConfigVideoItem2_C* GTab1ItemGainMultiplierToggle = nullptr;
UBPVE_JHConfigVolumeItem2_C* GTab1ItemGainMultiplierSlider = nullptr;
UBPVE_JHConfigVideoItem2_C* GTab1AllItemsSellableToggle = nullptr;
UBPVE_JHConfigVideoItem2_C* GTab1IncludeQuestItemsToggle = nullptr;
UBPVE_JHConfigVideoItem2_C* GTab1DropRate100Toggle = nullptr;
UBPVE_JHConfigVideoItem2_C* GTab1CraftEffectMultiplierToggle = nullptr;
UBPVE_JHConfigVolumeItem2_C* GTab1CraftItemIncrementSlider = nullptr;
UBPVE_JHConfigVolumeItem2_C* GTab1CraftExtraEffectSlider = nullptr;
UEditableTextBox* GTab1MaxExtraAffixesEdit = nullptr;
UBPVE_JHConfigVideoItem2_C* GTab1IgnoreItemUseCountToggle = nullptr;
UBPVE_JHConfigVideoItem2_C* GTab1IgnoreItemRequirementsToggle = nullptr;

UBP_JHConfigTabBtn_C* GDynTabBtn6 = nullptr;
UBP_JHConfigTabBtn_C* GDynTabBtn7 = nullptr;
UBP_JHConfigTabBtn_C* GDynTabBtn8 = nullptr;
UVerticalBox* GDynTabContent6 = nullptr;
UVerticalBox* GDynTabContent7 = nullptr;
UVerticalBox* GDynTabContent8 = nullptr;


UBPVE_JHConfigVideoItem2_C* GTeammateFollowToggle = nullptr;
UBPVE_JHConfigVolumeItem2_C* GTeammateFollowCount = nullptr;
UBPVE_JHConfigVideoItem2_C* GTeammateAddDD = nullptr;
UBPVE_JHConfigVideoItem2_C* GTeammateReplaceToggle = nullptr;
UBPVE_JHConfigVideoItem2_C* GTeammateReplaceDD = nullptr;

UBPVE_JHConfigVideoItem2_C* GQuestToggle = nullptr;
UBPVE_JHConfigVideoItem2_C* GQuestTypeDD = nullptr;
