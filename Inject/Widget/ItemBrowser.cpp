#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cwctype>
#include <string>
#include <unordered_map>

#include "ItemBrowser.hpp"
#include "GCManager.hpp"
#include "WidgetUtils.hpp"
#include "SDK/BPEntry_Item_classes.hpp"
#include "SDK/BPEntry_Item_WDT_classes.hpp"
#include "SDK/BP_ItemGridWDT_classes.hpp"
#include "SDK/BPVE_JHTips_Item_classes.hpp"
#include "SDK/BPVE_TipsBlock_classes.hpp"
#include "Logging.hpp"

namespace
{
	UBPVE_JHTips_Item_C* GStandaloneItemTipWidget = nullptr;
	std::vector<UItemInfoSpec*> GCurrentPageSpecs;
	constexpr int32 kItemTipZOrder = 20050;
	constexpr bool kVerboseItemHoverLogs = false;
	constexpr bool kVerboseItemFeedLogs = false;
	constexpr float kItemTipDefaultWidth = 520.0f;
	constexpr float kItemTipDefaultHeight = 420.0f;
	constexpr float kItemTipMouseOffset = 18.0f;
	constexpr float kItemTipViewportMargin = 12.0f;
	UEditableTextBox* GItemSearchEdit = nullptr;
	std::wstring GItemSearchKeyword;
	std::wstring GItemSearchKeywordFolded;

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

	std::wstring FoldSearchText(const std::wstring& In)
	{
		std::wstring Out;
		Out.reserve(In.size());
		for (wchar_t Ch : In)
			Out.push_back(static_cast<wchar_t>(std::towlower(Ch)));
		return Out;
	}

