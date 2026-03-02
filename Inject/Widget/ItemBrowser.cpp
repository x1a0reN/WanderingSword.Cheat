#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

#include "ItemBrowser.hpp"
#include "GCManager.hpp"
#include "WidgetUtils.hpp"
#include "SDK/BPEntry_Item_classes.hpp"
#include "SDK/BPEntry_Item_WDT_classes.hpp"
#include "SDK/BPVE_JHTips_Item_classes.hpp"
#include "SDK/BPVE_TipsBlock_classes.hpp"
#include "Logging.hpp"

namespace
{
	UBPVE_JHTips_Item_C* GStandaloneItemTipWidget = nullptr;
	std::vector<UItemInfoSpec*> GCurrentPageSpecs;
	constexpr int32 kItemTipZOrder = 20050;
	constexpr bool kVerboseItemHoverLogs = false;
	constexpr float kItemTipDefaultWidth = 520.0f;
	constexpr float kItemTipDefaultHeight = 420.0f;
	constexpr float kItemTipMouseOffset = 18.0f;
	constexpr float kItemTipViewportMargin = 12.0f;

	const char* VisName(ESlateVisibility V)
	{
		switch (V)
		{
		case ESlateVisibility::Visible: return "Visible";
		case ESlateVisibility::Collapsed: return "Collapsed";
		case ESlateVisibility::Hidden: return "Hidden";
		case ESlateVisibility::HitTestInvisible: return "HitTestInvisible";
		case ESlateVisibility::SelfHitTestInvisible: return "SelfHitTestInvisible";
		default: return "Unknown";
		}
	}

	int32 SanitizeItemQuality(uint8 RawQuality)
	{
		const int32 Q = static_cast<int32>(RawQuality);
		const int32 MaxQ = static_cast<int32>(EItemQuality::Red);
		if (Q < 0 || Q > MaxQ)
			return static_cast<int32>(EItemQuality::White);
		return Q;
	}

	uint64 MakeAssetPathKey(const FName& Name)
	{
		return (static_cast<uint64>(static_cast<uint32>(Name.ComparisonIndex)) << 32) |
			static_cast<uint64>(Name.Number);
	}

	const FName* GetAssetPathNameFromSoftTextureData(const uint8* SoftTextureData28)
	{
		if (!SoftTextureData28)
			return nullptr;
		return reinterpret_cast<const FName*>(SoftTextureData28 + 0x10);
	}

	UGameInstance* GetCurrentGameInstance()
	{
		UWorld* World = UWorld::GetWorld();
		if (!World)
			return nullptr;

		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC && IsSafeLiveObject(static_cast<UObject*>(PC)))
			return UGameplayStatics::GetGameInstance(PC);

