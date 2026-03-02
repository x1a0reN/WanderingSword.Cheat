#include <Windows.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CheatState.hpp"
#include "FrameHook.hpp"
#include "ItemBrowser.hpp"
#include "PanelManager.hpp"
#include "TabContent.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"
#include "SDK/JH_structs.hpp"
#include "SDK/JH_parameters.hpp"
#include "SDK/JH_classes.hpp"
#include "SDK/Engine_classes.hpp"

namespace
{
	struct PostRenderInFlightScope final
	{
		PostRenderInFlightScope()
		{
			GPostRenderInFlight.fetch_add(1, std::memory_order_acq_rel);
		}

		~PostRenderInFlightScope()
		{
			GPostRenderInFlight.fetch_sub(1, std::memory_order_acq_rel);
		}
	};

	bool EnsureLiveInternalWidgetForFrame()
	{
		if (!InternalWidget)
			return false;

		auto* Obj = static_cast<UObject*>(InternalWidget);
		if (IsSafeLiveObject(Obj))
			return true;

		std::cout << "[SDK] FrameHook: stale internal widget pointer detected, reset state\n";
		InternalWidget = nullptr;
		InternalWidgetVisible = false;
		GCachedBtnExit = nullptr;
		ClearRuntimeWidgetState();
		return false;
	}

	void DrawFpsOverlay(UCanvas* CanvasObj)
	{
		if (!CanvasObj || !IsSafeLiveObjectOfClass(static_cast<UObject*>(CanvasObj), UCanvas::StaticClass()))
			return;

		static ULONGLONG sFpsWindowStartTick = 0;
		static int32 sFpsFrameCount = 0;
		static float sDisplayFps = 0.0f;

		const ULONGLONG NowTick = GetTickCount64();
		if (sFpsWindowStartTick == 0)
			sFpsWindowStartTick = NowTick;

		++sFpsFrameCount;
		const ULONGLONG ElapsedMs = NowTick - sFpsWindowStartTick;
		if (ElapsedMs >= 250ULL)
		{
			sDisplayFps = (static_cast<float>(sFpsFrameCount) * 1000.0f) / static_cast<float>(ElapsedMs);
			sFpsFrameCount = 0;
			sFpsWindowStartTick = NowTick;
		}

		wchar_t Buf[64] = {};
		swprintf_s(Buf, 64, L"FPS: %.1f", sDisplayFps);

		UFont* Font = nullptr;
		UEngine* Engine = UEngine::GetEngine();
		if (Engine && IsSafeLiveObject(static_cast<UObject*>(Engine)))
			Font = Engine->SmallFont;

		CanvasObj->K2_DrawText(
			Font,
			FString(Buf),
			FVector2D{ 12.0f, 10.0f },
			FVector2D{ 1.0f, 1.0f },
			FLinearColor{ 0.1f, 1.0f, 0.1f, 1.0f },
			0.0f,
			FLinearColor{ 0.0f, 0.0f, 0.0f, 0.9f },
			FVector2D{ 1.0f, 1.0f },
			false,
			false,
			true,
			FLinearColor{ 0.0f, 0.0f, 0.0f, 1.0f });
	}

	struct FGuidKey final
	{
		int32 A = 0;
		int32 B = 0;
		int32 C = 0;
		int32 D = 0;

		bool operator==(const FGuidKey& Other) const
		{
			return A == Other.A && B == Other.B && C == Other.C && D == Other.D;
		}
	};

	struct FGuidKeyHasher final
	{
		size_t operator()(const FGuidKey& Key) const
		{
			uint64 V = static_cast<uint32>(Key.A);
			V = (V * 0x9E3779B185EBCA87ULL) ^ static_cast<uint32>(Key.B);
			V = (V * 0x9E3779B185EBCA87ULL) ^ static_cast<uint32>(Key.C);
			V = (V * 0x9E3779B185EBCA87ULL) ^ static_cast<uint32>(Key.D);
			return static_cast<size_t>(V);
		}
	};

	struct FItemInventorySnapshot final
	{
		int32 Num = 0;
		int32 DefId = 0;
	};

	struct FTab1RuntimeConfig final
	{
		bool ItemNoDecrease = false;
		bool ItemGainMultiplier = false;
		int32 ItemGainMultiplierValue = 1;
		bool AllItemsSellable = false;
		bool IncludeQuestItems = false;
		bool DropRate100 = false;
		bool CraftEffectMultiplier = false;
		float CraftItemIncrementMultiplier = 1.0f;
		float CraftExtraEffectMultiplier = 1.0f;
		int32 MaxExtraAffixes = 0;
		bool IgnoreItemUseCount = false;
		bool IgnoreItemRequirements = false;
	};

	struct FItemRowOriginalState final
	{
		bool bCantSell = false;
		float SellPrice = 0.0f;
		int32 UsedCountLimit = 0;
		std::vector<FRequirementSetting> Requirements;
		std::vector<int32> RandRange;
		std::vector<int32> NormalRandRange;
		std::vector<FActionSetting> Actions;
	};

	struct FDropPoolOriginalState final
	{
		std::vector<FDropWeight> DropWeights;
	};

	struct FRandPoolOriginalState final
	{
		std::vector<FActionWeight> ActionWeights;
	};

	std::unordered_map<FGuidKey, FItemInventorySnapshot, FGuidKeyHasher> GTab1ItemSnapshots;
	UDataTable* GTab1CachedItemTable = nullptr;
	UDataTable* GTab1CachedDropPoolTable = nullptr;
	UDataTable* GTab1CachedRandPoolTable = nullptr;
	std::unordered_map<uintptr_t, FItemRowOriginalState> GTab1ItemRowOriginals;
	std::unordered_map<uintptr_t, FDropPoolOriginalState> GTab1DropPoolOriginals;
	std::unordered_map<uintptr_t, FRandPoolOriginalState> GTab1RandPoolOriginals;

	bool IsLiveComboBox(UComboBoxString* Combo)
	{
		return Combo &&
			IsSafeLiveObject(static_cast<UObject*>(Combo)) &&
			!(Combo->Flags & EObjectFlags::BeginDestroyed) &&
			!(Combo->Flags & EObjectFlags::FinishDestroyed);
	}

	int32 GetComboSelectedIndexSafe(UComboBoxString* Combo)
	{
		if (!IsLiveComboBox(Combo))
			return -1;

		const int32 Count = Combo->GetOptionCount();
		if (Count <= 0)
			return -1;

		return Combo->GetSelectedIndex();
	}

	bool ReadToggleValue(UBPVE_JHConfigVideoItem2_C* Toggle, bool DefaultValue)
	{
		if (!Toggle || !IsSafeLiveObject(static_cast<UObject*>(Toggle)))
			return DefaultValue;
		if (!Toggle->CB_Main || !IsLiveComboBox(Toggle->CB_Main))
			return DefaultValue;

		const int32 Idx = GetComboSelectedIndexSafe(Toggle->CB_Main);
		if (Idx < 0)
			return DefaultValue;
		return Idx > 0;
	}