	bool ContainsSearchInsensitive(const wchar_t* Haystack, const std::wstring& NeedleFolded)
	{
		if (!Haystack || NeedleFolded.empty())
			return true;
		std::wstring Folded = FoldSearchText(std::wstring(Haystack));
		return Folded.find(NeedleFolded) != std::wstring::npos;
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

	constexpr uintptr_t kNeoTileInitWeakOffsetA = 0x388;
	constexpr uintptr_t kNeoTileInitWeakOffsetB = 0x8A0;
	constexpr int32 kGridSearchAnchorIndex = 59720;
	constexpr int32 kGridSearchWindow = 1000;
	constexpr bool kEnableEntryInitSearch = true;    // Enable anchor-window search path.
	constexpr bool kEnableFullScanFallback = false;  // 澶囩敤鍏ㄩ噺鎵紝榛樿鍏抽棴

	FWeakObjectPtr ReadWeakPtrAt(UObject* Obj, uintptr_t Offset)
	{
		FWeakObjectPtr Out{};
		if (!Obj)
			return Out;
		const uint8* Base = reinterpret_cast<const uint8*>(Obj);
		Out = *reinterpret_cast<const FWeakObjectPtr*>(Base + Offset);
		return Out;
	}

	void WriteWeakPtrAt(UObject* Obj, uintptr_t Offset, const FWeakObjectPtr& Value)
	{
		if (!Obj)
			return;
		uint8* Base = reinterpret_cast<uint8*>(Obj);
		*reinterpret_cast<FWeakObjectPtr*>(Base + Offset) = Value;
	}

	bool IsWeakPtrFilled(const FWeakObjectPtr& Weak)
	{
		return Weak.ObjectIndex >= 0 && Weak.ObjectSerialNumber > 0;
	}

	UObject* ResolveWeakPtrLoose(const FWeakObjectPtr& Weak)
	{
		if (!IsWeakPtrFilled(Weak))
			return nullptr;
		UObject* Obj = Weak.Get();
		if (!Obj)
			return nullptr;
		if (!IsSafeLiveObject(static_cast<UObject*>(Obj)))
			return nullptr;
		if (Obj->IsDefaultObject())
			return nullptr;
		return Obj;
	}

FWeakObjectPtr GCachedEntryInitWeakB{};
bool GHasCachedEntryInitWeakB = false;
UListView* GEntryInitPreparedListView = nullptr;

	void CacheEntryInitWeakB(const FWeakObjectPtr& WeakB)
	{
		if (!IsWeakPtrFilled(WeakB))
			return;
		GCachedEntryInitWeakB = WeakB;
		GHasCachedEntryInitWeakB = true;
	}

	bool HasUsableEntryInitWeakContext(UObject* GridObj)
	{
		if (!GridObj)
			return false;
		const FWeakObjectPtr B = ReadWeakPtrAt(GridObj, kNeoTileInitWeakOffsetB);
		return IsWeakPtrFilled(B);
	}

	bool IsSupportedItemGridObject(UObject* Obj)
	{
		if (!Obj)
			return false;
		if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemGrid::StaticClass()))
			return true;
		if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemGridWDT::StaticClass()))
			return true;
		if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemGrid_mobile::StaticClass()))
			return true;
		if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemGridWDT_mobile::StaticClass()))
			return true;
		return false;
	}

	void LogEntryInitWeakContext(const char* Stage, UObject* GridObj)
	{
		if (!GridObj)
			return;
		const FWeakObjectPtr A = ReadWeakPtrAt(GridObj, kNeoTileInitWeakOffsetA);
		const FWeakObjectPtr B = ReadWeakPtrAt(GridObj, kNeoTileInitWeakOffsetB);
		UObject* AO = ResolveWeakPtrLoose(A);
		UObject* BO = ResolveWeakPtrLoose(B);
		LOGI_STREAM("ItemBrowser")
			<< "[SDK] ItemGrid entry-init context(" << Stage << "): self=0x"
			<< std::hex << reinterpret_cast<uintptr_t>(GridObj)
			<< std::dec
			<< " A=(" << A.ObjectIndex << "," << A.ObjectSerialNumber << ")"
			<< " B=(" << B.ObjectIndex << "," << B.ObjectSerialNumber << ")"
			<< " AObj=0x" << std::hex << reinterpret_cast<uintptr_t>(AO)
			<< " BObj=0x" << reinterpret_cast<uintptr_t>(BO)
			<< std::dec
			<< "\n";
	}

	bool TryCopyEntryInitWeakContextFromLiveGrid(UObject* DestGridObj)
	{
		if (!IsSupportedItemGridObject(DestGridObj))
			return false;

		auto TryAdoptFromCandidate = [&](UObject* Candidate, int32 CandidateIndex) -> bool
		{
			if (!Candidate || Candidate == DestGridObj)
				return false;
			if (!IsSupportedItemGridObject(Candidate))
				return false;
			if (Candidate->IsDefaultObject())
				return false;

			const FWeakObjectPtr A = ReadWeakPtrAt(Candidate, kNeoTileInitWeakOffsetA);
			const FWeakObjectPtr B = ReadWeakPtrAt(Candidate, kNeoTileInitWeakOffsetB);
			if (!IsWeakPtrFilled(B))
				return false;
			UObject* BO = ResolveWeakPtrLoose(B);
			if (!IsSafeLiveObjectOfClass(BO, UNeoUIUniversalModuleVMBase::StaticClass()))
				return false;

			if (IsWeakPtrFilled(A))
				WriteWeakPtrAt(DestGridObj, kNeoTileInitWeakOffsetA, A);
			WriteWeakPtrAt(DestGridObj, kNeoTileInitWeakOffsetB, B);
			CacheEntryInitWeakB(B);
			LOGI_STREAM("ItemBrowser")
				<< "[SDK] ItemGrid entry-init context copied: src=0x"
				<< std::hex << reinterpret_cast<uintptr_t>(Candidate)
				<< " dst=0x" << reinterpret_cast<uintptr_t>(DestGridObj)
				<< " BObj=0x" << reinterpret_cast<uintptr_t>(BO)
				<< std::dec
				<< " index=" << CandidateIndex
				<< " srcClass=" << Candidate->GetFullName()
				<< "\n";
			return true;
		};

		// 1) 闈炴悳绱㈣矾寰勶細浼樺厛鍚冪紦瀛?		if (GHasCachedEntryInitWeakB && IsWeakPtrFilled(GCachedEntryInitWeakB))
		{
			UObject* CachedBO = ResolveWeakPtrLoose(GCachedEntryInitWeakB);
			if (IsSafeLiveObjectOfClass(CachedBO, UNeoUIUniversalModuleVMBase::StaticClass()))
			{
				WriteWeakPtrAt(DestGridObj, kNeoTileInitWeakOffsetB, GCachedEntryInitWeakB);
				LOGI_STREAM("ItemBrowser")
					<< "[SDK] ItemGrid entry-init context copied from cache: dst=0x"
					<< std::hex << reinterpret_cast<uintptr_t>(DestGridObj)
					<< std::dec
					<< " B=(" << GCachedEntryInitWeakB.ObjectIndex << "," << GCachedEntryInitWeakB.ObjectSerialNumber << ")\n";
				return true;
			}
		}

		// 2) 搜索逻辑默认禁用，只依赖 EVT_RenderView 注入缓存。
		if (!kEnableEntryInitSearch)
			return false;

		auto* ObjArray = UObject::GObjects.GetTypedPtr();
		if (!ObjArray)
			return false;

		const int32 Num = ObjArray->Num();

		// 3) 涓绘悳绱細閿氱偣绐楀彛鎵弿
		if (Num > 0)
		{
			int32 Start = kGridSearchAnchorIndex - kGridSearchWindow;
			int32 End = kGridSearchAnchorIndex + kGridSearchWindow;
			if (Start < 0) Start = 0;
			if (End >= Num) End = Num - 1;
			for (int32 i = Start; i <= End; ++i)
			{
				UObject* Candidate = ObjArray->GetByIndex(i);
				if (TryAdoptFromCandidate(Candidate, i))
					return true;
			}
		}

		// 4) 澶囩敤锛氬叏閲忔壂锛堥粯璁ゅ叧闂級
		if (!kEnableFullScanFallback)
			return false;

		for (int32 i = 0; i < Num; ++i)
		{
			UObject* Candidate = ObjArray->GetByIndex(i);
			if (TryAdoptFromCandidate(Candidate, i))
				return true;
		}

		return false;
	}

	void EnsureEntryInitWeakContext(UObject* GridObj)
	{
		if (!IsSupportedItemGridObject(GridObj))
			return;

		LogEntryInitWeakContext("before", GridObj);
		if (HasUsableEntryInitWeakContext(GridObj))
		{
			CacheEntryInitWeakB(ReadWeakPtrAt(GridObj, kNeoTileInitWeakOffsetB));
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemGrid entry-init context already valid\n";
			return;
		}

		const bool Copied = TryCopyEntryInitWeakContextFromLiveGrid(GridObj);
		if (!Copied)
		{
			static DWORD sLastContextMissLogTick = 0;
			const DWORD Now = GetTickCount();
			if (sLastContextMissLogTick == 0 || (Now - sLastContextMissLogTick) >= 1500)
			{
				sLastContextMissLogTick = Now;
				LOGE_STREAM("ItemBrowser") << "[SDK] ItemGrid entry-init context repair failed: "
					<< (kEnableEntryInitSearch ? "search miss" : "search disabled and cache empty")
					<< "\n";
			}
		}
		LogEntryInitWeakContext("after", GridObj);
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

		// 閺夌儐鍓欏┃鈧柡鍫㈠枛濡潡寮垫径澶屾槀閻犙冨缁喗瀵煎顐㈩槻闁哄啯婀圭粭澶愬矗椤栨粍鏆忛柨娑樼焷椤旀洜绱旈鈧崳鍝ユ嫚閺囩姷宕堕柛娆欑秮娴尖晠宕楀鍡橆攳濞?missing 婵厜鍓濋悡瀣Υ?
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

		// 閺夆晜鐟ょ花娲礌閸濆嫮鍘甸柛锔哄妿鐎氼厾绮╃€ｎ剙鈷栭柛?Tip 濞戞搩鍘煎ù鎰偓瑙勫哺濞堬綁鎸婅箛銉х闁告瑯浜滃﹢顏堝礆濠靛棭娼楅柛鏍ㄧ墬濡炲倻鎷嬮崜褏鏋傚☉鎾亾婵炲枴鎵冲亾?
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
		case 0: match = true; break;                          // 闁稿繈鍔戦崕?
		case 1: match = (st >= 1 && st <= 6); break;         // 婵繐绠戝▍?
		case 2: match = (st >= 10 && st <= 13); break;       // 闂傚啯褰冮崣?
		case 3: match = (st >= 14 && st <= 17); break;       // 婵炴垵鐗愰埀顒侇殔閹?
		default: match = (st == 0 || st > 17); break;        // 闁稿繑婀圭划?
		}
		if (match && !GItemSearchKeywordFolded.empty())
		{
			bool SearchMatch = ContainsSearchInsensitive(GAllItems[i].Name, GItemSearchKeywordFolded);
			if (!SearchMatch)
			{
				wchar_t DefIdBuf[32] = {};
				swprintf_s(DefIdBuf, 32, L"%d", GAllItems[i].DefId);
				SearchMatch = ContainsSearchInsensitive(DefIdBuf, GItemSearchKeywordFolded);
			}
			match = SearchMatch;
		}

		if (match)
			GFilteredIndices.push_back(i);
	}
	GItemTotalPages = ((int32)GFilteredIndices.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
	if (GItemTotalPages < 1) GItemTotalPages = 1;
	LOGI_STREAM("ItemBrowser") << "[SDK] Filter cat=" << category
		<< " searchLen=" << GItemSearchKeyword.size()
		<< ": " << GFilteredIndices.size()
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
		GEntryInitPreparedListView = nullptr;
	}
	else
	{
		const bool NeedPrepareEntryInit = (GEntryInitPreparedListView != ListView);
		if (NeedPrepareEntryInit)
		{
			const int32 EntryInitBindCountBefore = ListView->BP_OnEntryInitialized.InvocationList.Num();
			if (EntryInitBindCountBefore <= 0 &&
				IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UBP_ItemGridWDT_C::StaticClass()))
			{
				auto* ItemGridWdt = static_cast<UBP_ItemGridWDT_C*>(ListView);
				ItemGridWdt->ExecuteUbergraph_BP_ItemGridWDT(66);
				ItemGridWdt->EVT_InitOnce();
			}
			const int32 EntryInitBindCountAfter = ListView->BP_OnEntryInitialized.InvocationList.Num();
			LOGI_STREAM("ItemBrowser")
				<< "[SDK] ItemGrid entry-init delegate binds: before=" << EntryInitBindCountBefore
				<< " after=" << EntryInitBindCountAfter
				<< "\n";
			if (EntryInitBindCountAfter > 0)
			{
				const auto& BindList = ListView->BP_OnEntryInitialized.InvocationList;
				const int32 DumpCount = (BindList.Num() < 3) ? BindList.Num() : 3;
				for (int32 Bi = 0; Bi < DumpCount; ++Bi)
				{
					const FScriptDelegate& D = BindList[Bi];
					UObject* TargetObj = D.Object.Get();
					LOGI_STREAM("ItemBrowser")
						<< "[SDK] ItemGrid entry-init delegate[" << Bi << "]: target=0x"
						<< std::hex << reinterpret_cast<uintptr_t>(TargetObj)
						<< " self=0x" << reinterpret_cast<uintptr_t>(ListView)
						<< std::dec
						<< " isSelf=" << ((TargetObj == static_cast<UObject*>(ListView)) ? 1 : 0)
						<< " func=" << D.FunctionName.ToString()
						<< "\n";
				}
			}

			EnsureEntryInitWeakContext(static_cast<UObject*>(ListView));
			GEntryInitPreparedListView = ListView;
		}

		for (UItemInfoSpec* Spec : GCurrentPageSpecs)
		{
			if (Spec && IsSafeLiveObject(static_cast<UObject*>(Spec)))
				ClearGCRoot(static_cast<UObject*>(Spec));
		}
		GCurrentPageSpecs.clear();
		ListView->ClearListItems();

		std::vector<int32> PageItemIndices;
		PageItemIndices.reserve(ITEMS_PER_PAGE);
		int32 BuiltSlots = 0;
		int32 SpecCreateFail = 0;
		int32 SpecInvalidFail = 0;
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
			if (!Spec)
			{
				++SpecCreateFail;
				continue;
			}
			if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(Spec), UItemInfoSpec::StaticClass()))
			{
				++SpecInvalidFail;
				continue;
			}

			MarkAsGCRoot(static_cast<UObject*>(Spec));
			GCurrentPageSpecs.push_back(Spec);
			PageItemIndices.push_back(ItemIdx);
			++BuiltSlots;
		}

		TAllocatedArray<UObject*> InListItems(ITEMS_PER_PAGE);
		int32 SpecNonNullAtFeed = 0;
		int32 SpecLiveAtFeed = 0;
		int32 SpecClassOkAtFeed = 0;
		for (int32 SpecIdx = 0; SpecIdx < static_cast<int32>(GCurrentPageSpecs.size()); ++SpecIdx)
		{
			UItemInfoSpec* Spec = GCurrentPageSpecs[static_cast<size_t>(SpecIdx)];
			const bool NonNull = (Spec != nullptr);
			const bool Live = NonNull && IsSafeLiveObject(static_cast<UObject*>(Spec));
			const bool ClassOk = NonNull &&
				IsSafeLiveObjectOfClass(static_cast<UObject*>(Spec), UItemInfoSpec::StaticClass());
			if (NonNull) ++SpecNonNullAtFeed;
			if (Live) ++SpecLiveAtFeed;
			if (ClassOk) ++SpecClassOkAtFeed;

			const int32 BeforeNum = InListItems.Num();
			// Spec 闁革负鍔岄崹鍗烆嚈濞差亝鈻夋繛鍫ユ涧閸戯繝宕?IsSafeLiveObjectOfClass 闁哄稄绻濋悰娆撴晬瀹€鍐闂佹彃鐬煎ú鍧楀箳閵夈儱寮抽柛鎺擃殣缁?			// 闂侇剙鐏濋崢銈夋煂瀹ュ拋妲?validity 婵☆偀鍋撻柡灞诲劚濠€顏堝蓟閹邦亞鏄傞柡鍐煐濠р偓閻庝絻澹堥崵褏鎷犻姘伈濞?invalid闁?			if (Spec)
				InListItems.Add(static_cast<UObject*>(Spec));
			const int32 AfterNum = InListItems.Num();

			if (kVerboseItemFeedLogs)
			{
				LOGI_STREAM("ItemBrowser")
					<< "[SDK] ItemGrid feed spec[" << SpecIdx
					<< "]: ptr=0x" << std::hex << reinterpret_cast<uintptr_t>(Spec) << std::dec
					<< " nonNull=" << (NonNull ? 1 : 0)
					<< " live=" << (Live ? 1 : 0)
					<< " classOk=" << (ClassOk ? 1 : 0)
					<< " addNum=" << BeforeNum << "->" << AfterNum
					<< "\n";
			}
		}
		LOGI_STREAM("ItemBrowser")
			<< "[SDK] ItemGrid feed pre-set: page=" << (GItemCurrentPage + 1)
			<< "/" << GItemTotalPages
			<< " builtSlots=" << BuiltSlots
			<< " specVec=" << GCurrentPageSpecs.size()
			<< " inListItems=" << InListItems.Num()
			<< " specNonNullAtFeed=" << SpecNonNullAtFeed
			<< " specLiveAtFeed=" << SpecLiveAtFeed
			<< " specClassOkAtFeed=" << SpecClassOkAtFeed
			<< " specCreateFail=" << SpecCreateFail
			<< " specInvalidFail=" << SpecInvalidFail
			<< "\n";

		const TArray<UObject*> InListItemsView = InListItems;
		ListView->BP_SetListItems(InListItemsView);
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemGrid feed post BP_SetListItems: numItems="
			<< ListView->GetNumItems() << "\n";
		if (ListView->GetNumItems() > 0)
		{
			UObject* FirstItem = ListView->GetItemAt(0);
			LOGI_STREAM("ItemBrowser")
				<< "[SDK] ItemGrid list first item: ptr=0x"
				<< std::hex << reinterpret_cast<uintptr_t>(FirstItem)
				<< std::dec
				<< " classOk="
				<< (IsSafeLiveObjectOfClass(FirstItem, UItemInfoSpec::StaticClass()) ? 1 : 0)
				<< "\n";
		}
		ListView->ScrollToTop();
		ListView->RequestRefresh();
		ListView->RegenerateAllEntries();
		LOGI_STREAM("ItemBrowser") << "[SDK] ItemGrid feed post base refresh: numItems="
			<< ListView->GetNumItems() << "\n";

		if (IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UNeoUITileView::StaticClass()))
		{
			auto* NeoTile = static_cast<UNeoUITileView*>(ListView);
			NeoTile->ResetAndScrollToTop();
			ListView->RequestRefresh();
			ListView->RegenerateAllEntries();
			LOGI_STREAM("ItemBrowser") << "[SDK] ItemGrid feed post NeoTile reset: numItems="
				<< ListView->GetNumItems() << "\n";
		}
		if (IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UJHNeoUIItemGrid::StaticClass()))
		{
			if (NeedPrepareEntryInit)
			{
				auto* ItemGrid = static_cast<UJHNeoUIItemGrid*>(ListView);
				ItemGrid->EVT_InitOnce();
				ListView->RequestRefresh();
				ListView->RegenerateAllEntries();
				LOGI_STREAM("ItemBrowser") << "[SDK] ItemGrid feed post ItemGrid EVT_InitOnce: numItems="
					<< ListView->GetNumItems() << "\n";
			}
		}
		if (IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UNeoUIListView::StaticClass()))
		{
			if (NeedPrepareEntryInit)
			{
				auto* NeoList = static_cast<UNeoUIListView*>(ListView);
				NeoList->EVT_InitOnce();
				UJHNeoUIUtilLib::NeoUIListRender(NeoList);
				ListView->RequestRefresh();
				ListView->RegenerateAllEntries();
				LOGI_STREAM("ItemBrowser") << "[SDK] ItemGrid feed post NeoUIListRender: numItems="
					<< ListView->GetNumItems() << "\n";
			}
		}

		const int32 NumItemsInList = ListView->GetNumItems();
		TArray<UUserWidget*> Displayed = ListView->GetDisplayedEntryWidgets();
		int32 DisplayedNum = Displayed.Num();
		const int32 PoolActiveNum0 = ListView->EntryWidgetPool.ActiveWidgets.Num();
		const int32 PoolInactiveNum0 = ListView->EntryWidgetPool.InactiveWidgets.Num();
		if (DisplayedNum == 0 && PoolActiveNum0 > 0)
		{
			Displayed = ListView->EntryWidgetPool.ActiveWidgets;
			DisplayedNum = Displayed.Num();
			LOGI_STREAM("ItemBrowser")
				<< "[SDK] ItemGrid displayed fallback: source=EntryWidgetPool.ActiveWidgets"
				<< " active=" << PoolActiveNum0
				<< " inactive=" << PoolInactiveNum0
				<< " displayed=" << DisplayedNum
				<< "\n";
		}
		int32 LayoutRetryCount = 0;
		if (NumItemsInList > 0 && DisplayedNum == 0)
		{
			// 闁告帗绻傞～鎰板礌閺嶎厽鈻夋繛鍫㈡暩缁紕鏁粙鍨弗闁哥姴鍊归弳鐔煎箲椤旇　鍋撴担鍛婂€甸悗鐟版湰閸ㄦ氨鏁崘銊ф拱闁挎稒绋栬棢闁告垹濮鹃悿?prepass/refresh 閻?entry 闁活亞鍠愰婊呪偓鍦仒缁躲儵宕犻弽銉㈠亾?			for (int32 Retry = 0; Retry < 3 && DisplayedNum == 0; ++Retry)
			{
				++LayoutRetryCount;
				if (GItemGridPanel && IsSafeLiveObject(static_cast<UObject*>(GItemGridPanel)))
				{
					GItemGridPanel->ForceLayoutPrepass();
					UPanelWidget* P0 = GItemGridPanel->GetParent();
					if (P0 && IsSafeLiveObject(static_cast<UObject*>(P0)))
					{
						P0->ForceLayoutPrepass();
						UPanelWidget* P1 = P0->GetParent();
						if (P1 && IsSafeLiveObject(static_cast<UObject*>(P1)))
							P1->ForceLayoutPrepass();
					}
				}
				ListView->ForceLayoutPrepass();
				ListView->RequestRefresh();
				ListView->RegenerateAllEntries();
				Displayed = ListView->GetDisplayedEntryWidgets();
				DisplayedNum = Displayed.Num();
				if (DisplayedNum == 0)
				{
					const int32 PoolActiveNumRetry = ListView->EntryWidgetPool.ActiveWidgets.Num();
					const int32 PoolInactiveNumRetry = ListView->EntryWidgetPool.InactiveWidgets.Num();
					if (PoolActiveNumRetry > 0)
					{
						Displayed = ListView->EntryWidgetPool.ActiveWidgets;
						DisplayedNum = Displayed.Num();
						LOGI_STREAM("ItemBrowser")
							<< "[SDK] ItemGrid displayed fallback(retry): source=EntryWidgetPool.ActiveWidgets"
							<< " active=" << PoolActiveNumRetry
							<< " inactive=" << PoolInactiveNumRetry
							<< " displayed=" << DisplayedNum
							<< "\n";
					}
				}
			}

			const char* ListVis = VisName(ListView->GetVisibility());
			const char* GridVis = GItemGridPanel ? VisName(GItemGridPanel->GetVisibility()) : "null";
			const char* ParentVis = "null";
			if (GItemGridPanel && IsSafeLiveObject(static_cast<UObject*>(GItemGridPanel)))
			{
				UPanelWidget* P0 = GItemGridPanel->GetParent();
				if (P0 && IsSafeLiveObject(static_cast<UObject*>(P0)))
					ParentVis = VisName(P0->GetVisibility());
			}
			LOGI_STREAM("ItemBrowser")
				<< "[SDK] ItemGrid layout retry: tries=" << LayoutRetryCount
				<< " numItems=" << NumItemsInList
				<< " displayedAfter=" << DisplayedNum
				<< " poolActive=" << ListView->EntryWidgetPool.ActiveWidgets.Num()
				<< " poolInactive=" << ListView->EntryWidgetPool.InactiveWidgets.Num()
				<< " listVis=" << ListVis
				<< " gridVis=" << GridVis
				<< " parentVis=" << ParentVis
				<< "\n";
		}
		const int32 MappedNum = static_cast<int32>(PageItemIndices.size());

		auto BindEntryToSlot = [&](int32 Slot, UUserWidget* EntryWidget, int32 ItemIdx, UObject* ListItemObj) -> bool
		{
			if (Slot < 0 || Slot >= ITEMS_PER_PAGE)
				return false;
			if (!IsSafeLiveObject(static_cast<UObject*>(EntryWidget)))
				return false;
			if (ItemIdx < 0 || ItemIdx >= static_cast<int32>(GAllItems.size()))
				return false;

			// 鐎殿喖鎼崺妤冩喆閿曗偓瑜?ListEntry 闁轰胶澧楀畵浣虹磼閹存繄鏆板☉鎾冲鐟曞棝寮婚幘瑙勭ギ闁硅婢佺槐婵嬫焼閸喖甯抽柡灞惧姃缁?WDT 閻犱警鍨扮欢鐐哄矗椤忓棙鏅搁柟瀛樺姇閿涙挾鈧稒鍔掔粭澶愬礆瀹勬澘鏁堕悗鐟扮畭閳?			if (ListItemObj && EntryWidget->IsA(IUserObjectListEntry::StaticClass()))
			{
				auto* ObjEntry = reinterpret_cast<IUserObjectListEntry*>(EntryWidget);
				ObjEntry->OnListItemObjectSet(ListItemObj);
			}
			if (EntryWidget->IsA(UNeoUIReusableVisualEntryWithDataTransformer::StaticClass()))
			{
				auto* ReusableXform = static_cast<UNeoUIReusableVisualEntryWithDataTransformer*>(EntryWidget);
				ReusableXform->ForceConvertAndRender();
			}

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
			UObject* BindObj = nullptr;
			if (i >= 0 && i < static_cast<int32>(GCurrentPageSpecs.size()))
				BindObj = static_cast<UObject*>(GCurrentPageSpecs[static_cast<size_t>(i)]);
			if (BindEntryToSlot(i, Displayed[i], PageItemIndices[i], BindObj))
				++SlotCount;
		}

		// Scan fallback removed: only trust ListView displayed/pool entries.
		LOGI_STREAM("ItemBrowser")
			<< "[SDK] ItemGrid refresh: page=" << (GItemCurrentPage + 1)
			<< "/" << GItemTotalPages
			<< " dataSlots=" << BuiltSlots
			<< " listItems=" << NumItemsInList
			<< " displayedEntries=" << DisplayedNum
			<< " mappedEntries=" << SlotCount
			<< " expectedMap=" << MappedNum
			<< "\n";
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
	// 濞村吋锚閸樻稒鎷呯捄銊︽殢缂傚啯鍨堕悧鎼佸锤閹邦厾鍨奸柛娑欏灊閼垫垿鏁嶅畝鍕級闁稿繐绉甸惁鈩冩姜椤曗偓閸忔﹢宕?24 闁哄秴鍚嬬换浣规償?IsHovered 闁规亽鍨虹粊鎾Υ?
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

	// 缂傚啯鍨堕悧鎼佸川閹存帟鍘鎯扮簿鐟欙箓寮拋鍦濞达絽閰ｉ。鍓佹導閻楀牏绠掗幖杈鹃檮鐢澘霉鐎ｎ亜骞戦幖瀛樻穿缁辨繈宕楅悡搴晣闁绘顫夐悾鈺冩暜閸愩劎婀伴柕?
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

	// 闁诡噮鍓氱拠鐐哄礆閸ャ劌搴婄紒瀣珪閳ь兛绀侀崹鐣屸偓瑙勭啲缁辩増螚閻樺磭鍨奸煫鍥跺亰閳ь剛鍠愭竟鍌涙交閸ャ劎澹愰悗娑欏姈濡炲倿鏁嶇仦鑲╃憹閻熸洑鐒﹂惁锛勬暜瑜旈崗姗€鏌屽鍛处 Tip 闁告劕鎳庨鎰板Υ?
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
		// 濞寸姴鎳嶆繛鍥偨閵娿劌娈扮€?StandaloneGameTip闁挎稑濂旂粭澶愬礃瀹ュ牏娈堕柣顫妽閻栧爼骞嬭箛鎾虫枾闁?Tip VM 闁规亽鍎辫ぐ娑㈠Υ?
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
			// UBPVE_JHTips_Item_C 闁哄秶鎳撹ぐ鏌ユ嚄閼恒儲笑闁轰焦娼欓惈鍡欌偓鍦嚀濞呮帡鏁嶇€涚灜andalone 闁搞儱鎼悾楣冩偨閵娿儱骞㈤柣妤€娲ら弰鍌溾偓鐢殿焾瀵剚绋夋惔锛勬毎濞达絽绉查埀?
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

