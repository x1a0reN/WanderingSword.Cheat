#include <Windows.h>
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
#include "SDK/NeoUI_classes.hpp"
#include "SDK/Engine_classes.hpp"
#include "Logging.hpp"

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

		LOGI_STREAM("FrameHook") << "[SDK] FrameHook: stale internal widget pointer detected, reset state\n";
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
		int32 ItemGainMultiplierValue = 2;
		bool AllItemsSellable = false;
		bool DropRate100 = false;
		bool CraftEffectMultiplier = false;
		float CraftItemIncrementMultiplier = 2.0f;
		float CraftExtraEffectMultiplier = 2.0f;
		bool MaxExtraAffixes = false;
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

	using UObjectProcessEventFn = void(__fastcall*)(const UObject* /* This */, UFunction* /* Function */, void* /* Params */);
	VTableHook GItemEntryProcessEventHook;
	UObjectProcessEventFn GOriginalItemEntryProcessEvent = nullptr;
	std::atomic<int32> GPendingItemEntryClickDefId = 0;
	constexpr bool kVerboseItemEntryProcessEventLogs = false;
	constexpr bool kEnableBackpackViewProcessEventHook = false;
	VTableHook GBackpackViewProcessEventHook;
	UObjectProcessEventFn GOriginalBackpackViewProcessEvent = nullptr;
	constexpr bool kVerboseBackpackViewProcessEventLogs = false;
	constexpr uintptr_t kBackpackGridCtxWeakOffset = 0x8A0;
	constexpr int32 kBackpackCtxSourceAnchorIndex = 59720;
	constexpr int32 kBackpackCtxSourceWindow = 10;
	std::unordered_map<std::string, int32> GBackpackPECallCounts;
	std::unordered_set<std::string> GBackpackPESeenFuncs;
	DWORD GBackpackPELastDumpTick = 0;
	FWeakObjectPtr GLastCapturedBackpackCtxWeak{};
	uintptr_t GLastCapturedBackpackGrid = 0;
	int32 GBackpackCtxSourceGridIndex = -1;
	int32 GBackpackCtxSourceGridDistance = (std::numeric_limits<int32>::max)();
	bool GBackpackCtxSourceGridExact = false;

	bool IsInterestingBackpackPEName(const std::string& Name)
	{
		return
			(Name.find("Backpack") != std::string::npos) ||
			(Name.find("EVT_") != std::string::npos) ||
			(Name.find("SetupHierarchy") != std::string::npos) ||
			(Name.find("RenderView") != std::string::npos) ||
			(Name.find("OnEntry") != std::string::npos) ||
			(Name.find("Handle") != std::string::npos);
	}

	FWeakObjectPtr ReadWeakAt(UObject* Obj, uintptr_t Offset)
	{
		FWeakObjectPtr Out{};
		if (!Obj)
			return Out;
		const uint8* Base = reinterpret_cast<const uint8*>(Obj);
		Out = *reinterpret_cast<const FWeakObjectPtr*>(Base + Offset);
		return Out;
	}

	bool IsSameWeak(const FWeakObjectPtr& A, const FWeakObjectPtr& B)
	{
		return A.ObjectIndex == B.ObjectIndex &&
			A.ObjectSerialNumber == B.ObjectSerialNumber;
	}

	void ResetBackpackCtxCaptureState()
	{
		GLastCapturedBackpackCtxWeak = {};
		GLastCapturedBackpackGrid = 0;
		GBackpackCtxSourceGridIndex = -1;
		GBackpackCtxSourceGridDistance = (std::numeric_limits<int32>::max)();
		GBackpackCtxSourceGridExact = false;
	}

	bool ShouldCaptureBackpackCtxFromGrid(UObject* GridObj, int32& OutGridIndex, int32& OutDistance, bool& OutExact)
	{
		OutGridIndex = -1;
		OutDistance = (std::numeric_limits<int32>::max)();
		OutExact = false;
		if (!IsSafeLiveObject(GridObj) || GridObj->IsDefaultObject())
			return false;

		OutGridIndex = GridObj->Index;
		if (OutGridIndex < 0)
			return false;

		OutDistance = std::abs(OutGridIndex - kBackpackCtxSourceAnchorIndex);
		OutExact = (OutGridIndex == kBackpackCtxSourceAnchorIndex);
		if (OutDistance > kBackpackCtxSourceWindow)
			return false;

		if (GBackpackCtxSourceGridExact && !OutExact && GBackpackCtxSourceGridIndex != OutGridIndex)
			return false;
		if (GBackpackCtxSourceGridIndex < 0 || GBackpackCtxSourceGridIndex == OutGridIndex || OutExact)
			return true;
		return OutDistance < GBackpackCtxSourceGridDistance;
	}

	void DumpBackpackPECountsIfNeeded()
	{
		const DWORD Now = GetTickCount();
		if (GBackpackPELastDumpTick != 0 && (Now - GBackpackPELastDumpTick) < 1000)
			return;
		GBackpackPELastDumpTick = Now;

		if (GBackpackPECallCounts.empty())
			return;

		std::vector<std::pair<std::string, int32>> Pairs;
		Pairs.reserve(GBackpackPECallCounts.size());
		for (const auto& KV : GBackpackPECallCounts)
			Pairs.emplace_back(KV.first, KV.second);
		std::sort(Pairs.begin(), Pairs.end(),
			[](const auto& A, const auto& B) { return A.second > B.second; });

		const int32 DumpNum = (static_cast<int32>(Pairs.size()) < 6) ? static_cast<int32>(Pairs.size()) : 6;
		for (int32 i = 0; i < DumpNum; ++i)
		{
			LOGI_STREAM("FrameHook")
				<< "[SDK] BackpackView ProcessEvent hot[" << i << "]: func="
				<< Pairs[static_cast<size_t>(i)].first
				<< " count=" << Pairs[static_cast<size_t>(i)].second
				<< "\n";
		}
		GBackpackPECallCounts.clear();
	}

	bool TryQueueDefIdFromItemEntry(UObject* EntryObj, const std::string& TriggerFunc)
	{
		if (!IsSafeLiveObjectOfClass(EntryObj, UJHNeoUIItemEntryWDT::StaticClass()))
		{
			if (kVerboseItemEntryProcessEventLogs)
			{
				LOGI_STREAM("FrameHook") << "[SDK] ItemEntry click ignored: invalid entry, trigger="
					<< TriggerFunc << " this=" << (void*)EntryObj << "\n";
			}
			return false;
		}

		auto* Entry = static_cast<UJHNeoUIItemEntryWDT*>(EntryObj);
		UObject* BindObj = Entry->GetBindedDataObj();
		if (!IsSafeLiveObjectOfClass(BindObj, UItemInfoSpec::StaticClass()))
		{
			if (kVerboseItemEntryProcessEventLogs)
			{
				LOGI_STREAM("FrameHook") << "[SDK] ItemEntry click ignored: bindObj invalid, trigger="
					<< TriggerFunc << " this=" << (void*)EntryObj << " bindObj=" << (void*)BindObj << "\n";
			}
			return false;
		}

		auto* Spec = static_cast<UItemInfoSpec*>(BindObj);
		if (!Spec)
		{
			if (kVerboseItemEntryProcessEventLogs)
			{
				LOGI_STREAM("FrameHook") << "[SDK] ItemEntry click ignored: spec null, trigger="
					<< TriggerFunc << " this=" << (void*)EntryObj << "\n";
			}
			return false;
		}

		const int32 DefId = Spec->ItemDefId;
		if (DefId <= 0)
		{
			if (kVerboseItemEntryProcessEventLogs)
			{
				LOGI_STREAM("FrameHook") << "[SDK] ItemEntry click ignored: invalid defId="
					<< DefId << " trigger=" << TriggerFunc << " this=" << (void*)EntryObj << "\n";
			}
			return false;
		}

		GPendingItemEntryClickDefId.store(DefId, std::memory_order_release);
		if (kVerboseItemEntryProcessEventLogs)
		{
			LOGI_STREAM("FrameHook") << "[SDK] ItemEntry click queued: defId=" << DefId
				<< " trigger=" << TriggerFunc << " this=" << (void*)EntryObj << "\n";
		}
		return true;
	}

	void __fastcall HookedItemEntryProcessEvent(const UObject* ThisObj, UFunction* Function, void* Params)
	{
		if (ThisObj && Function && InternalWidgetVisible)
		{
			const std::string FuncName = Function->GetName();
			if (kVerboseItemEntryProcessEventLogs)
			{
				LOGI_STREAM("FrameHook") << "[SDK] ItemEntry ProcessEvent: func=" << FuncName
					<< " this=" << (void*)ThisObj << "\n";
			}

			// 只吃 ItemEntry 左键点击事件，上一页/下一页不会走到这里。
			const bool IsClickedDelegate =
				(FuncName.find("JHNeoUIGamepadConfirmButtonClicked__DelegateSignature") != std::string::npos);
			const bool IsDoubleClickedDelegate =
				(FuncName.find("JHNeoUIGamepadConfirmButtonDoubleClicked__DelegateSignature") != std::string::npos);
			if (IsClickedDelegate || IsDoubleClickedDelegate || FuncName == "HandleCommonSingleClick")
				TryQueueDefIdFromItemEntry(const_cast<UObject*>(ThisObj), FuncName);
		}

		if (GOriginalItemEntryProcessEvent)
			GOriginalItemEntryProcessEvent(ThisObj, Function, Params);
	}

	UObject* FindLiveItemEntryForHook()
	{
		for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
		{
			UObject* Obj = static_cast<UObject*>(GItemSlotEntryWidgets[i]);
			if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemEntryWDT::StaticClass()))
				return Obj;
		}

		UListView* ListView = GItemListView;
		if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(ListView), UListView::StaticClass()))
			return nullptr;

		const int32 ActiveNum = ListView->EntryWidgetPool.ActiveWidgets.Num();
		for (int32 i = 0; i < ActiveNum; ++i)
		{
			UObject* Obj = static_cast<UObject*>(ListView->EntryWidgetPool.ActiveWidgets[i]);
			if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemEntryWDT::StaticClass()))
				return Obj;
		}

		const int32 InactiveNum = ListView->EntryWidgetPool.InactiveWidgets.Num();
		for (int32 i = 0; i < InactiveNum; ++i)
		{
			UObject* Obj = static_cast<UObject*>(ListView->EntryWidgetPool.InactiveWidgets[i]);
			if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemEntryWDT::StaticClass()))
				return Obj;
		}

		return nullptr;
	}

	bool EnsureItemEntryProcessEventHookInstalled()
	{
		if (GOriginalItemEntryProcessEvent)
			return true;

		UObject* EntryObj = FindLiveItemEntryForHook();
		if (!EntryObj)
			return false;

		GItemEntryProcessEventHook = VTableHook(EntryObj, Offsets::ProcessEventIdx);
		GOriginalItemEntryProcessEvent =
			GItemEntryProcessEventHook.Install<UObjectProcessEventFn>(HookedItemEntryProcessEvent);
		if (!GOriginalItemEntryProcessEvent)
		{
			LOGE_STREAM("FrameHook") << "[SDK] ItemEntry ProcessEvent hook install failed (idx="
				<< Offsets::ProcessEventIdx << ")\n";
			return false;
		}

		LOGI_STREAM("FrameHook") << "[SDK] ItemEntry ProcessEvent hook installed (idx="
			<< Offsets::ProcessEventIdx << ")\n";
		return true;
	}

	bool TryConsumeItemEntryClickDefId(int32& OutDefId)
	{
		OutDefId = GPendingItemEntryClickDefId.exchange(0, std::memory_order_acq_rel);
		return OutDefId > 0;
	}

	void __fastcall HookedBackpackViewProcessEvent(const UObject* ThisObj, UFunction* Function, void* Params)
	{
		if (ThisObj && Function)
		{
			UObject* MutableThis = const_cast<UObject*>(ThisObj);
			if (IsSafeLiveObjectOfClass(MutableThis, UJHBackpackViewBase::StaticClass()))
			{
				const std::string FuncName = Function->GetName();
				++GBackpackPECallCounts[FuncName];

				// Core path: continuously capture ctx from backpack RenderView.
				if (FuncName == "EVT_RenderView")
				{
					auto* View = static_cast<UJHBackpackViewBase*>(MutableThis);
					UObject* GridObj = static_cast<UObject*>(View->GRID_ITEM);
					int32 GridIndex = -1;
					int32 GridDist = (std::numeric_limits<int32>::max)();
					bool bGridExact = false;
					if (ShouldCaptureBackpackCtxFromGrid(GridObj, GridIndex, GridDist, bGridExact))
					{
						FWeakObjectPtr CtxWeak = ReadWeakAt(GridObj, kBackpackGridCtxWeakOffset);
						if (CtxWeak.ObjectIndex >= 0 && CtxWeak.ObjectSerialNumber > 0)
						{
							UObject* CtxObj = CtxWeak.Get();
							if (!IsSafeLiveObject(CtxObj))
								CtxObj = nullptr;

							const bool SourceChanged =
								(GBackpackCtxSourceGridIndex != GridIndex) ||
								(GBackpackCtxSourceGridExact != bGridExact);
							GBackpackCtxSourceGridIndex = GridIndex;
							GBackpackCtxSourceGridDistance = GridDist;
							GBackpackCtxSourceGridExact = bGridExact;

							CacheEntryInitContextWeakB(CtxWeak, "BackpackView.EVT_RenderView");
							const uintptr_t GridAddr = reinterpret_cast<uintptr_t>(GridObj);
							if (!IsSameWeak(CtxWeak, GLastCapturedBackpackCtxWeak) ||
								GLastCapturedBackpackGrid != GridAddr ||
								SourceChanged)
							{
								GLastCapturedBackpackCtxWeak = CtxWeak;
								GLastCapturedBackpackGrid = GridAddr;
								LOGI_STREAM("FrameHook")
									<< "[SDK] BackpackView ctx captured: func=EVT_RenderView"
									<< " this=0x" << std::hex << reinterpret_cast<uintptr_t>(MutableThis)
									<< " grid=0x" << GridAddr
									<< std::dec
									<< " gridIndex=" << GridIndex
									<< " anchorDiff=" << GridDist
									<< " exact=" << (bGridExact ? 1 : 0)
									<< " ctxWeak=(" << CtxWeak.ObjectIndex << "," << CtxWeak.ObjectSerialNumber << ")"
									<< " ctxObj=0x" << std::hex << reinterpret_cast<uintptr_t>(CtxObj)
									<< std::dec
									<< "\n";
							}
						}
					}
				}

				if (kVerboseBackpackViewProcessEventLogs)
				{
					if (GBackpackPESeenFuncs.insert(FuncName).second && IsInterestingBackpackPEName(FuncName))
					{
						LOGI_STREAM("FrameHook")
							<< "[SDK] BackpackView ProcessEvent first-seen: func="
							<< FuncName
							<< " this=0x" << std::hex << reinterpret_cast<uintptr_t>(MutableThis)
							<< std::dec
							<< "\n";
					}

					if (IsInterestingBackpackPEName(FuncName))
					{
						auto* View = static_cast<UJHBackpackViewBase*>(MutableThis);
						UObject* GridObj = static_cast<UObject*>(View->GRID_ITEM);
						FWeakObjectPtr CtxWeak = ReadWeakAt(GridObj, kBackpackGridCtxWeakOffset);
						UObject* CtxObj = CtxWeak.Get();
						if (!IsSafeLiveObject(CtxObj))
							CtxObj = nullptr;

						LOGI_STREAM("FrameHook")
							<< "[SDK] BackpackView PE: func=" << FuncName
							<< " this=0x" << std::hex << reinterpret_cast<uintptr_t>(MutableThis)
							<< " params=0x" << reinterpret_cast<uintptr_t>(Params)
							<< " grid=0x" << reinterpret_cast<uintptr_t>(GridObj)
							<< std::dec
							<< " ctxWeak=(" << CtxWeak.ObjectIndex << "," << CtxWeak.ObjectSerialNumber << ")"
							<< " ctxObj=0x" << std::hex << reinterpret_cast<uintptr_t>(CtxObj)
							<< std::dec
							<< "\n";
					}

					DumpBackpackPECountsIfNeeded();
				}
			}
		}

		if (GOriginalBackpackViewProcessEvent)
			GOriginalBackpackViewProcessEvent(ThisObj, Function, Params);
	}

	UObject* FindLiveBackpackViewForHook()
	{
		auto* ObjArray = UObject::GObjects.GetTypedPtr();
		if (!ObjArray)
			return nullptr;

		UObject* Fallback = nullptr;
		const int32 Num = ObjArray->Num();
		for (int32 i = 0; i < Num; ++i)
		{
			UObject* Obj = ObjArray->GetByIndex(i);
			if (!IsSafeLiveObjectOfClass(Obj, UJHBackpackViewBase::StaticClass()))
				continue;
			if (Obj->IsDefaultObject())
				continue;

			if (!Fallback)
				Fallback = Obj;

			auto* View = static_cast<UJHBackpackViewBase*>(Obj);
			UObject* GridObj = static_cast<UObject*>(View->GRID_ITEM);
			if (IsSafeLiveObjectOfClass(GridObj, UJHNeoUIItemGrid::StaticClass()))
				return Obj;
		}
		return Fallback;
	}

	bool EnsureBackpackViewProcessEventHookInstalled()
	{
		if (GOriginalBackpackViewProcessEvent)
			return true;

		UObject* BackpackObj = FindLiveBackpackViewForHook();
		if (!BackpackObj)
			return false;

		GBackpackViewProcessEventHook = VTableHook(BackpackObj, Offsets::ProcessEventIdx);
		GOriginalBackpackViewProcessEvent =
			GBackpackViewProcessEventHook.Install<UObjectProcessEventFn>(HookedBackpackViewProcessEvent);
		if (!GOriginalBackpackViewProcessEvent)
		{
			LOGE_STREAM("FrameHook") << "[SDK] BackpackView ProcessEvent hook install failed (idx="
				<< Offsets::ProcessEventIdx << ")\n";
			return false;
		}

		LOGI_STREAM("FrameHook") << "[SDK] BackpackView ProcessEvent hook installed (idx="
			<< Offsets::ProcessEventIdx << ") this=0x"
			<< std::hex << reinterpret_cast<uintptr_t>(BackpackObj)
			<< std::dec
			<< "\n";
		return true;
	}

	void RemoveItemEntryProcessEventHook()
	{
		GPendingItemEntryClickDefId.store(0, std::memory_order_release);
		if (GItemEntryProcessEventHook.IsInstalled())
			GItemEntryProcessEventHook.Remove();
		GOriginalItemEntryProcessEvent = nullptr;
	}

	void RemoveBackpackViewProcessEventHook()
	{
		if (GBackpackViewProcessEventHook.IsInstalled())
			GBackpackViewProcessEventHook.Remove();
		GOriginalBackpackViewProcessEvent = nullptr;
		GBackpackPECallCounts.clear();
		GBackpackPESeenFuncs.clear();
		GBackpackPELastDumpTick = 0;
		ResetBackpackCtxCaptureState();
	}

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

			if (Cfg.AllItemsSellable)
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

			RestoreArrayFromVector(Row->RandRange, Original.RandRange);
			RestoreArrayFromVector(Row->NormalRandRange, Original.NormalRandRange);

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
		// 物品获得加倍改由 InlineHook 实现，避免与这里的轮询增量逻辑叠加。
		const bool EnableGainMultiplier = false;
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
			It = GTab1ItemSnapshots.erase(It);
		}
	}

	void ReadTab1ConfigFromUI(FTab1RuntimeConfig& Cfg)
	{
		// 读取开关选项
		const bool NewItemNoDecrease = ReadToggleValue(GTab1ItemNoDecreaseToggle, Cfg.ItemNoDecrease);
		if (NewItemNoDecrease != Cfg.ItemNoDecrease)
		{
			LOGI_STREAM("FrameHook") << "[SDK] ItemNoDecrease: " << (NewItemNoDecrease ? "ON" : "OFF") << "\n";
			Cfg.ItemNoDecrease = NewItemNoDecrease;
		}

		const bool NewItemGainMultiplier = ReadToggleValue(GTab1ItemGainMultiplierToggle, Cfg.ItemGainMultiplier);
		if (NewItemGainMultiplier != Cfg.ItemGainMultiplier)
		{
			LOGI_STREAM("FrameHook") << "[SDK] ItemGainMultiplier: " << (NewItemGainMultiplier ? "ON" : "OFF") << "\n";
			Cfg.ItemGainMultiplier = NewItemGainMultiplier;
		}

		const bool NewAllItemsSellable = ReadToggleValue(GTab1AllItemsSellableToggle, Cfg.AllItemsSellable);
		if (NewAllItemsSellable != Cfg.AllItemsSellable)
		{
			LOGI_STREAM("FrameHook") << "[SDK] AllItemsSellable: " << (NewAllItemsSellable ? "ON" : "OFF") << "\n";
			Cfg.AllItemsSellable = NewAllItemsSellable;
		}

		const bool NewDropRate100 = ReadToggleValue(GTab1DropRate100Toggle, Cfg.DropRate100);
		if (NewDropRate100 != Cfg.DropRate100)
		{
			LOGI_STREAM("FrameHook") << "[SDK] DropRate100: " << (NewDropRate100 ? "ON" : "OFF") << "\n";
			Cfg.DropRate100 = NewDropRate100;
		}

		const bool NewCraftEffectMultiplier = ReadToggleValue(GTab1CraftEffectMultiplierToggle, Cfg.CraftEffectMultiplier);
		if (NewCraftEffectMultiplier != Cfg.CraftEffectMultiplier)
		{
			LOGI_STREAM("FrameHook") << "[SDK] CraftEffectMultiplier: " << (NewCraftEffectMultiplier ? "ON" : "OFF") << "\n";
			Cfg.CraftEffectMultiplier = NewCraftEffectMultiplier;
		}

		const bool NewIgnoreItemUseCount = ReadToggleValue(GTab1IgnoreItemUseCountToggle, Cfg.IgnoreItemUseCount);
		if (NewIgnoreItemUseCount != Cfg.IgnoreItemUseCount)
		{
			LOGI_STREAM("FrameHook") << "[SDK] IgnoreItemUseCount: " << (NewIgnoreItemUseCount ? "ON" : "OFF") << "\n";
			Cfg.IgnoreItemUseCount = NewIgnoreItemUseCount;
		}

		const bool NewIgnoreItemRequirements = ReadToggleValue(GTab1IgnoreItemRequirementsToggle, Cfg.IgnoreItemRequirements);
		if (NewIgnoreItemRequirements != Cfg.IgnoreItemRequirements)
		{
			LOGI_STREAM("FrameHook") << "[SDK] IgnoreItemRequirements: " << (NewIgnoreItemRequirements ? "ON" : "OFF") << "\n";
			Cfg.IgnoreItemRequirements = NewIgnoreItemRequirements;
		}

		// Sliders - 直接读取滑块值
		auto ReadSliderValue = [](UBPVE_JHConfigVolumeItem2_C* SliderItem, float DefaultValue) -> float {
			if (!SliderItem || !IsSafeLiveObject(static_cast<UObject*>(SliderItem)))
				return DefaultValue;
			USlider* Slider = SliderItem->VolumeSlider;
			if (!Slider || !IsSafeLiveObject(static_cast<UObject*>(Slider)))
				return DefaultValue;
			return Slider->GetValue();
		};

		const float SliderGainValue = ReadSliderValue(GTab1ItemGainMultiplierSlider, 2.0f);
		const int32 NewGainValue = static_cast<int32>(SliderGainValue + 0.5f);
		if (NewGainValue != Cfg.ItemGainMultiplierValue)
		{
			LOGI_STREAM("FrameHook") << "[SDK] ItemGainMultiplier: " << NewGainValue << "x\n";
			Cfg.ItemGainMultiplierValue = NewGainValue;
		}

		const float SliderIncrementValue = ReadSliderValue(GTab1CraftItemIncrementSlider, 2.0f);
		const float NewIncrementValue = SliderIncrementValue;
		if (std::fabs(NewIncrementValue - Cfg.CraftItemIncrementMultiplier) > 0.001f)
		{
			LOGI_STREAM("FrameHook") << "[SDK] CraftItemIncrement: " << NewIncrementValue << "x\n";
			Cfg.CraftItemIncrementMultiplier = NewIncrementValue;
		}

		const float SliderExtraValue = ReadSliderValue(GTab1CraftExtraEffectSlider, 2.0f);
		const float NewExtraValue = SliderExtraValue;
		if (std::fabs(NewExtraValue - Cfg.CraftExtraEffectMultiplier) > 0.001f)
		{
			LOGI_STREAM("FrameHook") << "[SDK] CraftExtraEffect: " << NewExtraValue << "x\n";
			Cfg.CraftExtraEffectMultiplier = NewExtraValue;
		}

		// Toggle
		const bool NewMaxExtraAffixes = ReadToggleValue(GTab1MaxExtraAffixesToggle, Cfg.MaxExtraAffixes);
		if (NewMaxExtraAffixes != Cfg.MaxExtraAffixes)
		{
			LOGI_STREAM("FrameHook") << "[SDK] MaxExtraAffixes: " << (NewMaxExtraAffixes ? "ON" : "OFF") << "\n";
			Cfg.MaxExtraAffixes = NewMaxExtraAffixes;
		}
	}

	void PollAndApplyTab1Features(bool CanReadFromUI)
	{
		static FTab1RuntimeConfig Config{};
		if (CanReadFromUI)
			ReadTab1ConfigFromUI(Config);

		// 根据开关状态动态启用/禁用 Hook
		static bool LastItemNoDecrease = false;
		if (Config.ItemNoDecrease != LastItemNoDecrease)
		{
			if (Config.ItemNoDecrease)
				EnableItemNoDecreaseHook();
			else
				DisableItemNoDecreaseHook();
			LastItemNoDecrease = Config.ItemNoDecrease;
		}

		// 只在倍率值变化时更新
		static int32 LastItemGainMultiplierValue = 2;
		if (Config.ItemGainMultiplierValue != LastItemGainMultiplierValue)
		{
			SetItemGainMultiplierHookValue(Config.ItemGainMultiplierValue);
			LastItemGainMultiplierValue = Config.ItemGainMultiplierValue;
		}

		static float LastCraftItemIncrementValue = 2.0f;
		if (std::fabs(Config.CraftItemIncrementMultiplier - LastCraftItemIncrementValue) > 0.001f)
		{
			SetCraftItemIncrementHookValue(Config.CraftItemIncrementMultiplier);
			LastCraftItemIncrementValue = Config.CraftItemIncrementMultiplier;
		}

		static float LastCraftExtraEffectValue = 2.0f;
		if (std::fabs(Config.CraftExtraEffectMultiplier - LastCraftExtraEffectValue) > 0.001f)
		{
			SetCraftExtraEffectHookValue(Config.CraftExtraEffectMultiplier);
			LastCraftExtraEffectValue = Config.CraftExtraEffectMultiplier;
		}

		const bool WantItemGainMultiplierHook = Config.ItemGainMultiplier && Config.ItemGainMultiplierValue > 1;
		static bool LastItemGainMultiplierHook = false;
		if (WantItemGainMultiplierHook != LastItemGainMultiplierHook)
		{
			if (WantItemGainMultiplierHook)
				EnableItemGainMultiplierHook();
			else
				DisableItemGainMultiplierHook();
			LastItemGainMultiplierHook = WantItemGainMultiplierHook;
		}

		// 所有物品可出售
		static bool LastAllItemsSellable = false;
		if (Config.AllItemsSellable != LastAllItemsSellable)
		{
			if (Config.AllItemsSellable)
				EnableAllItemsSellable();
			else
				DisableAllItemsSellable();
			LastAllItemsSellable = Config.AllItemsSellable;
		}

		static bool LastDropRate100Patch = false;
		if (Config.DropRate100 != LastDropRate100Patch)
		{
			if (Config.DropRate100)
				EnableDropRate100Patch();
			else
				DisableDropRate100Patch();
			LastDropRate100Patch = Config.DropRate100;
		}

		static bool LastCraftEffectHook = false;
		if (Config.CraftEffectMultiplier != LastCraftEffectHook)
		{
			if (Config.CraftEffectMultiplier)
				EnableCraftEffectMultiplierHook();
			else
				DisableCraftEffectMultiplierHook();
			LastCraftEffectHook = Config.CraftEffectMultiplier;
		}

		static bool LastMaxExtraAffixesHook = false;
		if (Config.MaxExtraAffixes != LastMaxExtraAffixesHook)
		{
			if (Config.MaxExtraAffixes)
				EnableMaxExtraAffixesHooks();
			else
				DisableMaxExtraAffixesHooks();
			LastMaxExtraAffixesHook = Config.MaxExtraAffixes;
		}

		// 包括任务物品
		// 更新全局开关
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
			FTab1RuntimeConfig TableConfig = Config;
			// DropRate100 与 CraftEffectMultiplier 已切换到 AOB/InlineHook 路线，避免双重叠加。
			TableConfig.DropRate100 = false;
			TableConfig.CraftEffectMultiplier = false;
			ApplyTab1ItemDefinitionFeatures(TableConfig, ItemTable);
			ApplyTab1DropPoolFeature(TableConfig, DropTable);
			ApplyTab1RandPoolFeature(TableConfig, RandTable);
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
			RemoveItemEntryProcessEventHook();
			if (kEnableBackpackViewProcessEventHook)
				RemoveBackpackViewProcessEventHook();
			APlayerController* PC = GetFirstLocalPlayerController();
			DestroyInternalWidget(PC);
			LOGI_STREAM("FrameHook") << "[SDK] UnloadCleanup: runtime UI cleanup done on game thread\n";
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
			LOGI_STREAM("FrameHook") << "[SDK] WorldTransition: oldWorld=" << (void*)LastWorld
				<< " newWorld=" << (void*)CurrentWorld
				<< " oldLevel=" << (void*)LastLevel
				<< " newLevel=" << (void*)CurrentLevel
				<< " hadWidget=" << (HadWidget ? 1 : 0)
				<< " -> invalidate runtime state only\n";

			InternalWidget = nullptr;
			InternalWidgetVisible = false;
			GCachedBtnExit = nullptr;
			ClearRuntimeWidgetState();
			ResetBackpackCtxCaptureState();
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
	static bool HomeNeedAnchorCtxRefresh = false;
	const bool HomeDown = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
	if (!InTransitionGuard && HomeDown && !HomeWasDown)
	{
		const bool WasVisible = InternalWidgetVisible;
		ToggleInternalWidget();
		const bool IsNowVisible = InternalWidgetVisible &&
			InternalWidget &&
			IsSafeLiveObject(static_cast<UObject*>(InternalWidget)) &&
			InternalWidget->IsInViewport();
		HomeNeedAnchorCtxRefresh = (!WasVisible && IsNowVisible);
	}
	HomeWasDown = HomeDown;

	// PGUP: 输出当前世界状态
	static bool PGUPWasDown = false;
	const bool PGUPDown = (GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0;
	if (PGUPDown && !PGUPWasDown)
	{
		LOGI_STREAM("FrameHook") << "[SDK] PGUP pressed, checking world state...\n";
		UWorld* World = UWorld::GetWorld();
		if (World && World->OwningGameInstance)
		{
			EWorldStateType WorldState = UManagerFuncLib::GetWorldType();
			LOGI_STREAM("FrameHook") << "[SDK] WorldStateType: " << (int)WorldState << "\n";
			switch (WorldState)
			{
				case EWorldStateType::None: LOGI_STREAM("FrameHook") << "[SDK]   -> None (main menu)\n"; break;
				case EWorldStateType::Scene: LOGI_STREAM("FrameHook") << "[SDK]   -> Scene (world/menu scene)\n"; break;
				case EWorldStateType::IntoFight: LOGI_STREAM("FrameHook") << "[SDK]   -> IntoFight\n"; break;
				case EWorldStateType::Fighting: LOGI_STREAM("FrameHook") << "[SDK]   -> Fighting\n"; break;
				case EWorldStateType::IntoScene: LOGI_STREAM("FrameHook") << "[SDK]   -> IntoScene\n"; break;
				case EWorldStateType::CG: LOGI_STREAM("FrameHook") << "[SDK]   -> CG\n"; break;
				case EWorldStateType::ChangeScene: LOGI_STREAM("FrameHook") << "[SDK]   -> ChangeScene\n"; break;
				case EWorldStateType::SkipingCG: LOGI_STREAM("FrameHook") << "[SDK]   -> SkipingCG\n"; break;
				case EWorldStateType::GameSystemActived: LOGI_STREAM("FrameHook") << "[SDK]   -> GameSystemActived\n"; break;
				default: LOGI_STREAM("FrameHook") << "[SDK]   -> Unknown\n"; break;
			}

			// 获取当前关卡名称
			FString LevelName = UGameplayStatics::GetCurrentLevelName(World, false);
			const wchar_t* LevelNameWs = LevelName.CStr();
			if (LevelNameWs && LevelNameWs[0])
			{
				LOGI_STREAM("FrameHook") << L"[SDK] LevelName: " << LevelNameWs << L"\n";
			}
			else
			{
				LOGI_STREAM("FrameHook") << "[SDK] LevelName: (empty)\n";
			}

			// 检查 PersistentLevel 是否存在
			if (World->PersistentLevel)
			{
				LOGI_STREAM("FrameHook") << "[SDK] PersistentLevel: " << (void*)World->PersistentLevel << "\n";
			}
		}
		else
		{
			LOGI_STREAM("FrameHook") << "[SDK] World or GameInstance is null\n";
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

	// On each HOME show: run anchor scan; rebuild item manager only when ctx changes.
	if (HomeNeedAnchorCtxRefresh &&
		InternalWidgetVisible &&
		LiveConfigView &&
		IsSafeLiveObject(static_cast<UObject*>(LiveConfigView)))
	{
		HomeNeedAnchorCtxRefresh = false;
		bool CtxChanged = false;
		const bool FoundCtx = RefreshEntryInitContextFromAnchorScan(&CtxChanged);
		if (!FoundCtx)
		{
			LOGE_STREAM("FrameHook") << "[SDK] HomeAnchorCtxRefresh: anchor scan miss, keep current item manager\n";
		}
		else if (CtxChanged)
		{
			APlayerController* PC = GetFirstLocalPlayerController();
			if (PC)
			{
				PopulateTab_Items(LiveConfigView, PC);
				if (IsItemsTabActive)
					OnItemBrowserTabShown();
				LOGI_STREAM("FrameHook") << "[SDK] HomeAnchorCtxRefresh: ctx changed, item manager rebuilt\n";
			}
		}
		else
		{
			LOGI_STREAM("FrameHook") << "[SDK] HomeAnchorCtxRefresh: ctx unchanged, skip rebuild\n";
		}
	}

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

	// BackpackView ProcessEvent hook path is intentionally disabled.

	// Item browser per-frame polling
	static DWORD sLastItemUiPollTick = 0;
	if (InternalWidgetVisible && LiveInternalWidget && IsItemsTabActive)
	{
		EnsureItemEntryProcessEventHookInstalled();

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
						// 滑块值1-10，直接显示
						int32 DisplayValue = static_cast<int32>(CurValue + 0.5f);
						if (DisplayValue < 1) DisplayValue = 1;
						if (DisplayValue > 10) DisplayValue = 10;
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

			int32 catIdx = GItemLastCatIdx;
			bool needFilterRefresh = false;
			if (GItemCategoryDD && GItemCategoryDD->CB_Main)
			{
				int32 Selected = GetComboSelectedIndexSafe(GItemCategoryDD->CB_Main);
				if (Selected >= 0)
				{
					catIdx = Selected;
					if (catIdx != GItemLastCatIdx)
					{
						GItemLastCatIdx = catIdx;
						needFilterRefresh = true;
					}
				}
			}
			if (UpdateItemSearchKeywordFromEdit())
				needFilterRefresh = true;
			if (needFilterRefresh)
			{
				GItemCurrentPage = 0;
				FilterItems((catIdx >= 0) ? catIdx : 0);
				RefreshItemPage();
			}

			GItemAddQuantity = GetItemAddQuantityFromEdit();

			int32 ClickDefId = 0;
			if (TryConsumeItemEntryClickDefId(ClickDefId))
			{
				const bool CtrlDown = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
				const int32 Quantity = CtrlDown ? 10 : ((GItemAddQuantity > 0) ? GItemAddQuantity : 1);
				if (Quantity <= 1)
				{
					UItemFuncLib::AddItem(ClickDefId, 1);
				}
				else
				{
					// Use single-item adds to avoid unstable behavior with Num > 1 in this UI context.
					for (int32 i = 0; i < Quantity; ++i)
						UItemFuncLib::AddItem(ClickDefId, 1);
				}
				LOGI_STREAM("FrameHook") << "[SDK] AddItem(list-left-click): defId=" << ClickDefId
					<< " x" << Quantity
					<< " ctrl=" << (CtrlDown ? 1 : 0)
					<< "\n";
			}

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
		}
	}
	else
	{
		sLastItemUiPollTick = 0;
		GPendingItemEntryClickDefId.store(0, std::memory_order_release);
		GItemPrevWasPressed = false;
		GItemNextWasPressed = false;
		// Avoid BP_ClearSelection while UMG object is unreachable/destroying.
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
		LOGI_STREAM("FrameHook") << "[SDK] Widget closed externally, cached instance kept\n";
	}
}

// -- Main Thread --