		return World->OwningGameInstance;
	}

	UTexture2D* ResolveTextureFromSoftData(const uint8* SoftTextureData28)
	{
		static std::unordered_map<uint64, UTexture2D*> sIconCache;
		static std::unordered_map<uint64, DWORD> sMissingIconRetryUntil;
		static UWorld* sLastWorld = nullptr;
		static ULevel* sLastLevel = nullptr;

		UWorld* CurWorld = UWorld::GetWorld();
		ULevel* CurLevel = CurWorld ? CurWorld->PersistentLevel : nullptr;
		if (CurWorld != sLastWorld || CurLevel != sLastLevel)
		{
			sLastWorld = CurWorld;
			sLastLevel = CurLevel;
			sIconCache.clear();
			sMissingIconRetryUntil.clear();
		}

		const FName* AssetPathName = GetAssetPathNameFromSoftTextureData(SoftTextureData28);
		if (!AssetPathName || AssetPathName->IsNone())
			return nullptr;

		const uint64 Key = MakeAssetPathKey(*AssetPathName);
		const DWORD NowTick = GetTickCount();
		auto MissingIt = sMissingIconRetryUntil.find(Key);
		if (MissingIt != sMissingIconRetryUntil.end())
		{
			if (NowTick < MissingIt->second)
				return nullptr;
			sMissingIconRetryUntil.erase(MissingIt);
		}

		auto CachedIt = sIconCache.find(Key);
		if (CachedIt != sIconCache.end())
		{
			UTexture2D* CachedTex = CachedIt->second;
			if (IsSafeLiveObjectOfClass(static_cast<UObject*>(CachedTex), UTexture2D::StaticClass()))
				return CachedTex;
			sIconCache.erase(CachedIt);
		}

		TSoftObjectPtr<UTexture2D> SoftTexture{};
		memcpy(&SoftTexture, SoftTextureData28, sizeof(SoftTexture));

		UTexture2D* Texture = SoftTexture.Get();
		if (!Texture)
		{
			TSoftObjectPtr<UObject> SoftObject{};
			memcpy(&SoftObject, SoftTextureData28, sizeof(SoftObject));
			UObject* Loaded = UKismetSystemLibrary::LoadAsset_Blocking(SoftObject);
			if (Loaded && Loaded->IsA(UTexture2D::StaticClass()))
				Texture = static_cast<UTexture2D*>(Loaded);
		}

		if (IsSafeLiveObjectOfClass(static_cast<UObject*>(Texture), UTexture2D::StaticClass()))
		{
			sIconCache[Key] = Texture;
			sMissingIconRetryUntil.erase(Key);
			return Texture;
		}

		// 转场期间有些资源会临时不可用，设置重试窗口避免永久 missing 污染。
		sMissingIconRetryUntil[Key] = NowTick + 2500;
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemIconMissing: " << AssetPathName->GetRawString() << "\n";
		return nullptr;
	}

	bool IsWidgetHoveredWithGeometry(UWidget* W)
	{
		if (!IsSafeLiveObject(static_cast<UObject*>(W)))
			return false;

		if (W->IsHovered())
			return true;

		const FGeometry Geo = W->GetCachedGeometry();
		const FVector2D MouseAbs = UWidgetLayoutLibrary::GetMousePositionOnPlatform();
		if (USlateBlueprintLibrary::IsUnderLocation(Geo, MouseAbs))
			return true;

		UObject* WorldCtx = static_cast<UObject*>(W);
		if (UWorld* World = UWorld::GetWorld())
			WorldCtx = static_cast<UObject*>(World);

		// Fallback 1: compare mouse and widget rect in viewport coordinates.
		const FVector2D LocalSize = USlateBlueprintLibrary::GetLocalSize(Geo);
		if (LocalSize.X > 1.0f && LocalSize.Y > 1.0f)
		{
			FVector2D PixelTL{};
			FVector2D ViewTL{};
			FVector2D PixelBR{};
			FVector2D ViewBR{};
			USlateBlueprintLibrary::LocalToViewport(WorldCtx, Geo, FVector2D{ 0.0f, 0.0f }, &PixelTL, &ViewTL);
			USlateBlueprintLibrary::LocalToViewport(WorldCtx, Geo, LocalSize, &PixelBR, &ViewBR);

			const FVector2D MouseVP = UWidgetLayoutLibrary::GetMousePositionOnViewport(WorldCtx);
			const float MinX = (ViewTL.X < ViewBR.X) ? ViewTL.X : ViewBR.X;
			const float MaxX = (ViewTL.X > ViewBR.X) ? ViewTL.X : ViewBR.X;
			const float MinY = (ViewTL.Y < ViewBR.Y) ? ViewTL.Y : ViewBR.Y;
			const float MaxY = (ViewTL.Y > ViewBR.Y) ? ViewTL.Y : ViewBR.Y;
			if (MouseVP.X >= MinX && MouseVP.X <= MaxX &&
				MouseVP.Y >= MinY && MouseVP.Y <= MaxY)
			{
				return true;
			}
		}

		// Fallback 2: convert screen to widget-local and test local bounds.
		FVector2D LocalPos{};
		USlateBlueprintLibrary::ScreenToWidgetLocal(WorldCtx, Geo, MouseAbs, &LocalPos, true);
		if (LocalPos.X >= 0.0f && LocalPos.Y >= 0.0f &&
			LocalPos.X <= LocalSize.X && LocalPos.Y <= LocalSize.Y)
			return true;

		USlateBlueprintLibrary::ScreenToWidgetLocal(WorldCtx, Geo, MouseAbs, &LocalPos, false);
		if (LocalPos.X >= 0.0f && LocalPos.Y >= 0.0f &&
			LocalPos.X <= LocalSize.X && LocalPos.Y <= LocalSize.Y)
			return true;

		return false;
	}

	bool EnsureStandaloneItemTipWidget()
	{
		if (IsSafeLiveObject(static_cast<UObject*>(GStandaloneItemTipWidget)))
			return true;

		APlayerController* PC = nullptr;
		if (UWorld* World = UWorld::GetWorld())
			PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!PC)
			return false;

		auto* Created = static_cast<UBPVE_JHTips_Item_C*>(
			UWidgetBlueprintLibrary::Create(PC, UBPVE_JHTips_Item_C::StaticClass(), PC));
		if (!Created)
		{
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: StandaloneGameTip create failed\n";
			return false;
		}

		Created->SetOwningPlayer(PC);
		Created->AddToViewport(kItemTipZOrder);
		if (!Created->IsInViewport())
			Created->AddToPlayerScreen(kItemTipZOrder);

		Created->SetRenderOpacity(1.0f);
		Created->SetVisibility(ESlateVisibility::HitTestInvisible);
		Created->BP_AnchorTop_Left();
		Created->SetAlignmentInViewport(FVector2D{ 0.0f, 0.0f });
		Created->SetDesiredSizeInViewport(FVector2D{ kItemTipDefaultWidth, kItemTipDefaultHeight });

		// 这些区域在独立物品 Tip 中固定隐藏，只在初始化时设置一次。
		if (Created->VE_Effects)
			Created->VE_Effects->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->VE_Additional)
			Created->VE_Additional->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->NeoUIImageBase_53)
			Created->NeoUIImageBase_53->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->CT_Bottom)
			Created->CT_Bottom->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->BottomGap)
			Created->BottomGap->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->TXT_BuyTip)
			Created->TXT_BuyTip->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->TXT_SellTip)
			Created->TXT_SellTip->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->TXT_Price)
			Created->TXT_Price->SetVisibility(ESlateVisibility::Collapsed);
		if (Created->TXT_Acquired)
			Created->TXT_Acquired->SetVisibility(ESlateVisibility::Collapsed);

		if (Created->RootCanvas && Created->RootCanvas->Slot)
		{
			auto* RcSlot = static_cast<UCanvasPanelSlot*>(Created->RootCanvas->Slot);
			FVector2D OldPos = RcSlot->GetPosition();
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemTip RootCanvas slot pos before reset: ("
			          << OldPos.X << "," << OldPos.Y << ")\n";
			RcSlot->SetPosition(FVector2D{ 0.0f, 0.0f });
		}

		MarkAsGCRoot(static_cast<UObject*>(Created));
		GStandaloneItemTipWidget = Created;
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: StandaloneGameTip created widget="
		          << (void*)GStandaloneItemTipWidget
		          << " inViewport=" << (GStandaloneItemTipWidget->IsInViewport() ? 1 : 0) << "\n";
		return IsSafeLiveObject(static_cast<UObject*>(GStandaloneItemTipWidget));
	}

	bool UpdateStandaloneItemTipContent(const CachedItem& CI)
	{
		if (!EnsureStandaloneItemTipWidget())
			return false;

		auto* Tip = GStandaloneItemTipWidget;
		if (!Tip)
			return false;

		if (Tip->Header)
		{
			Tip->Header->UpdateTitle(MakeText(CI.Name));
			Tip->Header->UpdateSpellQuality(static_cast<int32>(SanitizeItemQuality(CI.Quality)));
			if (CI.HasIcon)
			{
				if (UTexture2D* IconTex = ResolveTextureFromSoftData(CI.IconData))
					Tip->Header->UpdateIconFromTexture(IconTex);
			}
		}

		if (Tip->VE_Desc)
		{
			const wchar_t* Desc = (CI.Desc[0] != 0) ? CI.Desc : L"";
			Tip->VE_Desc->UpdateDesc(MakeText(Desc));
		}

		Tip->SetRenderOpacity(1.0f);
		Tip->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (!Tip->IsInViewport())
		{
			Tip->AddToViewport(kItemTipZOrder);
			if (!Tip->IsInViewport())
				Tip->AddToPlayerScreen(kItemTipZOrder);
		}

		return Tip->IsInViewport();
	}
}
void BuildItemCache()
{
	if (GItemCacheBuilt) return;
	LOGI_STREAM("ItemBrowser") << "[SDK] Building item cache...\n";

	// Resolve class for fallback scan path
	UClass* ResMgrClass = UObject::FindClassFast("ItemResManager");
	if (!ResMgrClass)
		ResMgrClass = BasicFilesImpleUtils::FindClassByName("ItemResManager");

	// Primary path: use SDK static function (preferred over raw object scan)
	UItemResManager* ResMgr = UManagerFuncLib::GetItemResManager();
	if (ResMgr)
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemResManager from ManagerFuncLib: " << (void*)ResMgr << "\n";

	// Fallback path: scan GObjects for a live instance
	if (!ResMgr && ResMgrClass)
		ResMgr = static_cast<UItemResManager*>(FindFirstObjectOfClass(ResMgrClass));

	if (!ResMgr) {
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemResManager instance not found\n";
		return;
	}
	LOGI_STREAM("ItemBrowser") << "[SDK] ItemResManager at " << (void*)ResMgr << "\n";

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
					LOGI_STREAM("ItemBrowser") << "[SDK] ItemResManager fallback candidate: " << (void*)ResMgr << "\n";
					break;
				}
			}
		}
	}

	if (!DataTable) {
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemDataTable is null\n";
		return;
	}
	LOGI_STREAM("ItemBrowser") << "[SDK] ItemDataTable at " << (void*)DataTable
	          << ", RowStruct=" << (void*)DataTable->RowStruct << "\n";
    // RowMap: use SDK container layout directly (no raw pointer arithmetic)
    auto& RowMap = DataTable->RowMap;
    if (!RowMap.IsValid())
    {
        LOGI_STREAM("ItemBrowser") << "[SDK] RowMap container invalid\n";
        return;
    }
    const int32 AllocatedSlots = RowMap.NumAllocated();
    const int32 RowCount = RowMap.Num();
    if (AllocatedSlots <= 0 || RowCount <= 0)
    {
        LOGI_STREAM("ItemBrowser") << "[SDK] RowMap empty/invalid: rows=" << RowCount
                  << " allocated=" << AllocatedSlots << "\n";
        return;
    }
    LOGI_STREAM("ItemBrowser") << "[SDK] RowMap: rows=" << RowCount << " allocated=" << AllocatedSlots << "\n";

    GAllItems.clear();
    GAllItems.reserve(RowCount);
    int valid = 0;

    int32 QualityHistogram[8] = {};
    int32 InvalidQualityCount = 0;

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
		const uint8 RawQuality = *reinterpret_cast<uint8*>(RowData + 0x80);
		const int32 Quality = SanitizeItemQuality(RawQuality);
		if (Quality != static_cast<int32>(RawQuality))
			++InvalidQualityCount;
		Item.Quality = static_cast<uint8>(Quality);
		if (Quality >= 0 && Quality < 8)
			++QualityHistogram[Quality];

		// Read Name from FText at +0x10
		auto* TextData = *reinterpret_cast<FTextImpl::FTextData**>(RowData + 0x10);
		if (TextData) {
			const wchar_t* WStr = TextData->TextSource.CStr();
			if (WStr && WStr[0])
				wcsncpy_s(Item.Name, 64, WStr, _TRUNCATE);
		}

		// Read Description from FText at +0x28
		auto* DescData = *reinterpret_cast<FTextImpl::FTextData**>(RowData + 0x28);
		if (DescData) {
			const wchar_t* WDesc = DescData->TextSource.CStr();
			if (WDesc && WDesc[0])
				wcsncpy_s(Item.Desc, 256, WDesc, _TRUNCATE);
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
	LOGI_STREAM("ItemBrowser") << "[SDK] Item cache: " << valid << " items loaded\n";
	LOGI_STREAM("ItemBrowser") << "[SDK] Item quality histogram:"
	          << " Q0=" << QualityHistogram[0]
	          << " Q1=" << QualityHistogram[1]
	          << " Q2=" << QualityHistogram[2]
	          << " Q3=" << QualityHistogram[3]
	          << " Q4=" << QualityHistogram[4]
	          << " Q5=" << QualityHistogram[5]
	          << " Q6=" << QualityHistogram[6]
	          << " invalid=" << InvalidQualityCount << "\n";
}