void OnItemBrowserTabShown()
{
	UListView* ListView = GItemListView;
	if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UListView::StaticClass()))
		return;

	if (GItemGridPanel && IsSafeLiveObject(static_cast<UObject*>(GItemGridPanel)))
	{
		GItemGridPanel->ForceLayoutPrepass();
		UPanelWidget* P0 = GItemGridPanel->GetParent();
		if (P0 && IsSafeLiveObject(static_cast<UObject*>(P0)))
		{
			P0->ForceLayoutPrepass();
			UPanelWidget* P1 = P0->GetParent();
			if (P1 && IsSafeLiveObject(static_cast<UObject*>(P1)))
				P1->ForceLayoutPrepass();
		}
	}

	ListView->ForceLayoutPrepass();
	ListView->RequestRefresh();
	ListView->RegenerateAllEntries();
	EnsureEntryInitWeakContext(static_cast<UObject*>(ListView));

	LOGI_STREAM("ItemBrowser")
		<< "[SDK] ItemGrid tab-shown pre-refresh: numItems=" << ListView->GetNumItems()
		<< " gridVis=" << (GItemGridPanel ? VisName(GItemGridPanel->GetVisibility()) : "null")
		<< " listVis=" << VisName(ListView->GetVisibility())
		<< "\n";

	RefreshItemPage();
}

