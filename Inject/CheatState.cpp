#include "CheatState.hpp"

GVCPostRenderFn OriginalGVCPostRender = nullptr;
VTableHook GVCPostRenderHook;
UUserWidget* InternalWidget = nullptr;
bool InternalWidgetVisible = false;
std::atomic<bool> GIsUnloading = false;
std::atomic<bool> GHooksRemoved = false;
std::atomic<bool> GConsoleClosed = false;

UButton* GCachedBtnExit = nullptr;
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
int32 GItemSlotItemIndices[ITEMS_PER_PAGE] = {};
bool GItemSlotWasPressed[ITEMS_PER_PAGE] = {};

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