	float ReadSliderPercent(UBPVE_JHConfigVolumeItem2_C* SliderItem, float DefaultPercent)
	{
		if (!SliderItem || !IsSafeLiveObject(static_cast<UObject*>(SliderItem)))
			return DefaultPercent;
		USlider* Slider = SliderItem->VolumeSlider;
		if (!Slider || !IsSafeLiveObject(static_cast<UObject*>(Slider)))
			return DefaultPercent;

		float MinValue = Slider->MinValue;
		float MaxValue = Slider->MaxValue;
		float CurValue = Slider->GetValue();
		if (MaxValue < MinValue)
		{
			const float T = MaxValue;
			MaxValue = MinValue;
			MinValue = T;
		}

		float Norm = CurValue;
		if (MaxValue > MinValue)
			Norm = (CurValue - MinValue) / (MaxValue - MinValue);
		if (Norm < 0.0f) Norm = 0.0f;
		if (Norm > 1.0f) Norm = 1.0f;
		return Norm * 100.0f;
	}

	int32 ReadIntegerEditValue(UEditableTextBox* Edit, int32 DefaultValue, int32 MinValue, int32 MaxValue)
	{
		if (!Edit || !IsSafeLiveObject(static_cast<UObject*>(Edit)))
			return DefaultValue;

		const FText Text = Edit->GetText();
		const FString Raw = UKismetTextLibrary::Conv_TextToString(Text);
		const wchar_t* WS = Raw.CStr();
		if (!WS || !WS[0])
			return DefaultValue;

		int32 Value = _wtoi(WS);
		if (Value < MinValue) Value = MinValue;
		if (Value > MaxValue) Value = MaxValue;
		return Value;
	}

	int32 SliderPercentToIntMultiplier(float Percent)
	{
		int32 Value = static_cast<int32>(Percent + 0.5f);
		if (Value < 1) Value = 1;
		if (Value > 100) Value = 100;
		return Value;
	}

	float SliderPercentToFloatMultiplier(float Percent)
	{
		if (Percent < 0.0f) Percent = 0.0f;
		if (Percent > 100.0f) Percent = 100.0f;
		return 1.0f + Percent * 0.09f; // [1.0, 10.0]
	}

	template <typename T>
	std::vector<T> CopyArrayToVector(const TArray<T>& Array)
	{
		std::vector<T> Out;
		const int32 Count = Array.Num();
		if (Count <= 0)
			return Out;

		Out.reserve(static_cast<size_t>(Count));
		for (int32 i = 0; i < Count; ++i)
			Out.push_back(Array[i]);
		return Out;
	}

	template <typename T>
	void RestoreArrayFromVector(TArray<T>& Target, const std::vector<T>& Source)
	{
		const int32 TargetCount = Target.Num();
		const int32 SourceCount = static_cast<int32>(Source.size());
		int32 Count = TargetCount;
		if (SourceCount < Count)
			Count = SourceCount;

		for (int32 i = 0; i < Count; ++i)
			Target[i] = Source[static_cast<size_t>(i)];
	}

	FGuidKey MakeGuidKey(const FGuid& Guid)
	{
		FGuidKey Key{};
		Key.A = Guid.A;
		Key.B = Guid.B;
		Key.C = Guid.C;
		Key.D = Guid.D;
		return Key;
	}

	void ResetTab1TableCachesIfNeeded(UDataTable* ItemTable, UDataTable* DropTable, UDataTable* RandTable)
	{
		if (GTab1CachedItemTable != ItemTable)
		{
			GTab1CachedItemTable = ItemTable;
			GTab1ItemRowOriginals.clear();
		}
		if (GTab1CachedDropPoolTable != DropTable)
		{
			GTab1CachedDropPoolTable = DropTable;
			GTab1DropPoolOriginals.clear();
		}
		if (GTab1CachedRandPoolTable != RandTable)
		{
			GTab1CachedRandPoolTable = RandTable;
			GTab1RandPoolOriginals.clear();
		}
	}