void CacheEntryInitContextWeakB(const FWeakObjectPtr& WeakB, const char* SourceTag)
{
	if (!IsWeakPtrFilled(WeakB))
		return;

	UObject* CtxObj = ResolveWeakPtrLoose(WeakB);
	if (!IsSafeLiveObjectOfClass(CtxObj, UNeoUIUniversalModuleVMBase::StaticClass()))
		return;

	CacheEntryInitWeakB(WeakB);
	if (GItemListView && IsSafeLiveObject(static_cast<UObject*>(GItemListView)))
		WriteWeakPtrAt(static_cast<UObject*>(GItemListView), kNeoTileInitWeakOffsetB, WeakB);

	LOGI_STREAM("ItemBrowser")
		<< "[SDK] ItemGrid entry-init context cached from external source: src="
		<< (SourceTag ? SourceTag : "unknown")
		<< " B=(" << WeakB.ObjectIndex << "," << WeakB.ObjectSerialNumber << ")"
		<< " CtxObj=0x" << std::hex << reinterpret_cast<uintptr_t>(CtxObj) << std::dec
		<< "\n";
}

void SetItemSearchEditBox(UEditableTextBox* Edit)
{
	GItemSearchEdit = Edit;
}

bool UpdateItemSearchKeywordFromEdit()
{
	std::wstring NewKeyword;
	if (GItemSearchEdit && IsSafeLiveObject(static_cast<UObject*>(GItemSearchEdit)))
	{
		const FText Text = GItemSearchEdit->GetText();
		const FString Raw = UKismetTextLibrary::Conv_TextToString(Text);
		const wchar_t* WS = Raw.CStr();
		if (WS)
			NewKeyword = WS;
	}

	if (NewKeyword == GItemSearchKeyword)
		return false;

	GItemSearchKeyword = NewKeyword;
	GItemSearchKeywordFolded = FoldSearchText(GItemSearchKeyword);
	return true;
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
	GItemSearchEdit = nullptr;
	GItemSearchKeyword.clear();
	GItemSearchKeywordFolded.clear();

	GItemGridPanel = nullptr;
	GItemListView = nullptr;
	GEntryInitPreparedListView = nullptr;
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


