#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <cstring>

#include "ItemBrowser.hpp"
#include "WidgetUtils.hpp"
void BuildItemCache()
{
	if (GItemCacheBuilt) return;
	std::cout << "[SDK] Building item cache...\n";

	// Resolve class for fallback scan path
	UClass* ResMgrClass = UObject::FindClassFast("ItemResManager");
	if (!ResMgrClass)
		ResMgrClass = BasicFilesImpleUtils::FindClassByName("ItemResManager");

	// Primary path: use SDK static function (preferred over raw object scan)
	UItemResManager* ResMgr = UManagerFuncLib::GetItemResManager();
	if (ResMgr)
		std::cout << "[SDK] ItemResManager from ManagerFuncLib: " << (void*)ResMgr << "\n";

	// Fallback path: scan GObjects for a live instance
	if (!ResMgr && ResMgrClass)
		ResMgr = static_cast<UItemResManager*>(FindFirstObjectOfClass(ResMgrClass));

	if (!ResMgr) {
		std::cout << "[SDK] ItemResManager instance not found\n";
		return;
	}
	std::cout << "[SDK] ItemResManager at " << (void*)ResMgr << "\n";

	// ItemDataTable at UItemResManager+0x30
	UDataTable* DataTable = GetItemDataTableFromManager(ResMgr);

	// If table is null, scan all instances to find one with a valid table.
	// This avoids accidentally using a stale subsystem instance.
	if (!DataTable && ResMgrClass)
	{
		auto* ObjArray = UObject::GObjects.GetTypedPtr();
		UObject* CDO = ResMgrClass->ClassDefaultObject;
		if (ObjArray)
		{
			int32 Num = ObjArray->Num();
			for (int32 i = 0; i < Num; i++)
			{
				UObject* Obj = ObjArray->GetByIndex(i);
				if (!Obj || Obj == CDO || !Obj->IsA(ResMgrClass))
					continue;

				auto* Candidate = static_cast<UItemResManager*>(Obj);
				UDataTable* CandidateTable = GetItemDataTableFromManager(Candidate);
				if (CandidateTable)
				{
					ResMgr = Candidate;
					DataTable = CandidateTable;
					std::cout << "[SDK] ItemResManager fallback candidate: " << (void*)ResMgr << "\n";
					break;
				}
			}
		}
	}

	if (!DataTable) {
		std::cout << "[SDK] ItemDataTable is null\n";
		return;
	}
	std::cout << "[SDK] ItemDataTable at " << (void*)DataTable
	          << ", RowStruct=" << (void*)DataTable->RowStruct << "\n";
    // RowMap: use SDK container layout directly (no raw pointer arithmetic)
    auto& RowMap = DataTable->RowMap;
    if (!RowMap.IsValid())
    {
        std::cout << "[SDK] RowMap container invalid\n";
        return;
    }
    const int32 AllocatedSlots = RowMap.NumAllocated();
    const int32 RowCount = RowMap.Num();
    if (AllocatedSlots <= 0 || RowCount <= 0)
    {
        std::cout << "[SDK] RowMap empty/invalid: rows=" << RowCount
                  << " allocated=" << AllocatedSlots << "\n";
        return;
    }
    std::cout << "[SDK] RowMap: rows=" << RowCount << " allocated=" << AllocatedSlots << "\n";

    GAllItems.clear();
    GAllItems.reserve(RowCount);
    int valid = 0;

    for (int32 i = 0; i < AllocatedSlots; i++)
    {
        if (!RowMap.IsValidIndex(i))
            continue;

        uint8* RowData = RowMap[i].Value();
        if (!RowData) continue;

		CachedItem Item = {};
		// FItemInfoSetting (inherits FTableRowBase at +0x00):
		//   +0x08: int32 ID
		//   +0x10: FText Name (0x18 bytes, first 8 = TextData*)
		//   +0x40: EItemSubType (1 byte)
		//   +0x50: TSoftObjectPtr<UTexture2D> Icon (0x28 bytes)
		//   +0x80: EItemQuality (1 byte)
		Item.DefId   = *reinterpret_cast<int32*>(RowData + 0x08);
		Item.SubType = *reinterpret_cast<uint8*>(RowData + 0x40);
		Item.Quality = *reinterpret_cast<uint8*>(RowData + 0x80);

		// Read Name from FText at +0x10
		auto* TextData = *reinterpret_cast<FTextImpl::FTextData**>(RowData + 0x10);
		if (TextData) {
			const wchar_t* WStr = TextData->TextSource.CStr();
			if (WStr && WStr[0])
				wcsncpy_s(Item.Name, 64, WStr, _TRUNCATE);
		}

		// Read Icon (raw 0x28 bytes of TSoftObjectPtr)
		memcpy(Item.IconData, RowData + 0x50, 0x28);
		// Check if icon FName is valid (FSoftObjectPath.AssetPathName at TSoftObjectPtr+0x10)
		FName* IconAssetName = reinterpret_cast<FName*>(Item.IconData + 0x10);
		Item.HasIcon = !IconAssetName->IsNone();

		if (Item.Name[0] != 0 && Item.DefId > 0) {
			GAllItems.push_back(Item);
			valid++;
		}
	}

	// Sort by DefId for consistent display
	std::sort(GAllItems.begin(), GAllItems.end(),
		[](const CachedItem& a, const CachedItem& b) { return a.DefId < b.DefId; });

	GItemCacheBuilt = true;
	std::cout << "[SDK] Item cache: " << valid << " items loaded\n";
}