	void CaptureTab1ItemRowOriginals(UDataTable* ItemTable)
	{
		if (!ItemTable || !IsSafeLiveObject(static_cast<UObject*>(ItemTable)))
			return;
		if (!GTab1ItemRowOriginals.empty())
			return;

		auto& RowMap = ItemTable->RowMap;
		if (!RowMap.IsValid())
			return;

		const int32 AllocatedSlots = RowMap.NumAllocated();
		for (int32 i = 0; i < AllocatedSlots; ++i)
		{
			if (!RowMap.IsValidIndex(i))
				continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData)
				continue;

			auto* Row = reinterpret_cast<FItemInfoSetting*>(RowData);
			FItemRowOriginalState Original{};
			Original.bCantSell = Row->bCantSell;
			Original.SellPrice = Row->SellPrice;
			Original.UsedCountLimit = Row->UsedCountLimit;
			Original.Requirements = CopyArrayToVector(Row->Requirements);
			Original.RandRange = CopyArrayToVector(Row->RandRange);
			Original.NormalRandRange = CopyArrayToVector(Row->NormalRandRange);
			Original.Actions = CopyArrayToVector(Row->Actions);
			GTab1ItemRowOriginals.emplace(reinterpret_cast<uintptr_t>(RowData), std::move(Original));
		}
	}

	void CaptureTab1DropPoolOriginals(UDataTable* DropTable)
	{
		if (!DropTable || !IsSafeLiveObject(static_cast<UObject*>(DropTable)))
			return;
		if (!GTab1DropPoolOriginals.empty())
			return;

		auto& RowMap = DropTable->RowMap;
		if (!RowMap.IsValid())
			return;

		const int32 AllocatedSlots = RowMap.NumAllocated();
		for (int32 i = 0; i < AllocatedSlots; ++i)
		{
			if (!RowMap.IsValidIndex(i))
				continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData)
				continue;

			auto* Row = reinterpret_cast<FDropPoolSetting*>(RowData);
			FDropPoolOriginalState Original{};
			Original.DropWeights = CopyArrayToVector(Row->DropWeights);
			GTab1DropPoolOriginals.emplace(reinterpret_cast<uintptr_t>(RowData), std::move(Original));
		}
	}

	void CaptureTab1RandPoolOriginals(UDataTable* RandTable)
	{
		if (!RandTable || !IsSafeLiveObject(static_cast<UObject*>(RandTable)))
			return;
		if (!GTab1RandPoolOriginals.empty())
			return;

		auto& RowMap = RandTable->RowMap;
		if (!RowMap.IsValid())
			return;

		const int32 AllocatedSlots = RowMap.NumAllocated();
		for (int32 i = 0; i < AllocatedSlots; ++i)
		{
			if (!RowMap.IsValidIndex(i))
				continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData)
				continue;

			auto* Row = reinterpret_cast<FRandActionPoolSetting*>(RowData);
			FRandPoolOriginalState Original{};
			Original.ActionWeights = CopyArrayToVector(Row->ActionWeights);
			GTab1RandPoolOriginals.emplace(reinterpret_cast<uintptr_t>(RowData), std::move(Original));
		}
	}

	void ApplyTab1ItemDefinitionFeatures(const FTab1RuntimeConfig& Cfg, UDataTable* ItemTable)
	{
		if (!ItemTable || !IsSafeLiveObject(static_cast<UObject*>(ItemTable)))
			return;

		CaptureTab1ItemRowOriginals(ItemTable);
		if (GTab1ItemRowOriginals.empty())
			return;

		auto& RowMap = ItemTable->RowMap;
		if (!RowMap.IsValid())
			return;

		const int32 AllocatedSlots = RowMap.NumAllocated();
		for (int32 i = 0; i < AllocatedSlots; ++i)
		{
			if (!RowMap.IsValidIndex(i))
				continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData)
				continue;

			auto* Row = reinterpret_cast<FItemInfoSetting*>(RowData);
			const auto It = GTab1ItemRowOriginals.find(reinterpret_cast<uintptr_t>(RowData));
			if (It == GTab1ItemRowOriginals.end())
				continue;
			const FItemRowOriginalState& Original = It->second;

			const bool IsQuestItem = (Row->ItemType == EItemSubType::QuestItem);

			if (Cfg.AllItemsSellable && (Cfg.IncludeQuestItems || !IsQuestItem))
			{
				Row->bCantSell = false;
				if (Row->SellPrice <= 0.0f)
					Row->SellPrice = 1.0f;
			}
			else
			{
				Row->bCantSell = Original.bCantSell;
				Row->SellPrice = Original.SellPrice;
			}

			if (Cfg.IgnoreItemUseCount)
				Row->UsedCountLimit = 0;
			else
				Row->UsedCountLimit = Original.UsedCountLimit;

			if (Cfg.IgnoreItemRequirements)
			{
				const int32 ReqCount = Row->Requirements.Num();
				for (int32 ReqIdx = 0; ReqIdx < ReqCount; ++ReqIdx)
				{
					Row->Requirements[ReqIdx].Type = ERequirementType::None;
					Row->Requirements[ReqIdx].ID = 0;
					Row->Requirements[ReqIdx].Num = 0.0f;
				}
			}
			else
			{
				RestoreArrayFromVector(Row->Requirements, Original.Requirements);
			}

			if (Cfg.MaxExtraAffixes > 0)
			{
				if (Row->RandRange.Num() >= 2)
				{
					Row->RandRange[0] = Cfg.MaxExtraAffixes;
					Row->RandRange[1] = Cfg.MaxExtraAffixes;
				}
				if (Row->NormalRandRange.Num() >= 2)
				{
					Row->NormalRandRange[0] = Cfg.MaxExtraAffixes;
					Row->NormalRandRange[1] = Cfg.MaxExtraAffixes;
				}
			}
			else
			{
				RestoreArrayFromVector(Row->RandRange, Original.RandRange);
				RestoreArrayFromVector(Row->NormalRandRange, Original.NormalRandRange);
			}

			if (Cfg.CraftEffectMultiplier)
			{
				const int32 CurrentCount = Row->Actions.Num();
				int32 Count = CurrentCount;
				const int32 OriginalCount = static_cast<int32>(Original.Actions.size());
				if (OriginalCount < Count)
					Count = OriginalCount;

				for (int32 ActIdx = 0; ActIdx < Count; ++ActIdx)
				{
					const FActionSetting& BaseAction = Original.Actions[static_cast<size_t>(ActIdx)];
					if (BaseAction.Type == EActionType::CItem)
						Row->Actions[ActIdx].Num = BaseAction.Num * Cfg.CraftItemIncrementMultiplier;
					else
						Row->Actions[ActIdx].Num = BaseAction.Num * Cfg.CraftExtraEffectMultiplier;
				}
			}
			else
			{
				RestoreArrayFromVector(Row->Actions, Original.Actions);
			}
		}
	}

	void ApplyTab1DropPoolFeature(const FTab1RuntimeConfig& Cfg, UDataTable* DropTable)
	{
		if (!DropTable || !IsSafeLiveObject(static_cast<UObject*>(DropTable)))
			return;

		CaptureTab1DropPoolOriginals(DropTable);
		if (GTab1DropPoolOriginals.empty())
			return;

		auto& RowMap = DropTable->RowMap;
		if (!RowMap.IsValid())
			return;

		const int32 AllocatedSlots = RowMap.NumAllocated();
		for (int32 i = 0; i < AllocatedSlots; ++i)
		{
			if (!RowMap.IsValidIndex(i))
				continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData)
				continue;

			auto* Row = reinterpret_cast<FDropPoolSetting*>(RowData);
			const auto It = GTab1DropPoolOriginals.find(reinterpret_cast<uintptr_t>(RowData));
			if (It == GTab1DropPoolOriginals.end())
				continue;
			const FDropPoolOriginalState& Original = It->second;

			if (!Cfg.DropRate100)
			{
				RestoreArrayFromVector(Row->DropWeights, Original.DropWeights);
				continue;
			}

			const int32 CurrentCount = Row->DropWeights.Num();
			int32 Count = CurrentCount;
			const int32 OriginalCount = static_cast<int32>(Original.DropWeights.size());
			if (OriginalCount < Count)
				Count = OriginalCount;

			for (int32 WIdx = 0; WIdx < Count; ++WIdx)
			{
				const FDropWeight& BaseWeight = Original.DropWeights[static_cast<size_t>(WIdx)];
				if (BaseWeight.ItemDefId <= 0)
					Row->DropWeights[WIdx].Weight = 0;
				else
					Row->DropWeights[WIdx].Weight = 100000;
			}
		}
	}

	void ApplyTab1RandPoolFeature(const FTab1RuntimeConfig& Cfg, UDataTable* RandTable)
	{
		if (!RandTable || !IsSafeLiveObject(static_cast<UObject*>(RandTable)))
			return;

		CaptureTab1RandPoolOriginals(RandTable);
		if (GTab1RandPoolOriginals.empty())
			return;

		auto& RowMap = RandTable->RowMap;
		if (!RowMap.IsValid())
			return;

		const int32 AllocatedSlots = RowMap.NumAllocated();
		for (int32 i = 0; i < AllocatedSlots; ++i)
		{
			if (!RowMap.IsValidIndex(i))
				continue;
			uint8* RowData = RowMap[i].Value();
			if (!RowData)
				continue;

			auto* Row = reinterpret_cast<FRandActionPoolSetting*>(RowData);
			const auto It = GTab1RandPoolOriginals.find(reinterpret_cast<uintptr_t>(RowData));
			if (It == GTab1RandPoolOriginals.end())
				continue;
			const FRandPoolOriginalState& Original = It->second;

			if (!Cfg.CraftEffectMultiplier)
			{
				RestoreArrayFromVector(Row->ActionWeights, Original.ActionWeights);
				continue;
			}

			const int32 CurrentCount = Row->ActionWeights.Num();
			int32 Count = CurrentCount;
			const int32 OriginalCount = static_cast<int32>(Original.ActionWeights.size());
			if (OriginalCount < Count)
				Count = OriginalCount;

			for (int32 AIdx = 0; AIdx < Count; ++AIdx)
			{
				const FActionWeight& BaseWeight = Original.ActionWeights[static_cast<size_t>(AIdx)];
				Row->ActionWeights[AIdx].Action.Num = BaseWeight.Action.Num * Cfg.CraftExtraEffectMultiplier;
			}
		}
	}

	void ApplyTab1BackpackFeatures(const FTab1RuntimeConfig& Cfg)
	{
		const bool EnableNoDecrease = Cfg.ItemNoDecrease;
		const bool EnableGainMultiplier = Cfg.ItemGainMultiplier && Cfg.ItemGainMultiplierValue > 1;
		if (!EnableNoDecrease && !EnableGainMultiplier)
		{
			GTab1ItemSnapshots.clear();
			return;
		}

		UItemManager* ItemMgr = UManagerFuncLib::GetItemManager();
		if (!ItemMgr || !IsSafeLiveObject(static_cast<UObject*>(ItemMgr)))
			return;

		TArray<UItemInfoSpec*>& BackpackItems = ItemMgr->BackpackItems;
		if (!BackpackItems.IsValid())
			return;

		std::unordered_set<FGuidKey, FGuidKeyHasher> SeenKeys;
		SeenKeys.reserve(static_cast<size_t>(BackpackItems.Num()) + 8);

		for (int32 i = 0; i < BackpackItems.Num(); ++i)
		{
			UItemInfoSpec* Spec = BackpackItems[i];
			if (!Spec || !IsSafeLiveObject(static_cast<UObject*>(Spec)))
				continue;

			const FGuidKey Key = MakeGuidKey(Spec->ID);
			SeenKeys.insert(Key);

			auto It = GTab1ItemSnapshots.find(Key);
			const bool HadSnapshot = (It != GTab1ItemSnapshots.end());
			const int32 PrevNum = HadSnapshot ? It->second.Num : 0;

			int32 CurNum = Spec->Num;
			if (CurNum < 0)
				CurNum = 0;

			if (EnableGainMultiplier && CurNum > PrevNum)
			{
				const int64 Delta = static_cast<int64>(CurNum) - static_cast<int64>(PrevNum);
				const int64 Extra = Delta * static_cast<int64>(Cfg.ItemGainMultiplierValue - 1);
				int64 NewNum = static_cast<int64>(CurNum) + Extra;
				const int64 MaxInt = static_cast<int64>((std::numeric_limits<int32>::max)());
				if (NewNum > MaxInt)
					NewNum = MaxInt;
				CurNum = static_cast<int32>(NewNum);
			}

			if (EnableNoDecrease && CurNum < PrevNum)
				CurNum = PrevNum;

			if (CurNum != Spec->Num)
				Spec->Num = CurNum;

			FItemInventorySnapshot Snapshot{};
			Snapshot.Num = CurNum;
			Snapshot.DefId = Spec->ItemDefId;
			GTab1ItemSnapshots[Key] = Snapshot;
		}

		for (auto It = GTab1ItemSnapshots.begin(); It != GTab1ItemSnapshots.end(); )
		{
			if (SeenKeys.find(It->first) != SeenKeys.end())
			{
				++It;
				continue;
			}

			if (EnableNoDecrease && It->second.Num > 0 && It->second.DefId > 0)
				UItemFuncLib::AddItem(It->second.DefId, It->second.Num);

			It = GTab1ItemSnapshots.erase(It);
		}
	}

	void ReadTab1ConfigFromUI(FTab1RuntimeConfig& Cfg)
	{
		Cfg.ItemNoDecrease = ReadToggleValue(GTab1ItemNoDecreaseToggle, Cfg.ItemNoDecrease);
		Cfg.ItemGainMultiplier = ReadToggleValue(GTab1ItemGainMultiplierToggle, Cfg.ItemGainMultiplier);
		Cfg.AllItemsSellable = ReadToggleValue(GTab1AllItemsSellableToggle, Cfg.AllItemsSellable);
		Cfg.IncludeQuestItems = ReadToggleValue(GTab1IncludeQuestItemsToggle, Cfg.IncludeQuestItems);
		Cfg.DropRate100 = ReadToggleValue(GTab1DropRate100Toggle, Cfg.DropRate100);
		Cfg.CraftEffectMultiplier = ReadToggleValue(GTab1CraftEffectMultiplierToggle, Cfg.CraftEffectMultiplier);
		Cfg.IgnoreItemUseCount = ReadToggleValue(GTab1IgnoreItemUseCountToggle, Cfg.IgnoreItemUseCount);
		Cfg.IgnoreItemRequirements = ReadToggleValue(GTab1IgnoreItemRequirementsToggle, Cfg.IgnoreItemRequirements);

		const float GainPercent = ReadSliderPercent(GTab1ItemGainMultiplierSlider, static_cast<float>(Cfg.ItemGainMultiplierValue));
		Cfg.ItemGainMultiplierValue = SliderPercentToIntMultiplier(GainPercent);

		const float IncrementPercent = ReadSliderPercent(
			GTab1CraftItemIncrementSlider,
			(Cfg.CraftItemIncrementMultiplier - 1.0f) / 0.09f);
		Cfg.CraftItemIncrementMultiplier = SliderPercentToFloatMultiplier(IncrementPercent);

		const float ExtraPercent = ReadSliderPercent(
			GTab1CraftExtraEffectSlider,
			(Cfg.CraftExtraEffectMultiplier - 1.0f) / 0.09f);
		Cfg.CraftExtraEffectMultiplier = SliderPercentToFloatMultiplier(ExtraPercent);

		Cfg.MaxExtraAffixes = ReadIntegerEditValue(GTab1MaxExtraAffixesEdit, Cfg.MaxExtraAffixes, 0, 32);
	}

	void PollAndApplyTab1Features(bool CanReadFromUI)
	{
		static FTab1RuntimeConfig Config{};
		if (CanReadFromUI)
			ReadTab1ConfigFromUI(Config);

		// 更新全局开关，供 ProcessEvent Hook 读取
		GItemNoDecreaseEnabled.store(Config.ItemNoDecrease, std::memory_order_release);

		UItemResManager* ResMgr = UManagerFuncLib::GetItemResManager();
		UDataTable* ItemTable = nullptr;
		UDataTable* DropTable = nullptr;
		UDataTable* RandTable = nullptr;
		if (ResMgr && IsSafeLiveObject(static_cast<UObject*>(ResMgr)))
		{
			ItemTable = ResMgr->ItemDataTable;
			DropTable = ResMgr->DropPoolDataTable;
			RandTable = ResMgr->RandActionPoolDataTable;
		}

		ResetTab1TableCachesIfNeeded(ItemTable, DropTable, RandTable);

		static DWORD LastInventoryTick = 0;
		const DWORD NowTick = GetTickCount();
		if (LastInventoryTick == 0 || (NowTick - LastInventoryTick) >= 80)
		{
			LastInventoryTick = NowTick;
			ApplyTab1BackpackFeatures(Config);
		}

		static DWORD LastTableTick = 0;
		if (LastTableTick == 0 || (NowTick - LastTableTick) >= 300)
		{
			LastTableTick = NowTick;
			ApplyTab1ItemDefinitionFeatures(Config, ItemTable);
			ApplyTab1DropPoolFeature(Config, DropTable);
			ApplyTab1RandPoolFeature(Config, RandTable);
		}
	}
}