// Set icon brush through SDK wrapper (UImage::SetBrushFromSoftTexture)
void SetImageFromSoftTextureBySDK(UImage* ImageWidget, const uint8* SoftTextureData28)
{
	if (!SoftTextureData28 || !IsSafeLiveObjectOfClass(static_cast<UObject*>(ImageWidget), UImage::StaticClass()))
		return;

	UTexture2D* Texture = ResolveTextureFromSoftData(SoftTextureData28);
	if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(Texture), UTexture2D::StaticClass()))
		return;

	ImageWidget->SetBrushFromTexture(Texture, true);
}

static UTexture2D* GetItemQualityBorderTexture(uint8 Quality)
{
	const int32 SafeQuality = SanitizeItemQuality(Quality);
	UTexture2D* Tex = UJHNeoUITextureLoader::JHIcon_Item_QualityBorder(SafeQuality);
	if (!Tex)
		Tex = UJHNeoUITextureLoader_Console::JHIcon_C_Item_QualityBorder(SafeQuality);
	return Tex;
}

static UJHNeoUISubsystem* GetJHNeoUISubsystem()
{
	static UJHNeoUISubsystem* Cached = nullptr;
	UGameInstance* CurrentGI = GetCurrentGameInstance();

	auto IsRuntimeSubsystem = [&](UJHNeoUISubsystem* S) -> bool
	{
		if (!S)
			return false;
		auto* Obj = static_cast<UObject*>(S);
		if (!Obj || Obj->IsDefaultObject())
			return false;
		if ((static_cast<int32>(Obj->Flags) &
			static_cast<int32>(EObjectFlags::ClassDefaultObject)) != 0)
			return false;
		if (!IsSafeLiveObject(Obj))
			return false;
		if (!CurrentGI)
			return true;
		return Obj->Outer == static_cast<UObject*>(CurrentGI);
	};

	if (IsRuntimeSubsystem(Cached))
		return Cached;

	Cached = nullptr;

	// Primary path: query subsystem from current GI/world context.
	if (CurrentGI)
	{
		auto* SubObj = USubsystemBlueprintLibrary::GetGameInstanceSubsystem(
			static_cast<UObject*>(CurrentGI),
			UJHNeoUISubsystem::StaticClass());
		auto* Sub = static_cast<UJHNeoUISubsystem*>(SubObj);
		if (IsRuntimeSubsystem(Sub))
		{
			Cached = Sub;
			return Cached;
		}
	}

	if (!Cached)
	{
		if (UWorld* World = UWorld::GetWorld())
		{
			auto* SubObj = USubsystemBlueprintLibrary::GetGameInstanceSubsystem(
				static_cast<UObject*>(World),
				UJHNeoUISubsystem::StaticClass());
			auto* Sub = static_cast<UJHNeoUISubsystem*>(SubObj);
			if (IsRuntimeSubsystem(Sub))
			{
				Cached = Sub;
				return Cached;
			}
		}
	}

	// Fallback: scan live objects and pick runtime instance only.
	auto* ObjArray = UObject::GObjects.GetTypedPtr();
	if (ObjArray)
	{
		int32 Num = ObjArray->Num();
		for (int32 i = 0; i < Num; ++i)
		{
			UObject* Obj = ObjArray->GetByIndex(i);
			if (!Obj || !Obj->IsA(UJHNeoUISubsystem::StaticClass()))
				continue;

			auto* Candidate = static_cast<UJHNeoUISubsystem*>(Obj);
			if (IsRuntimeSubsystem(Candidate))
			{
				Cached = Candidate;
				break;
			}
		}
	}

	static DWORD sLastSubsystemNullLogTick = 0;
	const DWORD Now = GetTickCount();
	if (!Cached && (Now - sLastSubsystemNullLogTick > 1500))
	{
		sLastSubsystemNullLogTick = Now;
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: runtime JHNeoUISubsystem not found (CurrentGI="
		          << (void*)CurrentGI << ")\n";
	}
	return Cached;
}

static void HideCurrentItemTips()
{
	if (GItemHoverTipsWidget && IsSafeLiveObject(static_cast<UObject*>(GItemHoverTipsWidget)))
		GItemHoverTipsWidget->SetVisibility(ESlateVisibility::Collapsed);
	if (GStandaloneItemTipWidget && IsSafeLiveObject(static_cast<UObject*>(GStandaloneItemTipWidget)))
		GStandaloneItemTipWidget->SetVisibility(ESlateVisibility::Collapsed);

	GItemHoverTipsWidget = nullptr;
	GItemHoveredSlot = -1;

	if (GItemHoverTempSpec)
	{
		ClearGCRoot(static_cast<UObject*>(GItemHoverTempSpec));
		GItemHoverTempSpec = nullptr;
	}
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
	LOGI_STREAM("ItemBrowser") << "[SDK] Filter cat=" << category << ": " << GFilteredIndices.size()
	          << " items, " << GItemTotalPages << " pages\n";
}

// Refresh item grid to show the current page
void RefreshItemPage()
{
	if (GItemCurrentPage >= GItemTotalPages)
		GItemCurrentPage = GItemTotalPages - 1;
	if (GItemCurrentPage < 0)
		GItemCurrentPage = 0;

	const int32 StartIdx = GItemCurrentPage * ITEMS_PER_PAGE;
	for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
	{
		GItemSlotButtons[i] = nullptr;
		GItemSlotImages[i] = nullptr;
		GItemSlotQualityBorders[i] = nullptr;
		GItemSlotEntryWidgets[i] = nullptr;
		GItemSlotItemIndices[i] = -1;
		GItemSlotWasPressed[i] = false;
	}

	UListView* ListView = GItemListView;
	if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UListView::StaticClass()))
	{
		GItemListView = nullptr;
		GItemGridPanel = nullptr;
	}
	else
	{
		for (UItemInfoSpec* Spec : GCurrentPageSpecs)
		{
			if (Spec && IsSafeLiveObject(static_cast<UObject*>(Spec)))
				ClearGCRoot(static_cast<UObject*>(Spec));
		}
		GCurrentPageSpecs.clear();

		std::vector<int32> PageItemIndices;
		PageItemIndices.reserve(ITEMS_PER_PAGE);
		int32 BuiltSlots = 0;
		for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
		{
			const int32 FiltIdx = StartIdx + i;
			if (FiltIdx < 0 || FiltIdx >= static_cast<int32>(GFilteredIndices.size()))
				continue;

			const int32 ItemIdx = GFilteredIndices[FiltIdx];
			if (ItemIdx < 0 || ItemIdx >= static_cast<int32>(GAllItems.size()))
				continue;

			const CachedItem& CI = GAllItems[ItemIdx];
			UItemInfoSpec* Spec = UItemFuncLib::MakeItemInfoSpec(CI.DefId, 1, EItemRandPoolType::None);
			if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(Spec), UItemInfoSpec::StaticClass()))
				continue;

			MarkAsGCRoot(static_cast<UObject*>(Spec));
			GCurrentPageSpecs.push_back(Spec);
			PageItemIndices.push_back(ItemIdx);
			++BuiltSlots;
		}

		ListView->ClearListItems();
		for (UItemInfoSpec* Spec : GCurrentPageSpecs)
		{
			if (Spec && IsSafeLiveObject(static_cast<UObject*>(Spec)))
				ListView->AddItem(static_cast<UObject*>(Spec));
		}
		ListView->ScrollToTop();
		ListView->RequestRefresh();
		ListView->RegenerateAllEntries();
		if (IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UNeoUIListView::StaticClass()))
		{
			auto* NeoList = static_cast<UNeoUIListView*>(ListView);
			NeoList->EVT_InitOnce();
			UJHNeoUIUtilLib::NeoUIListRender(NeoList);
			ListView->RequestRefresh();
			ListView->RegenerateAllEntries();
		}

		const int32 NumItemsInList = ListView->GetNumItems();
		const TArray<UUserWidget*> Displayed = ListView->GetDisplayedEntryWidgets();
		const int32 DisplayedNum = Displayed.Num();
		const int32 MappedNum = static_cast<int32>(PageItemIndices.size());

		auto BindEntryToSlot = [&](int32 Slot, UUserWidget* EntryWidget, int32 ItemIdx) -> bool
		{
			if (Slot < 0 || Slot >= ITEMS_PER_PAGE)
				return false;
			if (!IsSafeLiveObject(static_cast<UObject*>(EntryWidget)))
				return false;
			if (ItemIdx < 0 || ItemIdx >= static_cast<int32>(GAllItems.size()))
				return false;

			GItemSlotItemIndices[Slot] = ItemIdx;
			GItemSlotEntryWidgets[Slot] = EntryWidget;
			GItemSlotWasPressed[Slot] = false;

			UJHGPCBtn_C* BtnJHItem = nullptr;
			UJHGPCBtn_ActiveBG_C* ActiveBG = nullptr;
			UJHItemDisplayElement* ItemDisplay = nullptr;
			if (EntryWidget->IsA(UBPEntry_Item_WDT_C::StaticClass()))
			{
				auto* Entry = static_cast<UBPEntry_Item_WDT_C*>(EntryWidget);
				BtnJHItem = Entry ? Entry->BTN_JHItem : nullptr;
				ActiveBG = Entry ? Entry->JHGPCBtn_ActiveBG : nullptr;
				ItemDisplay = Entry ? Entry->ItemDisplay : nullptr;
			}
			else if (EntryWidget->IsA(UBPEntry_Item_C::StaticClass()))
			{
				auto* Entry = static_cast<UBPEntry_Item_C*>(EntryWidget);
				BtnJHItem = Entry ? Entry->BTN_JHItem : nullptr;
				ActiveBG = Entry ? Entry->JHGPCBtn_ActiveBG : nullptr;
				ItemDisplay = Entry ? Entry->ItemDisplay : nullptr;
			}
			else
			{
				return false;
			}

			UImage* MainBorder = nullptr;
			if (ItemDisplay && ItemDisplay->CMP)
			{
				auto* Display = ItemDisplay->CMP;
				Display->SetVisibility(ESlateVisibility::HitTestInvisible);
				ItemDisplay->SetVisibility(ESlateVisibility::HitTestInvisible);

				if (Display->IMG_Item &&
					IsSafeLiveObjectOfClass(static_cast<UObject*>(Display->IMG_Item), UImage::StaticClass()))
					GItemSlotImages[Slot] = static_cast<UImage*>(Display->IMG_Item);

				if (Display->IMG_QualityBorder &&
					IsSafeLiveObjectOfClass(static_cast<UObject*>(Display->IMG_QualityBorder), UImage::StaticClass()))
					GItemSlotQualityBorders[Slot] = static_cast<UImage*>(Display->IMG_QualityBorder);

				if (Display->IMG_Border &&
					IsSafeLiveObjectOfClass(static_cast<UObject*>(Display->IMG_Border), UImage::StaticClass()))
					MainBorder = static_cast<UImage*>(Display->IMG_Border);

				if (Display->TXT_Count)
					Display->TXT_Count->SetVisibility(ESlateVisibility::Collapsed);
			}

			if (ActiveBG && IsSafeLiveObject(static_cast<UObject*>(ActiveBG)))
				ActiveBG->SetVisibility(ESlateVisibility::HitTestInvisible);

			if (BtnJHItem && IsSafeLiveObject(static_cast<UObject*>(BtnJHItem)))
			{
				if (ItemDisplay)
					BtnJHItem->RegisterInternalActiveDisplay(static_cast<UWidget*>(ItemDisplay));

				if (BtnJHItem->BtnMain &&
					IsSafeLiveObjectOfClass(static_cast<UObject*>(BtnJHItem->BtnMain), UButton::StaticClass()))
					GItemSlotButtons[Slot] = static_cast<UButton*>(BtnJHItem->BtnMain);
			}

			const CachedItem& CI = GAllItems[ItemIdx];
			if (GItemSlotButtons[Slot])
			{
				wchar_t Tip[96] = {};
				swprintf_s(Tip, 96, L"%s [%d]", CI.Name, CI.DefId);
				GItemSlotButtons[Slot]->SetToolTipText(MakeText(Tip));
				GItemSlotButtons[Slot]->SetIsEnabled(true);
			}

			if (GItemSlotImages[Slot])
			{
				if (CI.HasIcon)
				{
					UTexture2D* IconTex = ResolveTextureFromSoftData(CI.IconData);
					if (IconTex &&
						IsSafeLiveObjectOfClass(static_cast<UObject*>(IconTex), UTexture2D::StaticClass()))
					{
						GItemSlotImages[Slot]->SetBrushFromTexture(IconTex, true);
						GItemSlotImages[Slot]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
					else
					{
						GItemSlotImages[Slot]->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
				else
				{
					GItemSlotImages[Slot]->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			const int32 SafeQuality = SanitizeItemQuality(CI.Quality);
			if (MainBorder)
			{
				const FLinearColor MainBorderColor =
					UItemFuncLib::GetColorByQuality(static_cast<EItemQuality>(SafeQuality));
				MainBorder->SetColorAndOpacity(MainBorderColor);
				MainBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}

			if (GItemSlotQualityBorders[Slot])
			{
				UTexture2D* QTex = GetItemQualityBorderTexture(static_cast<uint8>(SafeQuality));
				if (QTex &&
					IsSafeLiveObjectOfClass(static_cast<UObject*>(QTex), UTexture2D::StaticClass()))
				{
					GItemSlotQualityBorders[Slot]->SetBrushFromTexture(QTex, true);
					GItemSlotQualityBorders[Slot]->SetColorAndOpacity(FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f });
					GItemSlotQualityBorders[Slot]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
				else
				{
					GItemSlotQualityBorders[Slot]->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			return true;
		};

		int32 SlotCount = 0;
		const int32 DisplayMapCount = (DisplayedNum < MappedNum) ? DisplayedNum : MappedNum;
		for (int32 i = 0; i < DisplayMapCount && i < ITEMS_PER_PAGE; ++i)
		{
			if (BindEntryToSlot(i, Displayed[i], PageItemIndices[i]))
				++SlotCount;
		}

		int32 ScanMapped = 0;
		int32 ScanFound = 0;
		int32 ScanInGrid = 0;
		int32 OrderMapped = 0;
		if (SlotCount < MappedNum)
		{
			std::unordered_map<UObject*, int32> SpecToSlot;
			SpecToSlot.reserve(GCurrentPageSpecs.size());
			for (int32 i = 0; i < static_cast<int32>(GCurrentPageSpecs.size()) && i < ITEMS_PER_PAGE; ++i)
			{
				UItemInfoSpec* Spec = GCurrentPageSpecs[static_cast<size_t>(i)];
				if (Spec && IsSafeLiveObject(static_cast<UObject*>(Spec)))
					SpecToSlot.emplace(static_cast<UObject*>(Spec), i);
			}

			UObject* WorldCtx = static_cast<UObject*>(UWorld::GetWorld());
			if (!WorldCtx)
				WorldCtx = static_cast<UObject*>(GetCurrentGameInstance());
			if (WorldCtx && !SpecToSlot.empty())
			{
				struct ViewRect
				{
					float MinX = 0.0f;
					float MaxX = 0.0f;
					float MinY = 0.0f;
					float MaxY = 0.0f;
					bool Valid = false;
				};
				auto TryGetViewportRect = [&](UWidget* Widget, ViewRect& Out) -> bool
				{
					if (!Widget || !IsSafeLiveObject(static_cast<UObject*>(Widget)))
						return false;

					const FGeometry Geo = Widget->GetCachedGeometry();
					const FVector2D Size = USlateBlueprintLibrary::GetLocalSize(Geo);
					if (Size.X <= 1.0f || Size.Y <= 1.0f)
						return false;

					FVector2D PixelTL{}, ViewTL{}, PixelBR{}, ViewBR{};
					USlateBlueprintLibrary::LocalToViewport(
						WorldCtx, Geo, FVector2D{ 0.0f, 0.0f }, &PixelTL, &ViewTL);
					USlateBlueprintLibrary::LocalToViewport(
						WorldCtx, Geo, Size, &PixelBR, &ViewBR);

					Out.MinX = (ViewTL.X < ViewBR.X) ? ViewTL.X : ViewBR.X;
					Out.MaxX = (ViewTL.X > ViewBR.X) ? ViewTL.X : ViewBR.X;
					Out.MinY = (ViewTL.Y < ViewBR.Y) ? ViewTL.Y : ViewBR.Y;
					Out.MaxY = (ViewTL.Y > ViewBR.Y) ? ViewTL.Y : ViewBR.Y;
					Out.Valid = true;
					return true;
				};
				auto IntersectsRect = [](const ViewRect& A, const ViewRect& B) -> bool
				{
					if (!A.Valid || !B.Valid)
						return false;
					if (A.MaxX < B.MinX || B.MaxX < A.MinX)
						return false;
					if (A.MaxY < B.MinY || B.MaxY < A.MinY)
						return false;
					return true;
				};

				ViewRect GridRect{};
				const bool HasGridRect = TryGetViewportRect(static_cast<UWidget*>(GItemGridPanel), GridRect);

				TArray<UUserWidget*> FoundWdtEntries;
				TArray<UUserWidget*> FoundLegacyEntries;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
					WorldCtx, &FoundWdtEntries, UBPEntry_Item_WDT_C::StaticClass(), false);
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
					WorldCtx, &FoundLegacyEntries, UBPEntry_Item_C::StaticClass(), false);
				ScanFound = FoundWdtEntries.Num() + FoundLegacyEntries.Num();

				bool SlotUsed[ITEMS_PER_PAGE] = {};
				for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
					SlotUsed[i] = (GItemSlotEntryWidgets[i] != nullptr);

				std::unordered_set<uintptr_t> SeenEntries;
				std::vector<UUserWidget*> CandidateEntries;
				CandidateEntries.reserve(static_cast<size_t>(ScanFound));
				auto PushCandidate = [&](UUserWidget* W)
				{
					if (!IsSafeLiveObject(static_cast<UObject*>(W)))
						return;

					const uintptr_t Key = reinterpret_cast<uintptr_t>(W);
					if (SeenEntries.find(Key) != SeenEntries.end())
						return;
					SeenEntries.insert(Key);

					if (HasGridRect)
					{
						ViewRect EntryRect{};
						if (!TryGetViewportRect(static_cast<UWidget*>(W), EntryRect))
							return;
						if (!IntersectsRect(GridRect, EntryRect))
							return;
					}

					CandidateEntries.push_back(W);
				};
				for (UUserWidget* W : FoundWdtEntries) PushCandidate(W);
				for (UUserWidget* W : FoundLegacyEntries) PushCandidate(W);
				ScanInGrid = static_cast<int32>(CandidateEntries.size());

				for (UUserWidget* Entry : CandidateEntries)
				{
					if (!Entry->IsA(UNeoUIReusableVisualEntry::StaticClass()))
						continue;

					auto* Reusable = static_cast<UNeoUIReusableVisualEntry*>(Entry);
					UObject* BoundObj = Reusable ? Reusable->GetBindedDataObj() : nullptr;
					auto It = SpecToSlot.find(BoundObj);
					if (It == SpecToSlot.end())
						continue;

					const int32 Slot = It->second;
					if (Slot < 0 || Slot >= ITEMS_PER_PAGE || SlotUsed[Slot])
						continue;

					const int32 ItemIdx = PageItemIndices[static_cast<size_t>(Slot)];
					if (BindEntryToSlot(Slot, Entry, ItemIdx))
					{
						SlotUsed[Slot] = true;
						++ScanMapped;
					}
				}

				// 兜底：按界面位置(先Y后X)强行映射到当前页槽位，至少保证图标/tooltip/点击一致。
				if (ScanMapped < MappedNum && !CandidateEntries.empty())
				{
					struct EntryPos
					{
						UUserWidget* Entry = nullptr;
						float X = 0.0f;
						float Y = 0.0f;
					};
					std::vector<EntryPos> Ordered;
					Ordered.reserve(CandidateEntries.size());

					for (UUserWidget* Entry : CandidateEntries)
					{
						ViewRect EntryRect{};
						if (!TryGetViewportRect(static_cast<UWidget*>(Entry), EntryRect))
							continue;
						if (HasGridRect && !IntersectsRect(GridRect, EntryRect))
							continue;

						EntryPos P{};
						P.Entry = Entry;
						P.X = EntryRect.MinX;
						P.Y = EntryRect.MinY;
						Ordered.push_back(P);
					}

					std::stable_sort(
						Ordered.begin(), Ordered.end(),
						[](const EntryPos& A, const EntryPos& B)
						{
							const float dy = A.Y - B.Y;
							if (dy > 1.5f || dy < -1.5f)
								return A.Y < B.Y;
							return A.X < B.X;
						});

					int32 OrderIdx = 0;
					for (const EntryPos& P : Ordered)
					{
						while (OrderIdx < MappedNum && OrderIdx < ITEMS_PER_PAGE && SlotUsed[OrderIdx])
							++OrderIdx;
						if (OrderIdx >= MappedNum || OrderIdx >= ITEMS_PER_PAGE)
							break;

						const int32 ItemIdx = PageItemIndices[static_cast<size_t>(OrderIdx)];
						if (BindEntryToSlot(OrderIdx, P.Entry, ItemIdx))
						{
							SlotUsed[OrderIdx] = true;
							++OrderMapped;
						}
						++OrderIdx;
					}
				}
			}
		}

		LOGI_STREAM("ItemBrowser")
			<< "[SDK] ItemGrid refresh: page=" << (GItemCurrentPage + 1)
			<< "/" << GItemTotalPages
			<< " dataSlots=" << BuiltSlots
			<< " listItems=" << NumItemsInList
			<< " displayedEntries=" << DisplayedNum
			<< " mappedEntries=" << (SlotCount + ScanMapped + OrderMapped)
			<< " scanFound=" << ScanFound
			<< " scanInGrid=" << ScanInGrid
			<< " scanMapped=" << ScanMapped
			<< " orderMapped=" << OrderMapped << "\n";
	}

	if (GItemPageLabel)
	{
		wchar_t Buf[16] = {};
		swprintf_s(Buf, 16, L"%d/%d", GItemCurrentPage + 1, GItemTotalPages);
		GItemPageLabel->SetText(MakeText(Buf));
	}
	auto SetBtnEnabled = [](UJHCommon_Btn_Free_C* W, bool bEnabled) {
		if (!W) return;
		W->SetIsEnabled(bEnabled);
		if (bEnabled)
			W->GotoNormalStatus();
		else
			W->GotoDisableStatus();
		if (W->Btn)
			W->Btn->SetIsEnabled(bEnabled);
		if (W->JHGPCBtn)
			W->JHGPCBtn->SetIsEnabled(bEnabled);
	};
	SetBtnEnabled(GItemPrevPageBtn, GItemCurrentPage > 0);
	SetBtnEnabled(GItemNextPageBtn, (GItemCurrentPage + 1) < GItemTotalPages);
}

void PollItemBrowserHoverTips()
{
	static int32 sLastProbeSlot = -1;
	static int32 sLastFailCode = -1;
	static DWORD sLastHeartbeatTick = 0;
	static DWORD sLastHoverSeenTick = 0;
	static int32 sPendingHoverSlot = -1;
	static DWORD sPendingHoverTick = 0;

	const DWORD NowTick = GetTickCount();
	const bool Heartbeat = kVerboseItemHoverLogs && (NowTick - sLastHeartbeatTick >= 1000);
	if (Heartbeat)
		sLastHeartbeatTick = NowTick;

	int32 ValidBtnCount = 0;
	int32 ValidEntryCount = 0;
	int32 CandidateSlotCount = 0;
	int32 EnabledSlotCount = 0;
	int32 HoverByBtnCount = 0;
	int32 HoverByEntryCount = 0;
	int32 HoverByGpcWrapperCount = 0;
	int32 HoverByGpcMainCount = 0;
	int32 HoverByDisplayElemCount = 0;
	int32 HoverByDisplayCmpCount = 0;
	int32 HoverByDisplayCanvasCount = 0;
	int32 HoverByDisplayImageCount = 0;
	int32 HoverByGridFallbackCount = 0;

	int32 HoveredSlot = -1;
	// 优先使用网格坐标命中，避免每轮都做 24 格深度 IsHovered 探测。
	if (IsSafeLiveObject(static_cast<UObject*>(GItemGridPanel)))
	{
		UObject* WorldCtx = nullptr;
		if (UWorld* World = UWorld::GetWorld())
			WorldCtx = static_cast<UObject*>(World);

		if (WorldCtx)
		{
			const FGeometry GridGeo = GItemGridPanel->GetCachedGeometry();
			const FVector2D GridSize = USlateBlueprintLibrary::GetLocalSize(GridGeo);
			if (GridSize.X > 1.0f && GridSize.Y > 1.0f)
			{
				FVector2D PixelTL{}, ViewTL{}, PixelBR{}, ViewBR{};
				USlateBlueprintLibrary::LocalToViewport(WorldCtx, GridGeo, FVector2D{ 0.0f, 0.0f }, &PixelTL, &ViewTL);
				USlateBlueprintLibrary::LocalToViewport(WorldCtx, GridGeo, GridSize, &PixelBR, &ViewBR);

				const float MinX = (ViewTL.X < ViewBR.X) ? ViewTL.X : ViewBR.X;
				const float MaxX = (ViewTL.X > ViewBR.X) ? ViewTL.X : ViewBR.X;
				const float MinY = (ViewTL.Y < ViewBR.Y) ? ViewTL.Y : ViewBR.Y;
				const float MaxY = (ViewTL.Y > ViewBR.Y) ? ViewTL.Y : ViewBR.Y;

				const FVector2D MouseVP = UWidgetLayoutLibrary::GetMousePositionOnViewport(WorldCtx);
				const bool InGrid =
					(MouseVP.X >= MinX && MouseVP.X <= MaxX &&
					 MouseVP.Y >= MinY && MouseVP.Y <= MaxY);
				if (InGrid)
				{
					const float GridW = MaxX - MinX;
					const float GridH = MaxY - MinY;
					if (GridW > 1.0f && GridH > 1.0f)
					{
						const float CellW = GridW / static_cast<float>(ITEM_GRID_COLS);
						const float CellH = GridH / static_cast<float>(ITEM_GRID_ROWS);
						const int32 Col = static_cast<int32>((MouseVP.X - MinX) / CellW);
						const int32 Row = static_cast<int32>((MouseVP.Y - MinY) / CellH);
						if (Col >= 0 && Col < ITEM_GRID_COLS && Row >= 0 && Row < ITEM_GRID_ROWS)
						{
							const int32 Slot = Row * ITEM_GRID_COLS + Col;
							if (Slot >= 0 && Slot < ITEMS_PER_PAGE)
							{
								const int32 ItemIdxFallback = GItemSlotItemIndices[Slot];
								if (ItemIdxFallback >= 0 && ItemIdxFallback < static_cast<int32>(GAllItems.size()))
								{
									HoveredSlot = Slot;
									++HoverByGridFallbackCount;
									if (kVerboseItemHoverLogs && Heartbeat)
									{
										LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverGridFallback: mouseVP=("
											<< MouseVP.X << "," << MouseVP.Y
											<< ") gridViewTL=(" << MinX << "," << MinY
											<< ") gridViewBR=(" << MaxX << "," << MaxY
											<< ") slot=" << Slot
											<< " itemIdx=" << ItemIdxFallback << "\n";
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// 网格命中失败时，低频走深度探测兜底，兼容特殊布局。
	if (HoveredSlot < 0)
	{
		static DWORD sLastDeepProbeTick = 0;
		if ((NowTick - sLastDeepProbeTick) >= 80)
		{
			sLastDeepProbeTick = NowTick;
			for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
			{
				auto* Btn = GItemSlotButtons[i];
				auto* Entry = GItemSlotEntryWidgets[i];
				const bool BtnValid = IsSafeLiveObject(static_cast<UObject*>(Btn));
				const bool EntryValid = IsSafeLiveObject(static_cast<UObject*>(Entry));
				if (BtnValid) ++ValidBtnCount;
				if (EntryValid) ++ValidEntryCount;
				if (!BtnValid && !EntryValid)
					continue;
				++CandidateSlotCount;

				bool bEnabled = true;
				if (BtnValid)
					bEnabled = Btn->GetIsEnabled();
				if (!bEnabled)
					continue;
				++EnabledSlotCount;

				bool bHovered = false;
				if (BtnValid)
				{
					bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(Btn));
					if (bHovered) ++HoverByBtnCount;
				}
				if (!bHovered && EntryValid)
				{
					bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(Entry));
					if (bHovered) ++HoverByEntryCount;
				}
				if (!bHovered && EntryValid &&
					(Entry->IsA(UBPEntry_Item_C::StaticClass()) || Entry->IsA(UBPEntry_Item_WDT_C::StaticClass())))
				{
					UJHGPCBtn_C* GpcBtn = nullptr;
					UJHItemDisplayElement* ItemDisplay = nullptr;
					if (Entry->IsA(UBPEntry_Item_WDT_C::StaticClass()))
					{
						auto* EntryWdt = static_cast<UBPEntry_Item_WDT_C*>(Entry);
						GpcBtn = EntryWdt ? EntryWdt->BTN_JHItem : nullptr;
						ItemDisplay = EntryWdt ? EntryWdt->ItemDisplay : nullptr;
					}
					else
					{
						auto* EntryBP = static_cast<UBPEntry_Item_C*>(Entry);
						GpcBtn = EntryBP ? EntryBP->BTN_JHItem : nullptr;
						ItemDisplay = EntryBP ? EntryBP->ItemDisplay : nullptr;
					}

					if (IsSafeLiveObject(static_cast<UObject*>(GpcBtn)))
					{
						bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(GpcBtn));
						if (bHovered) ++HoverByGpcWrapperCount;

						if (!bHovered && IsSafeLiveObject(static_cast<UObject*>(GpcBtn->BtnMain)))
						{
							bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(GpcBtn->BtnMain));
							if (bHovered) ++HoverByGpcMainCount;
						}
					}

					if (!bHovered && IsSafeLiveObject(static_cast<UObject*>(ItemDisplay)))
					{
						bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(ItemDisplay));
						if (bHovered) ++HoverByDisplayElemCount;
					}

					if (!bHovered && ItemDisplay &&
						IsSafeLiveObject(static_cast<UObject*>(ItemDisplay->CMP)))
					{
						auto* DisplayCmp = ItemDisplay->CMP;
						bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(DisplayCmp));
						if (bHovered) ++HoverByDisplayCmpCount;

						if (!bHovered && IsSafeLiveObject(static_cast<UObject*>(DisplayCmp->CT_Main)))
						{
							bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(DisplayCmp->CT_Main));
							if (bHovered) ++HoverByDisplayCanvasCount;
						}

						if (!bHovered && IsSafeLiveObject(static_cast<UObject*>(DisplayCmp->IMG_SolidBG)))
						{
							bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(DisplayCmp->IMG_SolidBG));
							if (bHovered) ++HoverByDisplayImageCount;
						}
						if (!bHovered && IsSafeLiveObject(static_cast<UObject*>(DisplayCmp->IMG_Item)))
						{
							bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(DisplayCmp->IMG_Item));
							if (bHovered) ++HoverByDisplayImageCount;
						}
						if (!bHovered && IsSafeLiveObject(static_cast<UObject*>(DisplayCmp->IMG_Border)))
						{
							bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(DisplayCmp->IMG_Border));
							if (bHovered) ++HoverByDisplayImageCount;
						}
						if (!bHovered && IsSafeLiveObject(static_cast<UObject*>(DisplayCmp->IMG_QualityBorder)))
						{
							bHovered = IsWidgetHoveredWithGeometry(static_cast<UWidget*>(DisplayCmp->IMG_QualityBorder));
							if (bHovered) ++HoverByDisplayImageCount;
						}
					}
				}

				if (bHovered)
				{
					HoveredSlot = i;
					break;
				}
			}
		}
	}

	if (HoveredSlot >= 0)
		sLastHoverSeenTick = NowTick;

	if (HoveredSlot < 0)
	{
		sPendingHoverSlot = -1;
		sPendingHoverTick = 0;

		if (kVerboseItemHoverLogs && sLastProbeSlot >= 0)
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: cleared\n";
		sLastProbeSlot = -1;
		sLastFailCode = -1;
		HideCurrentItemTips();
		return;
	}

	// 悬浮切换稳态判定：鼠标快速扫过格子时，不要每帧都重建 Tip 内容。
	const bool HoverTargetChanged = (HoveredSlot != GItemHoveredSlot);
	if (HoverTargetChanged)
	{
		if (sPendingHoverSlot != HoveredSlot)
		{
			sPendingHoverSlot = HoveredSlot;
			sPendingHoverTick = NowTick;
			return;
		}

		const DWORD HoverSettleMs = 28;
		if ((NowTick - sPendingHoverTick) < HoverSettleMs)
			return;
	}
	else
	{
		sPendingHoverSlot = -1;
		sPendingHoverTick = 0;
	}

	const int32 ItemIdx = GItemSlotItemIndices[HoveredSlot];
	if (HoveredSlot != sLastProbeSlot)
	{
		if (kVerboseItemHoverLogs)
		{
			int32 DefId = -1;
			if (ItemIdx >= 0 && ItemIdx < static_cast<int32>(GAllItems.size()))
				DefId = GAllItems[ItemIdx].DefId;
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: slot=" << HoveredSlot
			          << " itemIdx=" << ItemIdx
			          << " defId=" << DefId << "\n";
		}
		sLastProbeSlot = HoveredSlot;
	}

	if (ItemIdx < 0 || ItemIdx >= static_cast<int32>(GAllItems.size()))
	{
		if (sLastFailCode != 1)
		{
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: invalid item index\n";
			sLastFailCode = 1;
		}
		HideCurrentItemTips();
		return;
	}

	// Hover target changed: build a fresh game-native tips widget.
	const bool NeedRebuildTips =
		(HoveredSlot != GItemHoveredSlot) ||
		(!GItemHoverTipsWidget) ||
		(!IsSafeLiveObject(static_cast<UObject*>(GItemHoverTipsWidget)));
	static DWORD sLastTipRebuildTick = 0;

	if (NeedRebuildTips)
	{
		const CachedItem& CI = GAllItems[ItemIdx];
		const bool HasReusableStandalone = IsSafeLiveObject(static_cast<UObject*>(GStandaloneItemTipWidget));
		const bool RebuildTooFrequent = HasReusableStandalone && (NowTick - sLastTipRebuildTick < 40);
		if (RebuildTooFrequent)
			GItemHoverTipsWidget = static_cast<UJHNeoUITipsVEBase*>(GStandaloneItemTipWidget);
		else
			GItemHoverTipsWidget = nullptr;
		// 仅使用自建 StandaloneGameTip，不再调用游戏原生 Tip VM 接口。
		if (!GItemHoverTipsWidget || !IsSafeLiveObject(static_cast<UObject*>(GItemHoverTipsWidget)))
		{
			const bool StandaloneOK = UpdateStandaloneItemTipContent(CI);
			if (StandaloneOK && IsSafeLiveObject(static_cast<UObject*>(GStandaloneItemTipWidget)))
			{
				GItemHoverTipsWidget = static_cast<UJHNeoUITipsVEBase*>(GStandaloneItemTipWidget);
				sLastTipRebuildTick = NowTick;
				if (kVerboseItemHoverLogs)
				{
					LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: tip source=StandaloneGameTip inViewport="
					          << (GStandaloneItemTipWidget->IsInViewport() ? 1 : 0) << "\n";
				}
			}
		}

		if (!GItemHoverTipsWidget || !IsSafeLiveObject(static_cast<UObject*>(GItemHoverTipsWidget)))
		{
			if (sLastFailCode != 5)
			{
				LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: StandaloneGameTip failed defId=" << CI.DefId << "\n";
				sLastFailCode = 5;
			}
			HideCurrentItemTips();
			return;
		}

		GItemHoveredSlot = HoveredSlot;
		sLastFailCode = -1;
		if (kVerboseItemHoverLogs)
		{
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverProbe: tips created widget=" << (void*)GItemHoverTipsWidget
			          << " defId=" << CI.DefId << "\n";
		}
	}

	// Keep the tips anchored to the currently hovered backpack entry widget.
	auto* Anchor = GItemSlotEntryWidgets[HoveredSlot];
	const bool IsStandaloneTip =
		(GStandaloneItemTipWidget &&
		 GItemHoverTipsWidget == static_cast<UJHNeoUITipsVEBase*>(GStandaloneItemTipWidget));
	if (!IsStandaloneTip && IsSafeLiveObject(static_cast<UObject*>(Anchor)))
	{
		if (auto* Subsystem = GetJHNeoUISubsystem())
			Subsystem->UpdateItemTipsPosition(Anchor, GItemHoverTipsWidget, false);
	}

	if (UWorld* World = UWorld::GetWorld())
	{
		UObject* ViewportContext = static_cast<UObject*>(World);
		APlayerController* PC = nullptr;
		PC = UGameplayStatics::GetPlayerController(World, 0);

		FVector2D MouseVP{};
		bool GotMouseVP = false;
		if (PC)
		{
			float MouseX = 0.0f;
			float MouseY = 0.0f;
			GotMouseVP = UWidgetLayoutLibrary::GetMousePositionScaledByDPI(PC, &MouseX, &MouseY);
			if (GotMouseVP)
				MouseVP = FVector2D{ MouseX, MouseY };
		}
		if (!GotMouseVP)
			MouseVP = UWidgetLayoutLibrary::GetMousePositionOnViewport(ViewportContext);

		FVector2D TipSize{};
		if (IsStandaloneTip)
		{
			// UBPVE_JHTips_Item_C 根可能是整屏容器，Standalone 固定用卡片尺寸参与定位。
			TipSize = FVector2D{ kItemTipDefaultWidth, kItemTipDefaultHeight };
		}
		else
		{
			TipSize = USlateBlueprintLibrary::GetLocalSize(GItemHoverTipsWidget->GetCachedGeometry());
			if (TipSize.X < 1.0f || TipSize.Y < 1.0f)
				TipSize = FVector2D{ kItemTipDefaultWidth, kItemTipDefaultHeight };
		}

		FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(ViewportContext);
		if (ViewportSize.X < 64.0f || ViewportSize.Y < 64.0f)
		{
			if (UWorld* World = UWorld::GetWorld())
				ViewportSize = UWidgetLayoutLibrary::GetViewportSize(static_cast<UObject*>(World));
		}
		if (ViewportSize.X < 64.0f || ViewportSize.Y < 64.0f)
			ViewportSize = FVector2D{ 1920.0f, 1080.0f };

		const float VPScale = UWidgetLayoutLibrary::GetViewportScale(ViewportContext);
		if (VPScale > 0.01f)
		{
			ViewportSize.X /= VPScale;
			ViewportSize.Y /= VPScale;
		}

		const bool MouseOutOfViewportRange =
			(MouseVP.X < -64.0f || MouseVP.Y < -64.0f ||
			 MouseVP.X > (ViewportSize.X + 64.0f) ||
			 MouseVP.Y > (ViewportSize.Y + 64.0f));
		if (MouseOutOfViewportRange && IsSafeLiveObject(static_cast<UObject*>(Anchor)))
		{
			const FGeometry AnchorGeo = Anchor->GetCachedGeometry();
			const FVector2D AnchorSize = USlateBlueprintLibrary::GetLocalSize(AnchorGeo);
			if (AnchorSize.X > 1.0f && AnchorSize.Y > 1.0f)
			{
				FVector2D PixelTL{}, ViewTL{}, PixelBR{}, ViewBR{};
				USlateBlueprintLibrary::LocalToViewport(
					ViewportContext, AnchorGeo, FVector2D{ 0.0f, 0.0f }, &PixelTL, &ViewTL);
				USlateBlueprintLibrary::LocalToViewport(
					ViewportContext, AnchorGeo, AnchorSize, &PixelBR, &ViewBR);
				MouseVP.X = (ViewTL.X + ViewBR.X) * 0.5f;
				MouseVP.Y = (ViewTL.Y + ViewBR.Y) * 0.5f;
			}
		}

		float TipX = MouseVP.X + kItemTipMouseOffset;
		float TipY = MouseVP.Y + kItemTipMouseOffset;

		const FVector2D TipPos{ TipX, TipY };
		static FVector2D sLastTipPos{ -99999.0f, -99999.0f };
		static DWORD sLastTipPosTick = 0;
		const bool NeedMove =
			(std::fabs(TipPos.X - sLastTipPos.X) > 1.0f) ||
			(std::fabs(TipPos.Y - sLastTipPos.Y) > 1.0f) ||
			((NowTick - sLastTipPosTick) >= 66);
		if (NeedMove)
		{
			GItemHoverTipsWidget->SetAlignmentInViewport(FVector2D{ 0.0f, 0.0f });
			GItemHoverTipsWidget->SetPositionInViewport(TipPos, false);
			sLastTipPos = TipPos;
			sLastTipPosTick = NowTick;
		}
	}

	GItemHoverTipsWidget->SetRenderOpacity(1.0f);
	GItemHoverTipsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	static DWORD sLastTipsStateLogTick = 0;
	if (kVerboseItemHoverLogs && (NowTick - sLastTipsStateLogTick >= 1000))
	{
		sLastTipsStateLogTick = NowTick;
		const FGeometry TipsGeo = GItemHoverTipsWidget->GetCachedGeometry();
		const FVector2D TipsSize = USlateBlueprintLibrary::GetLocalSize(TipsGeo);
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemHoverTipsState: widget=" << (void*)GItemHoverTipsWidget
		          << " inViewport=" << (GItemHoverTipsWidget->IsInViewport() ? 1 : 0)
		          << " vis=" << VisName(GItemHoverTipsWidget->GetVisibility())
		          << " opacity=" << GItemHoverTipsWidget->GetRenderOpacity()
		          << " size=(" << TipsSize.X << "," << TipsSize.Y << ")\n";
	}
}

// Clear item browser widget state (called when panel closes)
void ClearItemBrowserState()
{
	for (UItemInfoSpec* Spec : GCurrentPageSpecs)
	{
		if (Spec && IsSafeLiveObject(static_cast<UObject*>(Spec)))
			ClearGCRoot(static_cast<UObject*>(Spec));
	}
	GCurrentPageSpecs.clear();

	HideCurrentItemTips();
	if (GStandaloneItemTipWidget && IsSafeLiveObject(static_cast<UObject*>(GStandaloneItemTipWidget)))
	{
		GStandaloneItemTipWidget->RemoveFromParent();
		ClearGCRoot(static_cast<UObject*>(GStandaloneItemTipWidget));
	}
	GStandaloneItemTipWidget = nullptr;

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
	GItemListView = nullptr;
	for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
	{
		GItemSlotButtons[i] = nullptr;
		GItemSlotImages[i] = nullptr;
		GItemSlotQualityBorders[i] = nullptr;
		GItemSlotEntryWidgets[i] = nullptr;
		GItemSlotItemIndices[i] = -1;
		GItemSlotWasPressed[i] = false;
	}

	GItemCurrentPage = 0;
	GItemTotalPages = 0;
}