// Set icon brush through SDK wrapper (UImage::SetBrushFromSoftTexture)
void SetImageFromSoftTextureBySDK(UImage* ImageWidget, const uint8* SoftTextureData28)
{
	if (!ImageWidget || !SoftTextureData28)
		return;

	TSoftObjectPtr<class UTexture2D> SoftTexture{};
	memcpy(&SoftTexture, SoftTextureData28, sizeof(SoftTexture));
	ImageWidget->SetBrushFromSoftTexture(SoftTexture, false);
}

// Filter items by category, rebuild GFilteredIndices
void FilterItems(int32 category)
{
	GFilteredIndices.clear();
	for (int32 i = 0; i < (int32)GAllItems.size(); i++)
	{
		bool match = false;
		uint8 st = GAllItems[i].SubType;
		switch (category) {
		case 0: match = true; break;                          // 全部
		case 1: match = (st >= 1 && st <= 6); break;         // 武器
		case 2: match = (st >= 10 && st <= 13); break;       // 防具
		case 3: match = (st >= 14 && st <= 17); break;       // 消耗品
		default: match = (st == 0 || st > 17); break;        // 其他
		}
		if (match)
			GFilteredIndices.push_back(i);
	}
	GItemTotalPages = ((int32)GFilteredIndices.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
	if (GItemTotalPages < 1) GItemTotalPages = 1;
	std::cout << "[SDK] Filter cat=" << category << ": " << GFilteredIndices.size()
	          << " items, " << GItemTotalPages << " pages\n";
}

// Refresh item grid to show the current page
void RefreshItemPage()
{
	if (GItemCurrentPage >= GItemTotalPages)
		GItemCurrentPage = GItemTotalPages - 1;
	if (GItemCurrentPage < 0)
		GItemCurrentPage = 0;

	int32 startIdx = GItemCurrentPage * ITEMS_PER_PAGE;
	for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
	{
		GItemSlotItemIndices[i] = -1;
		auto* Btn = GItemSlotButtons[i];
		auto* Img = GItemSlotImages[i];
		if (!Btn)
			continue;

		int32 filtIdx = startIdx + i;
		if (filtIdx < (int32)GFilteredIndices.size())
		{
			int32 itemIdx = GFilteredIndices[filtIdx];
			GItemSlotItemIndices[i] = itemIdx;
			CachedItem& CI = GAllItems[itemIdx];

			Btn->SetIsEnabled(true);
			Btn->SetVisibility(ESlateVisibility::Visible);

			wchar_t Tip[96] = {};
			swprintf_s(Tip, 96, L"%s [%d]", CI.Name, CI.DefId);
			Btn->SetToolTipText(MakeText(Tip));

			if (Img)
			{
				if (CI.HasIcon)
				{
					Img->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					SetImageFromSoftTextureBySDK(Img, CI.IconData);
				}
				else
				{
					Img->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		else
		{
			Btn->SetIsEnabled(false);
			Btn->SetToolTipText(MakeText(L""));
			if (Img)
				Img->SetVisibility(ESlateVisibility::Collapsed);
		}

		GItemSlotWasPressed[i] = false;
	}

	if (GItemPageLabel)
	{
		wchar_t Buf[16] = {};
		swprintf_s(Buf, 16, L"%d/%d", GItemCurrentPage + 1, GItemTotalPages);
		GItemPageLabel->SetText(MakeText(Buf));
	}
	if (GItemPrevPageBtn)
		GItemPrevPageBtn->SetIsEnabled(GItemCurrentPage > 0);
	if (GItemNextPageBtn)
		GItemNextPageBtn->SetIsEnabled((GItemCurrentPage + 1) < GItemTotalPages);
}

// Clear item browser widget state (called when panel closes)
void ClearItemBrowserState()
{
	GItemCategoryDD = nullptr;
	GItemLastCatIdx = -1;

	GItemPagerRow = nullptr;
	GItemPrevPageBtn = nullptr;
	GItemNextPageBtn = nullptr;
	GItemPageLabel = nullptr;
	GItemPrevWasPressed = false;
	GItemNextWasPressed = false;

	GItemQuantityRow = nullptr;
	GItemQuantityEdit = nullptr;
	GItemAddQuantity = 1;

	GItemGridPanel = nullptr;
	for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
	{
		GItemSlotButtons[i] = nullptr;
		GItemSlotImages[i] = nullptr;
		GItemSlotItemIndices[i] = -1;
		GItemSlotWasPressed[i] = false;
	}

	GItemCurrentPage = 0;
	GItemTotalPages = 0;
}