void __fastcall HookedGVCPostRender(void* This, void* Canvas)
{
	PostRenderInFlightScope InFlightScope;

	// Call original first to preserve normal rendering
	if (OriginalGVCPostRender)
		OriginalGVCPostRender(This, Canvas);

	DrawFpsOverlay(static_cast<UCanvas*>(Canvas));

	if (GIsUnloading.load(std::memory_order_relaxed))
	{
		if (!GUnloadCleanupDone.exchange(true, std::memory_order_acq_rel))
		{
			APlayerController* PC = GetFirstLocalPlayerController();
			DestroyInternalWidget(PC);
			std::cout << "[SDK] UnloadCleanup: runtime UI cleanup done on game thread\n";
		}
		return;
	}

	// World/level transition guard:
	// avoid DestroyInternalWidget/ShowInternalWidget in PostRender during unstable frames.
	static UWorld* LastWorld = nullptr;
	static ULevel* LastLevel = nullptr;
	static int32 TransitionGuardFrames = 0;
	UWorld* CurrentWorld = UWorld::GetWorld();
	ULevel* CurrentLevel = CurrentWorld ? CurrentWorld->PersistentLevel : nullptr;
	if (CurrentWorld != LastWorld || CurrentLevel != LastLevel)
	{
		if (LastWorld != nullptr)
		{
			const bool HadWidget = (InternalWidget != nullptr) || InternalWidgetVisible;
			std::cout << "[SDK] WorldTransition: oldWorld=" << (void*)LastWorld
				<< " newWorld=" << (void*)CurrentWorld
				<< " oldLevel=" << (void*)LastLevel
				<< " newLevel=" << (void*)CurrentLevel
				<< " hadWidget=" << (HadWidget ? 1 : 0)
				<< " -> invalidate runtime state only\n";

			InternalWidget = nullptr;
			InternalWidgetVisible = false;
			GCachedBtnExit = nullptr;
			ClearRuntimeWidgetState();
			TransitionGuardFrames = 120;
		}
		LastWorld = CurrentWorld;
		LastLevel = CurrentLevel;
	}
	const bool InTransitionGuard = (TransitionGuardFrames > 0);
	if (TransitionGuardFrames > 0)
		--TransitionGuardFrames;

	EnsureMouseCursorVisible();

	// Edge-trigger HOME so one press toggles once
	static bool HomeWasDown = false;
	const bool HomeDown = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
	if (!InTransitionGuard && HomeDown && !HomeWasDown)
		ToggleInternalWidget();
	HomeWasDown = HomeDown;

	// PGUP: 输出当前世界状态
	static bool PGUPWasDown = false;
	const bool PGUPDown = (GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0;
	if (PGUPDown && !PGUPWasDown)
	{
		std::cout << "[SDK] PGUP pressed, checking world state...\n";
		UWorld* World = UWorld::GetWorld();
		if (World && World->OwningGameInstance)
		{
			EWorldStateType WorldState = UManagerFuncLib::GetWorldType();
			std::cout << "[SDK] WorldStateType: " << (int)WorldState << "\n";
			switch (WorldState)
			{
				case EWorldStateType::None: std::cout << "[SDK]   -> None (main menu)\n"; break;
				case EWorldStateType::Scene: std::cout << "[SDK]   -> Scene (world/menu scene)\n"; break;
				case EWorldStateType::IntoFight: std::cout << "[SDK]   -> IntoFight\n"; break;
				case EWorldStateType::Fighting: std::cout << "[SDK]   -> Fighting\n"; break;
				case EWorldStateType::IntoScene: std::cout << "[SDK]   -> IntoScene\n"; break;
				case EWorldStateType::CG: std::cout << "[SDK]   -> CG\n"; break;
				case EWorldStateType::ChangeScene: std::cout << "[SDK]   -> ChangeScene\n"; break;
				case EWorldStateType::SkipingCG: std::cout << "[SDK]   -> SkipingCG\n"; break;
				case EWorldStateType::GameSystemActived: std::cout << "[SDK]   -> GameSystemActived\n"; break;
				default: std::cout << "[SDK]   -> Unknown\n"; break;
			}

			// 获取当前关卡名称
			FString LevelName = UGameplayStatics::GetCurrentLevelName(World, false);
			const wchar_t* LevelNameWs = LevelName.CStr();
			if (LevelNameWs && LevelNameWs[0])
			{
				std::wcout << L"[SDK] LevelName: " << LevelNameWs << L"\n";
			}
			else
			{
				std::cout << "[SDK] LevelName: (empty)\n";
			}

			// 检查 PersistentLevel 是否存在
			if (World->PersistentLevel)
			{
				std::cout << "[SDK] PersistentLevel: " << (void*)World->PersistentLevel << "\n";
			}
		}
		else
		{
			std::cout << "[SDK] World or GameInstance is null\n";
		}
	}
	PGUPWasDown = PGUPDown;

	if (InTransitionGuard)
		return;

	const bool HasLiveInternalWidget = EnsureLiveInternalWidgetForFrame();
	UUserWidget* LiveInternalWidget = HasLiveInternalWidget ? InternalWidget : nullptr;
	UBPMV_ConfigView2_C* LiveConfigView = nullptr;
	int32 ActiveNativeTabIndex = -1;
	if (InternalWidgetVisible &&
		LiveInternalWidget &&
		LiveInternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass()))
	{
		LiveConfigView = static_cast<UBPMV_ConfigView2_C*>(LiveInternalWidget);
		if (LiveConfigView->CT_Contents &&
			IsSafeLiveObject(static_cast<UObject*>(LiveConfigView->CT_Contents)))
		{
			ActiveNativeTabIndex = LiveConfigView->CT_Contents->GetActiveWidgetIndex();
		}
	}
	const bool IsItemsTabActive = (ActiveNativeTabIndex == 1);
	const bool IsCharacterTabActive = (ActiveNativeTabIndex == 0);

	// Tab0（角色）编辑框：按 Enter 提交写回并回填。
	PollTab0CharacterInput(IsCharacterTabActive);

	// Detect BTN_Exit click (edge-triggered).
	// Only poll while panel is visible and pointer is valid to avoid unreachable UObject asserts.
	static bool ExitWasPressed = false;
	bool ExitPressed = false;
	const bool CanCheckExit =
		InternalWidgetVisible &&
		LiveInternalWidget &&
		LiveInternalWidget->IsInViewport() &&
		LiveInternalWidget->IsA(UBPMV_ConfigView2_C::StaticClass());

	if (CanCheckExit)
	{
		auto* CV = static_cast<UBPMV_ConfigView2_C*>(LiveInternalWidget);
		GCachedBtnExit = CV ? CV->BTN_Exit : nullptr;

		if (GCachedBtnExit)
		{
			auto* ExitObj = static_cast<UObject*>(GCachedBtnExit);
			if (IsSafeLiveObject(ExitObj))
				ExitPressed = GCachedBtnExit->IsPressed();
			else
				GCachedBtnExit = nullptr;
		}
	}
	else
	{
		GCachedBtnExit = nullptr;
		ExitWasPressed = false;
	}

	if (ExitWasPressed && !ExitPressed && CanCheckExit)
	{
		APlayerController* PC = GetFirstLocalPlayerController();
		if (PC)
			HideInternalWidget(PC);
	}
	ExitWasPressed = ExitPressed;

	// 鈹€鈹€ Item browser per-frame polling 鈹€鈹€
	static DWORD sLastItemUiPollTick = 0;
	static bool sItemGridClickWasDown = false;
	static int32 sItemGridPressedSlot = -1;
	if (InternalWidgetVisible && LiveInternalWidget && IsItemsTabActive)
	{
		const DWORD ItemUiNow = GetTickCount();
		const bool RunItemUiPoll = (sLastItemUiPollTick == 0) || ((ItemUiNow - sLastItemUiPollTick) >= 16);
		if (RunItemUiPoll)
		{
			sLastItemUiPollTick = ItemUiNow;
			PollCollapsiblePanelsInput();

			if (GVolumeLastValues.size() != GVolumeItems.size())
				GVolumeLastValues.resize(GVolumeItems.size(), 0.0f);
			if (GVolumeMinusWasPressed.size() != GVolumeItems.size())
				GVolumeMinusWasPressed.resize(GVolumeItems.size(), false);
			if (GVolumePlusWasPressed.size() != GVolumeItems.size())
				GVolumePlusWasPressed.resize(GVolumeItems.size(), false);

			for (size_t i = 0; i < GVolumeItems.size(); ++i)
			{
				auto* Item = GVolumeItems[i];
				if (!IsSafeLiveObject(static_cast<UObject*>(Item)))
					continue;

				auto* Slider = Item->VolumeSlider;
				if (!IsSafeLiveObject(static_cast<UObject*>(Slider)))
					continue;

				bool MinusPressed = false;
				bool PlusPressed = false;
				if (Item->BTN_Minus && IsSafeLiveObject(static_cast<UObject*>(Item->BTN_Minus)))
					MinusPressed = Item->BTN_Minus->IsPressed();
				if (Item->BTN_Plus && IsSafeLiveObject(static_cast<UObject*>(Item->BTN_Plus)))
					PlusPressed = Item->BTN_Plus->IsPressed();

				const bool MinusClicked = GVolumeMinusWasPressed[i] && !MinusPressed;
				const bool PlusClicked = GVolumePlusWasPressed[i] && !PlusPressed;

				float CurValue = Slider->GetValue();
				bool bValueChanged = std::fabs(CurValue - GVolumeLastValues[i]) > 0.0001f;

				if (MinusClicked || PlusClicked)
				{
					float Step = Slider->StepSize;
					if (Step <= 0.0001f)
						Step = 0.01f;

					float NewValue = CurValue + (PlusClicked ? Step : 0.0f) - (MinusClicked ? Step : 0.0f);
					float MinValue = Slider->MinValue;
					float MaxValue = Slider->MaxValue;
					if (MaxValue < MinValue)
					{
						const float Tmp = MinValue;
						MinValue = MaxValue;
						MaxValue = Tmp;
					}

					if (NewValue < MinValue) NewValue = MinValue;
					if (NewValue > MaxValue) NewValue = MaxValue;

					if (std::fabs(NewValue - CurValue) > 0.0001f)
					{
						Slider->SetValue(NewValue);
						CurValue = NewValue;
						bValueChanged = true;
					}
				}

				if (bValueChanged)
				{
					CurValue = Slider->GetValue();
					if (Item->TXT_CurrentValue)
					{
						float MinValue = Slider->MinValue;
						float MaxValue = Slider->MaxValue;
						float Norm = CurValue;
						if (MaxValue > MinValue)
							Norm = (CurValue - MinValue) / (MaxValue - MinValue);
						if (Norm < 0.0f) Norm = 0.0f;
						if (Norm > 1.0f) Norm = 1.0f;
						const int32 DisplayValue = static_cast<int32>(Norm * 100.0f + 0.5f);
						wchar_t Buf[16] = {};
						swprintf_s(Buf, 16, L"%d", DisplayValue);
						Item->TXT_CurrentValue->SetText(MakeText(Buf));
					}
				}

				GVolumeLastValues[i] = CurValue;
				GVolumeMinusWasPressed[i] = MinusPressed;
				GVolumePlusWasPressed[i] = PlusPressed;
			}

			static DWORD LastItemCacheRetryTick = 0;
			if (!GItemCacheBuilt && (GItemCategoryDD || GItemGridPanel))
			{
				DWORD NowTick = GetTickCount();
				if (NowTick - LastItemCacheRetryTick > 1000)
				{
					LastItemCacheRetryTick = NowTick;
					BuildItemCache();
					if (GItemCacheBuilt)
					{
						int32 CurrentCat = 0;
						if (GItemCategoryDD && GItemCategoryDD->CB_Main)
						{
							int32 SelectedCat = GetComboSelectedIndexSafe(GItemCategoryDD->CB_Main);
							if (SelectedCat >= 0)
								CurrentCat = SelectedCat;
						}
						GItemCurrentPage = 0;
						GItemLastCatIdx = CurrentCat;
						FilterItems(CurrentCat);
						RefreshItemPage();
					}
				}
			}

			if (GItemCategoryDD && GItemCategoryDD->CB_Main)
			{
				int32 catIdx = GetComboSelectedIndexSafe(GItemCategoryDD->CB_Main);
				if (catIdx != GItemLastCatIdx && catIdx >= 0)
				{
					GItemLastCatIdx = catIdx;
					GItemCurrentPage = 0;
					FilterItems(catIdx);
					RefreshItemPage();
				}
			}

			GItemAddQuantity = GetItemAddQuantityFromEdit();

			auto GetClickableButton = [](UJHCommon_Btn_Free_C* W) -> UButton* {
				if (!W) return nullptr;
				if (W->Btn) return W->Btn;
				if (W->JHGPCBtn)
					return static_cast<UJHNeoUIGamepadConfirmButton*>(W->JHGPCBtn)->BtnMain;
				return nullptr;
			};

			UButton* PrevInner = GetClickableButton(GItemPrevPageBtn);
			bool PrevPressed = PrevInner &&
				IsSafeLiveObject(static_cast<UObject*>(PrevInner)) &&
				PrevInner->IsPressed();
			if (GItemPrevWasPressed && !PrevPressed && GItemCurrentPage > 0)
			{
				GItemCurrentPage--;
				RefreshItemPage();
			}
			GItemPrevWasPressed = PrevPressed;

			UButton* NextInner = GetClickableButton(GItemNextPageBtn);
			bool NextPressed = NextInner &&
				IsSafeLiveObject(static_cast<UObject*>(NextInner)) &&
				NextInner->IsPressed();
			if (GItemNextWasPressed && !NextPressed && (GItemCurrentPage + 1) < GItemTotalPages)
			{
				GItemCurrentPage++;
				RefreshItemPage();
			}
			GItemNextWasPressed = NextPressed;

			auto AddItemBySlot = [&](int32 Slot, const char* TriggerTag) -> bool
			{
				if (Slot < 0 || Slot >= ITEMS_PER_PAGE)
					return false;

				const int32 itemIdx = GItemSlotItemIndices[Slot];
				if (itemIdx < 0 || itemIdx >= static_cast<int32>(GAllItems.size()))
					return false;

				const int32 Quantity = (GItemAddQuantity > 0) ? GItemAddQuantity : 1;
				CachedItem& item = GAllItems[itemIdx];
				UItemFuncLib::AddItem(item.DefId, Quantity);
				std::cout << "[SDK] AddItem(" << (TriggerTag ? TriggerTag : "unknown")
					<< "): slot=" << Slot
					<< " defId=" << item.DefId
					<< " x" << Quantity << "\n";
				return true;
			};

			bool AddedByButtonEdge = false;
			for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
			{
				auto* Btn = GItemSlotButtons[i];
				if (!IsSafeLiveObject(static_cast<UObject*>(Btn)))
					continue;

				bool Pressed = Btn->IsPressed();
				if (GItemSlotWasPressed[i] && !Pressed)
				{
					AddedByButtonEdge = AddItemBySlot(i, "button-edge") || AddedByButtonEdge;
				}
				GItemSlotWasPressed[i] = Pressed;
			}

			auto ResolveHoveredGridSlot = [&]() -> int32
			{
				if (!GItemGridPanel || !IsSafeLiveObject(static_cast<UObject*>(GItemGridPanel)))
					return -1;

				UWorld* World = UWorld::GetWorld();
				if (!World)
					return -1;

				UObject* ViewportContext = static_cast<UObject*>(World);
				const FGeometry GridGeo = GItemGridPanel->GetCachedGeometry();
				const FVector2D GridSize = USlateBlueprintLibrary::GetLocalSize(GridGeo);
				if (GridSize.X <= 1.0f || GridSize.Y <= 1.0f)
					return -1;

				FVector2D PixelTL{}, ViewTL{}, PixelBR{}, ViewBR{};
				USlateBlueprintLibrary::LocalToViewport(
					ViewportContext, GridGeo, FVector2D{ 0.0f, 0.0f }, &PixelTL, &ViewTL);
				USlateBlueprintLibrary::LocalToViewport(
					ViewportContext, GridGeo, GridSize, &PixelBR, &ViewBR);

				const float MinX = (ViewTL.X < ViewBR.X) ? ViewTL.X : ViewBR.X;
				const float MaxX = (ViewTL.X > ViewBR.X) ? ViewTL.X : ViewBR.X;
				const float MinY = (ViewTL.Y < ViewBR.Y) ? ViewTL.Y : ViewBR.Y;
				const float MaxY = (ViewTL.Y > ViewBR.Y) ? ViewTL.Y : ViewBR.Y;

				const FVector2D MouseVP = UWidgetLayoutLibrary::GetMousePositionOnViewport(ViewportContext);
				if (MouseVP.X < MinX || MouseVP.X > MaxX || MouseVP.Y < MinY || MouseVP.Y > MaxY)
					return -1;

				const float GridW = MaxX - MinX;
				const float GridH = MaxY - MinY;
				if (GridW <= 1.0f || GridH <= 1.0f)
					return -1;

				const int32 Col = static_cast<int32>(((MouseVP.X - MinX) / GridW) * ITEM_GRID_COLS);
				const int32 Row = static_cast<int32>(((MouseVP.Y - MinY) / GridH) * ITEM_GRID_ROWS);
				if (Col < 0 || Col >= ITEM_GRID_COLS || Row < 0 || Row >= ITEM_GRID_ROWS)
					return -1;

				const int32 Slot = Row * ITEM_GRID_COLS + Col;
				if (Slot < 0 || Slot >= ITEMS_PER_PAGE)
					return -1;

				const int32 itemIdx = GItemSlotItemIndices[Slot];
				if (itemIdx < 0 || itemIdx >= static_cast<int32>(GAllItems.size()))
					return -1;
				return Slot;
			};

			// 兜底：某些 BPEntry_Item 组合下 BtnMain->IsPressed 边沿不稳定，改用网格命中点击判定补齐。
			const bool LeftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			const int32 HoveredSlot = ResolveHoveredGridSlot();
			if (LeftDown && !sItemGridClickWasDown)
			{
				sItemGridPressedSlot = HoveredSlot;
			}
			else if (!LeftDown && sItemGridClickWasDown)
			{
				if (!AddedByButtonEdge &&
					sItemGridPressedSlot >= 0 &&
					HoveredSlot >= 0 &&
					HoveredSlot == sItemGridPressedSlot)
				{
					AddItemBySlot(HoveredSlot, "grid-hit");
				}
				sItemGridPressedSlot = -1;
			}
			sItemGridClickWasDown = LeftDown;
		}
	}
	else
	{
		sLastItemUiPollTick = 0;
		sItemGridClickWasDown = false;
		sItemGridPressedSlot = -1;
		GItemPrevWasPressed = false;
		GItemNextWasPressed = false;
		for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
			GItemSlotWasPressed[i] = false;
	}

	const bool CanReadTab1FromUI =
		InternalWidgetVisible &&
		LiveInternalWidget &&
		IsItemsTabActive;
	PollAndApplyTab1Features(CanReadTab1FromUI);

	// Hover tips polling:
	// 1) 物品 Tab 内高频轮询（节流到约 60Hz）
	// 2) 非物品 Tab 仅在已有 tips 残留时低频轮询，用于快速收口隐藏
	bool HoverGridValid = false;
	if (GItemGridPanel)
	{
		auto* GridObj = static_cast<UObject*>(GItemGridPanel);
		HoverGridValid = IsSafeLiveObject(GridObj);
	}
	const bool HasActiveHoverTips =
		(GItemHoveredSlot >= 0) ||
		(GItemHoverTipsWidget && IsSafeLiveObject(static_cast<UObject*>(GItemHoverTipsWidget)));
	const bool CanPollHover =
		InternalWidgetVisible &&
		LiveInternalWidget &&
		HoverGridValid &&
		(IsItemsTabActive || HasActiveHoverTips);

	if (CanPollHover)
	{
		static DWORD sLastHoverPollTick = 0;
		const DWORD HoverPollNow = GetTickCount();
		const DWORD PollIntervalMs = IsItemsTabActive ? (HasActiveHoverTips ? 33 : 20) : 80;
		if (sLastHoverPollTick == 0 || (HoverPollNow - sLastHoverPollTick) >= PollIntervalMs)
		{
			sLastHoverPollTick = HoverPollNow;
			// 临时禁用：用于验证物品 Tab 悬浮 Tip 轮询是否为卡顿主因
			// PollItemBrowserHoverTips();
		}
	}

	// 鈹€鈹€ Dynamic tab button click detection 鈹€鈹€
	if (InternalWidgetVisible && LiveConfigView)
	{
		auto* CV = LiveConfigView;
		auto IsLiveTabBtn = [](UBP_JHConfigTabBtn_C* Btn) -> bool
		{
			return IsSafeLiveObject(static_cast<UObject*>(Btn));
		};

		// Native tabs fully handle themselves via AutoFocusForMouseEntering 鈫?
		// HandleMainBtn() 鈫?EVT_SyncTabIndex(). We only manage dynamic tab
		// content visibility (VBoxes outside the Switcher) and active state.
		static int32 sActiveDynTab = -1;

		int32 dynHoverIdx = -1;
		if      (IsLiveTabBtn(GDynTabBtn6) && GDynTabBtn6->IsHovered()) dynHoverIdx = 6;
		else if (IsLiveTabBtn(GDynTabBtn7) && GDynTabBtn7->IsHovered()) dynHoverIdx = 7;
		else if (IsLiveTabBtn(GDynTabBtn8) && GDynTabBtn8->IsHovered()) dynHoverIdx = 8;

		if (dynHoverIdx >= 6 && dynHoverIdx != sActiveDynTab)
		{
			// Entering a (different) dynamic tab 鈥?show its content
			ShowDynamicTab(CV, dynHoverIdx);
			if (IsLiveTabBtn(GDynTabBtn6)) GDynTabBtn6->EVT_UpdateActiveStatus(dynHoverIdx == 6);
			if (IsLiveTabBtn(GDynTabBtn7)) GDynTabBtn7->EVT_UpdateActiveStatus(dynHoverIdx == 7);
			if (IsLiveTabBtn(GDynTabBtn8)) GDynTabBtn8->EVT_UpdateActiveStatus(dynHoverIdx == 8);
			sActiveDynTab = dynHoverIdx;
		}
		else if (sActiveDynTab >= 6 && dynHoverIdx == -1)
		{
			// Only restore original content when a NATIVE tab is hovered.
			// Moving mouse to content area or empty space keeps dynamic tab active
			// 鈥?same behavior as native tabs.
			bool nativeHovered =
				(CV->BTN_Sound && IsSafeLiveObject(static_cast<UObject*>(CV->BTN_Sound)) && CV->BTN_Sound->IsHovered()) ||
				(CV->BTN_Video && IsSafeLiveObject(static_cast<UObject*>(CV->BTN_Video)) && CV->BTN_Video->IsHovered()) ||
				(CV->BTN_Keys && IsSafeLiveObject(static_cast<UObject*>(CV->BTN_Keys)) && CV->BTN_Keys->IsHovered()) ||
				(CV->BTN_Lan && IsSafeLiveObject(static_cast<UObject*>(CV->BTN_Lan)) && CV->BTN_Lan->IsHovered()) ||
				(CV->BTN_Others && IsSafeLiveObject(static_cast<UObject*>(CV->BTN_Others)) && CV->BTN_Others->IsHovered()) ||
				(CV->BTN_Gamepad && IsSafeLiveObject(static_cast<UObject*>(CV->BTN_Gamepad)) && CV->BTN_Gamepad->IsHovered());
			if (nativeHovered)
			{
				ShowOriginalTab(CV);
				if (IsLiveTabBtn(GDynTabBtn6)) GDynTabBtn6->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTabBtn7)) GDynTabBtn7->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTabBtn8)) GDynTabBtn8->EVT_UpdateActiveStatus(false);
				sActiveDynTab = -1;
			}
		}
	}

	// Detect if blueprint logic closed the widget externally
	if (InternalWidgetVisible && LiveInternalWidget && !LiveInternalWidget->IsInViewport())
	{
		APlayerController* PC = GetFirstLocalPlayerController();
		HideInternalWidget(PC);
		std::cout << "[SDK] Widget closed externally, cached instance kept\n";
	}
}

// 物品管理器 ProcessEvent Hook：拦截 ChangeItemNum 实现物品不减
void __stdcall HookedProcessEvent(void* This, void* Function, void* Parms)
{
	// 检查是否是物品不减功能启用
	if (GItemNoDecreaseEnabled.load(std::memory_order_acquire))
	{
		// 获取 Function 对象
		UFunction* Func = reinterpret_cast<UFunction*>(Function);
		if (Func)
		{
			// 检查函数名是否是 ChangeItemNum
			std::string FuncName = Func->GetName();
			if (FuncName == "ChangeItemNum")
			{
				// 使用 SDK 中的参数结构访问参数 (SDK::Params 命名空间)
				auto* Params = reinterpret_cast<SDK::Params::ItemManager_ChangeItemNum*>(Parms);

				// 只有当 Num < 0 时才拦截（减少物品）
				if (Params->Num < 0)
				{
					// 设置返回值为 true（表示操作成功），但不执行原函数
					Params->ReturnValue = true;
					return;
				}
			}
		}
	}

	// 未拦截，调用原始函数
	if (OriginalProcessEvent)
		OriginalProcessEvent(This, Function, Parms);
}

// -- Main Thread --
