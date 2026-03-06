// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  FrameHook.cpp  —  帧回调核心逻辑 (2000+ 行)
//
//  本文件是作弊模块的心脏：通过 VTable Hook 挂载在引擎 PostRender
//  回调上，每帧执行一次。内部按逻辑分为以下几大区块：
//
//    §1  辅助工具        RAII 作用域、存活性检查、FPS 叠加
//    §2  内部数据结构    运行时配置快照、DataTable 原始状态备份
//    §3  ItemEntry Hook  物品槽位点击事件拦截 (VTable ProcessEvent)
//    §4  BackpackView    背包视图上下文捕获 (用于添加物品)
//    §5  UI 读取工具     ComboBox / Toggle / Slider / EditText 安全读取
//    §6  TArray 工具     UE4 TArray ↔ std::vector 双向转换
//    §7  Tab1 物品逻辑   DataTable 行遍历、补丁应用、功能轮询
//    §8  Tab2 战斗逻辑   各类 Inline Hook 启停控制
//    §9  滑块通用轮询    VolumeItem 按钮点击与文本标签同步
//    §10 主回调入口      HookedGVCPostRender — 统揽全局
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
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
	// ┌─────────────────────────────────────────────────────────────┐
// │  §1  辅助工具 — RAII 作用域 · 存活性检查 · FPS 叠加        │
// └─────────────────────────────────────────────────────────────┘

/// RAII 守卫：构造时 +1、析构时 -1 GPostRenderInFlight 计数
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

	/// 检查作弊面板 Widget 指针是否仍指向有效的存活对象，
	/// 若已被引擎回收则重置全局状态并返回 false。
	bool EnsureLiveInternalWidgetForFrame()
	{
		if (!GInternalWidget)
			return false;

		auto* Obj = static_cast<UObject*>(GInternalWidget);
		if (IsSafeLiveObject(Obj))
			return true;

		LOGI_STREAM("FrameHook") << "[SDK] FrameHook: stale internal widget pointer detected, reset state\n";
		GInternalWidget = nullptr;
		GInternalWidgetVisible = false;
		GCachedBtnExit = nullptr;
		ClearRuntimeWidgetState();
		return false;
	}

	/// 在屏幕左上角绘制实时 FPS 计数 (250ms 滑动窗口采样)
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

	// ┌─────────────────────────────────────────────────────────────┐
	// │  §2  内部数据结构 — 运行时配置快照 · DataTable 原始备份    │
	// └─────────────────────────────────────────────────────────────┘

	/// 用于物品 GUID 哈希表的键类型
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

	struct FTab2RuntimeConfig final
	{
		bool SkillNoCooldown = false;
		bool NoEncounter = false;
		bool AllTeammatesInFight = false;
		bool DefeatAsVictory = false;
		bool NeiGongFillLastSlot = false;
		bool AutoRecoverHpMp = false;
		bool TotalMoveSpeedEnabled = false;
		bool TotalMoveSpeedFriendlyOnly = false;
		bool BattleSpeedEnabled = false;
		float BattleSpeedMultiplier = 2.0f;
		float TotalMoveSpeedMultiplier = 2.0f;
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

	using UObjectProcessEventFn = void(__fastcall*)(const UObject* , UFunction* , void* );
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

	// ┌─────────────────────────────────────────────────────────────┐
	// │  §3  ItemEntry Hook — 物品槽位点击事件拦截                 │
	// └─────────────────────────────────────────────────────────────┘

	/// 尝试从 ItemEntry 控件中提取物品 DefId 并入队等待主线程消费
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
		if (ThisObj && Function && GInternalWidgetVisible)
		{
			const std::string FuncName = Function->GetName();
			if (kVerboseItemEntryProcessEventLogs)
			{
				LOGI_STREAM("FrameHook") << "[SDK] ItemEntry ProcessEvent: func=" << FuncName
					<< " this=" << (void*)ThisObj << "\n";
			}

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
		for (int32 i = 0; i < kItemsPerPage; ++i)
		{
			UObject* Obj = static_cast<UObject*>(GItemBrowser.SlotEntryWidgets[i]);
			if (IsSafeLiveObjectOfClass(Obj, UJHNeoUIItemEntryWDT::StaticClass()))
				return Obj;
		}

		UListView* ListView = GItemBrowser.ListView;
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

	// ┌─────────────────────────────────────────────────────────────┐
	// │  §4  BackpackView — 背包视图上下文捕获                     │
	// └─────────────────────────────────────────────────────────────┘

	/// 被挂钩的背包视图 ProcessEvent，持续捕获 Ctx 弱引用
	void __fastcall HookedBackpackViewProcessEvent(const UObject* ThisObj, UFunction* Function, void* Params)
	{
		if (ThisObj && Function)
		{
			UObject* MutableThis = const_cast<UObject*>(ThisObj);
			if (IsSafeLiveObjectOfClass(MutableThis, UJHBackpackViewBase::StaticClass()))
			{
				const std::string FuncName = Function->GetName();
				++GBackpackPECallCounts[FuncName];

				if (FuncName == "EVT_RenderView")
				{
					auto* View = static_cast<UJHBackpackViewBase*>(MutableThis);
					UObject* GridObj = static_cast<UObject*>(View->GRID_ITEM);
					int32 GridIndex = -1;
					int32 GridDist = (std::numeric_limits<int32>::max)();
					bool bGridExact = false;
					if (ShouldCaptureBackpackCtxFromGrid(GridObj, GridIndex, GridDist, bGridExact))
					{
						FWeakObjectPtr CtxWeak = ReadWeakPtrAt(GridObj, kBackpackGridCtxWeakOffset);
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
						FWeakObjectPtr CtxWeak = ReadWeakPtrAt(GridObj, kBackpackGridCtxWeakOffset);
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

	// ┌─────────────────────────────────────────────────────────────┐
	// │  §5  UI 读取工具 — ComboBox / Toggle / Slider 安全读取     │
	// └─────────────────────────────────────────────────────────────┘

	/// 检查 ComboBox 控件是否仍然存活且未被引擎标记为待销毁
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
		return 1.0f + Percent * 0.09f;
	}

	// ┌─────────────────────────────────────────────────────────────┐
	// │  §6  TArray 工具 — UE4 TArray ↔ std::vector 双向转换       │
	// └─────────────────────────────────────────────────────────────┘

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

	// ┌─────────────────────────────────────────────────────────────┐
	// │  §7  Tab1 物品逻辑 — DataTable 遍历 · 补丁应用 · 功能轮询   │
	// └─────────────────────────────────────────────────────────────┘

	/// 当 DataTable 指针发生变化时清空对应的原始状态缓存
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

			Row->UsedCountLimit = Original.UsedCountLimit;

			if (Cfg.IgnoreItemRequirements)
			{
			}
			RestoreArrayFromVector(Row->Requirements, Original.Requirements);

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
		const bool NewItemNoDecrease = ReadToggleValue(GTab1.ItemNoDecreaseToggle, Cfg.ItemNoDecrease);
		if (NewItemNoDecrease != Cfg.ItemNoDecrease)
		{
			LOGI_STREAM("FrameHook") << "[SDK] ItemNoDecrease: " << (NewItemNoDecrease ? "ON" : "OFF") << "\n";
			Cfg.ItemNoDecrease = NewItemNoDecrease;
		}

		const bool NewItemGainMultiplier = ReadToggleValue(GTab1.ItemGainMultiplierToggle, Cfg.ItemGainMultiplier);
		if (NewItemGainMultiplier != Cfg.ItemGainMultiplier)
		{
			LOGI_STREAM("FrameHook") << "[SDK] ItemGainMultiplier: " << (NewItemGainMultiplier ? "ON" : "OFF") << "\n";
			Cfg.ItemGainMultiplier = NewItemGainMultiplier;
		}

		const bool NewAllItemsSellable = ReadToggleValue(GTab1.AllItemsSellableToggle, Cfg.AllItemsSellable);
		if (NewAllItemsSellable != Cfg.AllItemsSellable)
		{
			LOGI_STREAM("FrameHook") << "[SDK] AllItemsSellable: " << (NewAllItemsSellable ? "ON" : "OFF") << "\n";
			Cfg.AllItemsSellable = NewAllItemsSellable;
		}

		const bool NewDropRate100 = ReadToggleValue(GTab1.DropRate100Toggle, Cfg.DropRate100);
		if (NewDropRate100 != Cfg.DropRate100)
		{
			LOGI_STREAM("FrameHook") << "[SDK] DropRate100: " << (NewDropRate100 ? "ON" : "OFF") << "\n";
			Cfg.DropRate100 = NewDropRate100;
		}

		const bool NewCraftEffectMultiplier = ReadToggleValue(GTab1.CraftEffectMultiplierToggle, Cfg.CraftEffectMultiplier);
		if (NewCraftEffectMultiplier != Cfg.CraftEffectMultiplier)
		{
			LOGI_STREAM("FrameHook") << "[SDK] CraftEffectMultiplier: " << (NewCraftEffectMultiplier ? "ON" : "OFF") << "\n";
			Cfg.CraftEffectMultiplier = NewCraftEffectMultiplier;
		}

		const bool NewIgnoreItemUseCount = ReadToggleValue(GTab1.IgnoreItemUseCountToggle, Cfg.IgnoreItemUseCount);
		if (NewIgnoreItemUseCount != Cfg.IgnoreItemUseCount)
		{
			LOGI_STREAM("FrameHook") << "[SDK] IgnoreItemUseCount: " << (NewIgnoreItemUseCount ? "ON" : "OFF") << "\n";
			Cfg.IgnoreItemUseCount = NewIgnoreItemUseCount;
		}

		const bool NewIgnoreItemRequirements = ReadToggleValue(GTab1.IgnoreItemRequirementsToggle, Cfg.IgnoreItemRequirements);
		if (NewIgnoreItemRequirements != Cfg.IgnoreItemRequirements)
		{
			LOGI_STREAM("FrameHook") << "[SDK] IgnoreItemRequirements: " << (NewIgnoreItemRequirements ? "ON" : "OFF") << "\n";
			Cfg.IgnoreItemRequirements = NewIgnoreItemRequirements;
		}

		auto ReadSliderValue = [](UBPVE_JHConfigVolumeItem2_C* SliderItem, float DefaultValue) -> float {
			if (!SliderItem || !IsSafeLiveObject(static_cast<UObject*>(SliderItem)))
				return DefaultValue;
			USlider* Slider = SliderItem->VolumeSlider;
			if (!Slider || !IsSafeLiveObject(static_cast<UObject*>(Slider)))
				return DefaultValue;
			return Slider->GetValue();
		};

		const float SliderGainValue = ReadSliderValue(GTab1.ItemGainMultiplierSlider, 2.0f);
		const int32 NewGainValue = static_cast<int32>(SliderGainValue + 0.5f);
		if (NewGainValue != Cfg.ItemGainMultiplierValue)
		{
			LOGI_STREAM("FrameHook") << "[SDK] ItemGainMultiplier: " << NewGainValue << "x\n";
			Cfg.ItemGainMultiplierValue = NewGainValue;
		}

		const float SliderIncrementValue = ReadSliderValue(GTab1.CraftItemIncrementSlider, 2.0f);
		const float NewIncrementValue = SliderIncrementValue;
		if (std::fabs(NewIncrementValue - Cfg.CraftItemIncrementMultiplier) > 0.001f)
		{
			LOGI_STREAM("FrameHook") << "[SDK] CraftItemIncrement: " << NewIncrementValue << "x\n";
			Cfg.CraftItemIncrementMultiplier = NewIncrementValue;
		}

		const float SliderExtraValue = ReadSliderValue(GTab1.CraftExtraEffectSlider, 2.0f);
		const float NewExtraValue = SliderExtraValue;
		if (std::fabs(NewExtraValue - Cfg.CraftExtraEffectMultiplier) > 0.001f)
		{
			LOGI_STREAM("FrameHook") << "[SDK] CraftExtraEffect: " << NewExtraValue << "x\n";
			Cfg.CraftExtraEffectMultiplier = NewExtraValue;
		}

		const bool NewMaxExtraAffixes = ReadToggleValue(GTab1.MaxExtraAffixesToggle, Cfg.MaxExtraAffixes);
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

		static bool LastItemNoDecrease = false;
		if (Config.ItemNoDecrease != LastItemNoDecrease)
		{
			if (Config.ItemNoDecrease)
				EnableItemNoDecreaseHook();
			else
				DisableItemNoDecreaseHook();
			LastItemNoDecrease = Config.ItemNoDecrease;
		}

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

		static bool LastIgnoreItemUseCountFeature = false;
		if (Config.IgnoreItemUseCount != LastIgnoreItemUseCountFeature)
		{
			if (Config.IgnoreItemUseCount)
				EnableIgnoreItemUseCountFeature();
			else
				DisableIgnoreItemUseCountFeature();
			LastIgnoreItemUseCountFeature = Config.IgnoreItemUseCount;
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

		static bool LastIgnoreItemRequirementsPatch = false;
		if (Config.IgnoreItemRequirements != LastIgnoreItemRequirementsPatch)
		{
			if (Config.IgnoreItemRequirements)
				EnableIgnoreItemRequirementsPatch();
			else
				DisableIgnoreItemRequirementsPatch();
			LastIgnoreItemRequirementsPatch = Config.IgnoreItemRequirements;
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

        const DWORD NowTick = GetTickCount();

        static DWORD LastInventoryTick = 0;
        const bool NeedBackpackSync = Config.ItemNoDecrease || Config.ItemGainMultiplier || !GTab1ItemSnapshots.empty();
        if (NeedBackpackSync && (LastInventoryTick == 0 || (NowTick - LastInventoryTick) >= 80))
        {
            LastInventoryTick = NowTick;
            ApplyTab1BackpackFeatures(Config);
        }

        static UDataTable* LastSyncedItemTable = nullptr;
        static bool LastSyncedAllItemsSellable = false;
        static bool HasItemTableSync = false;

        if (!ItemTable)
        {
            LastSyncedItemTable = nullptr;
            HasItemTableSync = false;
        }
        else
        {
            const bool ItemTableChanged = (ItemTable != LastSyncedItemTable);
            const bool SellableStateChanged = (!HasItemTableSync) || (Config.AllItemsSellable != LastSyncedAllItemsSellable);
            if (ItemTableChanged || SellableStateChanged)
            {
                FTab1RuntimeConfig TableConfig{};
                TableConfig.AllItemsSellable = Config.AllItemsSellable;
                ApplyTab1ItemDefinitionFeatures(TableConfig, ItemTable);

                LastSyncedItemTable = ItemTable;
                LastSyncedAllItemsSellable = Config.AllItemsSellable;
                HasItemTableSync = true;
            }
        }
		}
	// ┌─────────────────────────────────────────────────────────────┐
	// │  §8  Tab2 战斗逻辑 — Inline Hook 启停 · 倍率设置           │
	// └─────────────────────────────────────────────────────────────┘

	/// 从 Tab2 UI 控件读取当前战斗功能配置快照
	void ReadTab2ConfigFromUI(FTab2RuntimeConfig& Cfg)
	{
		const bool NewSkillNoCooldown = ReadToggleValue(GTab2.SkillNoCooldownToggle, Cfg.SkillNoCooldown);
		if (NewSkillNoCooldown != Cfg.SkillNoCooldown)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 SkillNoCooldown: " << (NewSkillNoCooldown ? "ON" : "OFF") << "\n";
			Cfg.SkillNoCooldown = NewSkillNoCooldown;
		}

		const bool NewBattleSpeedEnabled = ReadToggleValue(GTab2.DamageBoostToggle, Cfg.BattleSpeedEnabled);
		if (NewBattleSpeedEnabled != Cfg.BattleSpeedEnabled)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 BattleSpeed: " << (NewBattleSpeedEnabled ? "ON" : "OFF") << "\n";
			Cfg.BattleSpeedEnabled = NewBattleSpeedEnabled;
		}

		const bool NewNoEncounter = ReadToggleValue(GTab2.NoEncounterToggle, Cfg.NoEncounter);
		if (NewNoEncounter != Cfg.NoEncounter)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 NoEncounter: " << (NewNoEncounter ? "ON" : "OFF") << "\n";
			Cfg.NoEncounter = NewNoEncounter;
		}

		const bool NewAllTeammatesInFight = ReadToggleValue(GTab2.AllTeammatesInFightToggle, Cfg.AllTeammatesInFight);
		if (NewAllTeammatesInFight != Cfg.AllTeammatesInFight)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 AllTeammatesInFight: " << (NewAllTeammatesInFight ? "ON" : "OFF") << "\n";
			Cfg.AllTeammatesInFight = NewAllTeammatesInFight;
		}

		const bool NewDefeatAsVictory = ReadToggleValue(GTab2.DefeatAsVictoryToggle, Cfg.DefeatAsVictory);
		if (NewDefeatAsVictory != Cfg.DefeatAsVictory)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 DefeatAsVictory: " << (NewDefeatAsVictory ? "ON" : "OFF") << "\n";
			Cfg.DefeatAsVictory = NewDefeatAsVictory;
		}

		const bool NewNeiGongFillLastSlot = ReadToggleValue(GTab2.NeiGongFillLastSlotToggle, Cfg.NeiGongFillLastSlot);
		if (NewNeiGongFillLastSlot != Cfg.NeiGongFillLastSlot)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 NeiGongFillLastSlot: " << (NewNeiGongFillLastSlot ? "ON" : "OFF") << "\n";
			Cfg.NeiGongFillLastSlot = NewNeiGongFillLastSlot;
		}

		const bool NewAutoRecoverHpMp = ReadToggleValue(GTab2.AutoRecoverHpMpToggle, Cfg.AutoRecoverHpMp);
		if (NewAutoRecoverHpMp != Cfg.AutoRecoverHpMp)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 AutoRecoverHpMp: " << (NewAutoRecoverHpMp ? "ON" : "OFF") << "\n";
			Cfg.AutoRecoverHpMp = NewAutoRecoverHpMp;
		}

		const bool NewTotalMoveSpeedEnabled = ReadToggleValue(GTab2.TotalMoveSpeedToggle, Cfg.TotalMoveSpeedEnabled);
		if (NewTotalMoveSpeedEnabled != Cfg.TotalMoveSpeedEnabled)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 TotalMoveSpeed: " << (NewTotalMoveSpeedEnabled ? "ON" : "OFF") << "\n";
			Cfg.TotalMoveSpeedEnabled = NewTotalMoveSpeedEnabled;
		}

		const bool NewTotalMoveSpeedFriendlyOnly = ReadToggleValue(GTab2.DamageFriendlyOnlyToggle, Cfg.TotalMoveSpeedFriendlyOnly);
		if (NewTotalMoveSpeedFriendlyOnly != Cfg.TotalMoveSpeedFriendlyOnly)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 TotalMoveSpeedFriendlyOnly: " << (NewTotalMoveSpeedFriendlyOnly ? "ON" : "OFF") << "\n";
			Cfg.TotalMoveSpeedFriendlyOnly = NewTotalMoveSpeedFriendlyOnly;
		}

		auto ReadSliderValue = [](UBPVE_JHConfigVolumeItem2_C* SliderItem, float DefaultValue) -> float {
			if (!SliderItem || !IsSafeLiveObject(static_cast<UObject*>(SliderItem)))
				return DefaultValue;
			USlider* Slider = SliderItem->VolumeSlider;
			if (!Slider || !IsSafeLiveObject(static_cast<UObject*>(Slider)))
				return DefaultValue;
			return Slider->GetValue();
		};

		float NewBattleSpeedMultiplier = ReadSliderValue(GTab2.DamageMultiplierSlider, Cfg.BattleSpeedMultiplier);
		if (NewBattleSpeedMultiplier < 0.0f) NewBattleSpeedMultiplier = 0.0f;
		if (NewBattleSpeedMultiplier > 10.0f) NewBattleSpeedMultiplier = 10.0f;
		if (std::fabs(NewBattleSpeedMultiplier - Cfg.BattleSpeedMultiplier) > 0.001f)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 BattleSpeedMultiplier: " << NewBattleSpeedMultiplier << "x\n";
			Cfg.BattleSpeedMultiplier = NewBattleSpeedMultiplier;
		}

		float NewTotalMoveSpeedMultiplier = ReadSliderValue(GTab2.MoveSpeedMultiplierSlider, Cfg.TotalMoveSpeedMultiplier);
		if (NewTotalMoveSpeedMultiplier < 0.0f) NewTotalMoveSpeedMultiplier = 0.0f;
		if (NewTotalMoveSpeedMultiplier > 10.0f) NewTotalMoveSpeedMultiplier = 10.0f;
		if (std::fabs(NewTotalMoveSpeedMultiplier - Cfg.TotalMoveSpeedMultiplier) > 0.001f)
		{
			LOGI_STREAM("FrameHook") << "[SDK] Tab2 TotalMoveSpeedMultiplier: " << NewTotalMoveSpeedMultiplier << "x\n";
			Cfg.TotalMoveSpeedMultiplier = NewTotalMoveSpeedMultiplier;
		}
	}

	void PollAndApplyTab2Features(bool CanReadFromUI)
	{
		static FTab2RuntimeConfig Config{};
		if (CanReadFromUI)
			ReadTab2ConfigFromUI(Config);

		static bool LastSkillNoCooldownHook = false;
		if (Config.SkillNoCooldown != LastSkillNoCooldownHook)
		{
			if (Config.SkillNoCooldown)
				EnableSkillNoCooldownHooks();
			else
				DisableSkillNoCooldownHooks();
			LastSkillNoCooldownHook = Config.SkillNoCooldown;
		}

		static bool LastNoEncounterPatch = false;
		if (Config.NoEncounter != LastNoEncounterPatch)
		{
			if (Config.NoEncounter)
				EnableNoEncounterPatch();
			else
				DisableNoEncounterPatch();
			LastNoEncounterPatch = Config.NoEncounter;
		}

		static bool LastAllTeammatesInFightHook = false;
		if (Config.AllTeammatesInFight != LastAllTeammatesInFightHook)
		{
			if (Config.AllTeammatesInFight)
				EnableAllTeammatesInFightHooks();
			else
				DisableAllTeammatesInFightHooks();
			LastAllTeammatesInFightHook = Config.AllTeammatesInFight;
		}

		static bool LastDefeatAsVictoryHook = false;
		if (Config.DefeatAsVictory != LastDefeatAsVictoryHook)
		{
			if (Config.DefeatAsVictory)
				EnableDefeatAsVictoryHook();
			else
				DisableDefeatAsVictoryHook();
			LastDefeatAsVictoryHook = Config.DefeatAsVictory;
		}

		static bool LastNeiGongFillLastSlot = false;
		if (Config.NeiGongFillLastSlot != LastNeiGongFillLastSlot)
		{
			if (Config.NeiGongFillLastSlot)
				EnableNeiGongFillLastSlotFeature();
			else
				DisableNeiGongFillLastSlotFeature();
			LastNeiGongFillLastSlot = Config.NeiGongFillLastSlot;
		}

		static bool LastAutoRecoverHpMp = false;
		if (Config.AutoRecoverHpMp != LastAutoRecoverHpMp)
		{
			if (Config.AutoRecoverHpMp)
				EnableAutoRecoverHpMpHook();
			else
				DisableAutoRecoverHpMpHook();
			LastAutoRecoverHpMp = Config.AutoRecoverHpMp;
		}

		SetTotalMoveSpeedMultiplier(Config.TotalMoveSpeedMultiplier);
		SetTotalMoveSpeedFriendlyOnly(Config.TotalMoveSpeedFriendlyOnly);
		static bool LastTotalMoveSpeedEnabled = false;
		if (Config.TotalMoveSpeedEnabled != LastTotalMoveSpeedEnabled)
		{
			if (Config.TotalMoveSpeedEnabled)
				EnableTotalMoveSpeedHook();
			else
				DisableTotalMoveSpeedHook();
			LastTotalMoveSpeedEnabled = Config.TotalMoveSpeedEnabled;
		}

		SetBattleSpeedHookMultiplier(Config.BattleSpeedMultiplier);
		static bool LastBattleSpeedHookEnabled = false;
		if (Config.BattleSpeedEnabled != LastBattleSpeedHookEnabled)
		{
			if (Config.BattleSpeedEnabled)
				EnableBattleSpeedHooks();
			else
				DisableBattleSpeedHooks();
			LastBattleSpeedHookEnabled = Config.BattleSpeedEnabled;
		}
	}

	// ┌─────────────────────────────────────────────────────────────┐
	// │  §9  滑块通用轮询 — 按钮点击增减 · 文本标签同步             │
	// └─────────────────────────────────────────────────────────────┘

	/// 轮询所有 VolumeItem 滑块控件的 +/- 按钮和数值标签
	void PollVolumeItemsButtonsAndText()
	{
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
					wchar_t Buf[16] = {};
					swprintf_s(Buf, 16, L"%.3g", static_cast<double>(CurValue));
					Item->TXT_CurrentValue->SetText(MakeText(Buf));
				}
				RememberSingleSliderState(Item);
			}

			GVolumeLastValues[i] = CurValue;
			GVolumeMinusWasPressed[i] = MinusPressed;
			GVolumePlusWasPressed[i] = PlusPressed;
		}
	}
}

// ┌─────────────────────────────────────────────────────────────┐
// │  §10  主回调入口 — HookedGVCPostRender                     │
// └─────────────────────────────────────────────────────────────┘

/// 每帧由引擎渲染线程调用的顶层分发函数。
/// 按序执行：快捷键 → 面板生命周期 → Tab1 → Tab2 → 动态Tab → 滑块 → FPS
void __fastcall HookedGVCPostRender(void* This, void* Canvas)
{
	PostRenderInFlightScope InFlightScope;

	if (GOriginalPostRender)
		GOriginalPostRender(This, Canvas);

	DrawFpsOverlay(static_cast<UCanvas*>(Canvas));

	if (GIsUnloading.load(std::memory_order_relaxed))
	{
		if (!GUnloadCleanupDone.exchange(true, std::memory_order_acq_rel))
		{
			RemoveItemEntryProcessEventHook();
			if (kEnableBackpackViewProcessEventHook)
				RemoveBackpackViewProcessEventHook();
			DisableSkillNoCooldownHooks();
			DisableNoEncounterPatch();
			DisableAllTeammatesInFightHooks();
			DisableDefeatAsVictoryHook();
			DisableNeiGongFillLastSlotFeature();
			DisableAutoRecoverHpMpHook();
			DisableTotalMoveSpeedHook();
			DisableBattleSpeedHooks();
			APlayerController* PC = GetFirstLocalPlayerController();
			DestroyInternalWidget(PC);
			LOGI_STREAM("FrameHook") << "[SDK] UnloadCleanup: runtime UI cleanup done on game thread\n";
		}
		return;
	}

	static UWorld* LastWorld = nullptr;
	static ULevel* LastLevel = nullptr;
	static int32 TransitionGuardFrames = 0;
	UWorld* CurrentWorld = UWorld::GetWorld();
	ULevel* CurrentLevel = CurrentWorld ? CurrentWorld->PersistentLevel : nullptr;
	if (CurrentWorld != LastWorld || CurrentLevel != LastLevel)
	{
		if (LastWorld != nullptr)
		{
			const bool HadWidget = (GInternalWidget != nullptr) || GInternalWidgetVisible;
			LOGI_STREAM("FrameHook") << "[SDK] WorldTransition: oldWorld=" << (void*)LastWorld
				<< " newWorld=" << (void*)CurrentWorld
				<< " oldLevel=" << (void*)LastLevel
				<< " newLevel=" << (void*)CurrentLevel
				<< " hadWidget=" << (HadWidget ? 1 : 0)
				<< " -> invalidate runtime state only\n";

			GInternalWidget = nullptr;
			GInternalWidgetVisible = false;
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

	static bool HomeWasDown = false;
	static bool HomeNeedAnchorCtxRefresh = false;
	static bool HomeNeedDynTabRestore = false;
	static bool HomeNeedSliderRememberRestore = false;
	const bool HomeDown = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
	if (!InTransitionGuard && HomeDown && !HomeWasDown)
	{
		const bool WasVisible = GInternalWidgetVisible;
		ToggleInternalWidget();
		const bool IsNowVisible = GInternalWidgetVisible &&
			GInternalWidget &&
			IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)) &&
			GInternalWidget->IsInViewport();
		HomeNeedAnchorCtxRefresh = (!WasVisible && IsNowVisible);
		HomeNeedDynTabRestore = (!WasVisible && IsNowVisible);
		HomeNeedSliderRememberRestore = (!WasVisible && IsNowVisible);
	}
	HomeWasDown = HomeDown;

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
	UUserWidget* LiveInternalWidget = HasLiveInternalWidget ? GInternalWidget : nullptr;
	UBPMV_ConfigView2_C* LiveConfigView = nullptr;
	int32 ActiveNativeTabIndex = -1;
	if (GInternalWidgetVisible &&
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
	const bool IsBattleTabActive = (ActiveNativeTabIndex == 2);

	if (HomeNeedAnchorCtxRefresh &&
		GInternalWidgetVisible &&
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

	if (HomeNeedSliderRememberRestore &&
		GInternalWidgetVisible &&
		LiveInternalWidget &&
		IsSafeLiveObject(static_cast<UObject*>(LiveInternalWidget)))
	{
		HomeNeedSliderRememberRestore = false;
		SuppressSliderRealtimeRememberForMs(800);
		RestoreRememberedSliderStatesToLiveWidgets();
	}

	PollTab0CharacterInput(IsCharacterTabActive);

	static bool ExitWasPressed = false;
	bool ExitPressed = false;
	const bool CanCheckExit =
		GInternalWidgetVisible &&
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


	static DWORD sLastItemUiPollTick = 0;
	if (GInternalWidgetVisible && LiveInternalWidget && IsItemsTabActive)
	{
		EnsureItemEntryProcessEventHookInstalled();

		const DWORD ItemUiNow = GetTickCount();
		const bool RunItemUiPoll = (sLastItemUiPollTick == 0) || ((ItemUiNow - sLastItemUiPollTick) >= 16);
		if (RunItemUiPoll)
		{
			sLastItemUiPollTick = ItemUiNow;
			PollCollapsiblePanelsInput();
			PollVolumeItemsButtonsAndText();

			static DWORD LastItemCacheRetryTick = 0;
			if (!GItemBrowser.CacheBuilt && (GItemBrowser.CategoryDD || GItemBrowser.GridPanel))
			{
				DWORD NowTick = GetTickCount();
				if (NowTick - LastItemCacheRetryTick > 1000)
				{
					LastItemCacheRetryTick = NowTick;
					BuildItemCache();
					if (GItemBrowser.CacheBuilt)
					{
						int32 CurrentCat = 0;
						if (GItemBrowser.CategoryDD && GItemBrowser.CategoryDD->CB_Main)
						{
							int32 SelectedCat = GetComboSelectedIndexSafe(GItemBrowser.CategoryDD->CB_Main);
							if (SelectedCat >= 0)
								CurrentCat = SelectedCat;
						}
						GItemBrowser.CurrentPage = 0;
						GItemBrowser.LastCatIdx = CurrentCat;
						FilterItems(CurrentCat);
						RefreshItemPage();
					}
				}
			}

			int32 catIdx = GItemBrowser.LastCatIdx;
			bool needFilterRefresh = false;
			if (GItemBrowser.CategoryDD && GItemBrowser.CategoryDD->CB_Main)
			{
				int32 Selected = GetComboSelectedIndexSafe(GItemBrowser.CategoryDD->CB_Main);
				if (Selected >= 0)
				{
					catIdx = Selected;
					if (catIdx != GItemBrowser.LastCatIdx)
					{
						GItemBrowser.LastCatIdx = catIdx;
						needFilterRefresh = true;
					}
				}
			}
			if (UpdateItemSearchKeywordFromEdit())
				needFilterRefresh = true;
			if (needFilterRefresh)
			{
				GItemBrowser.CurrentPage = 0;
				FilterItems((catIdx >= 0) ? catIdx : 0);
				RefreshItemPage();
			}

			GItemBrowser.AddQuantity = GetItemAddQuantityFromEdit();
			GUIRememberState.ItemAddQuantity = GItemBrowser.AddQuantity;

			int32 ClickDefId = 0;
			if (TryConsumeItemEntryClickDefId(ClickDefId))
			{
				const bool CtrlDown = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
				const int32 Quantity = CtrlDown ? 10 : ((GItemBrowser.AddQuantity > 0) ? GItemBrowser.AddQuantity : 1);
				if (Quantity <= 1)
				{
					UItemFuncLib::AddItem(ClickDefId, 1);
				}
				else
				{
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

			UButton* PrevInner = GetClickableButton(GItemBrowser.PrevPageBtn);
			bool PrevPressed = PrevInner &&
				IsSafeLiveObject(static_cast<UObject*>(PrevInner)) &&
				PrevInner->IsPressed();
			if (GItemBrowser.PrevWasPressed && !PrevPressed && GItemBrowser.CurrentPage > 0)
			{
				GItemBrowser.CurrentPage--;
				RefreshItemPage();
			}
			GItemBrowser.PrevWasPressed = PrevPressed;

			UButton* NextInner = GetClickableButton(GItemBrowser.NextPageBtn);
			bool NextPressed = NextInner &&
				IsSafeLiveObject(static_cast<UObject*>(NextInner)) &&
				NextInner->IsPressed();
			if (GItemBrowser.NextWasPressed && !NextPressed && (GItemBrowser.CurrentPage + 1) < GItemBrowser.TotalPages)
			{
				GItemBrowser.CurrentPage++;
				RefreshItemPage();
			}
			GItemBrowser.NextWasPressed = NextPressed;
		}
	}
	else
	{
		sLastItemUiPollTick = 0;
		GPendingItemEntryClickDefId.store(0, std::memory_order_release);
		GItemBrowser.PrevWasPressed = false;
		GItemBrowser.NextWasPressed = false;
	}

	static DWORD sLastBattleSliderPollTick = 0;
	if (GInternalWidgetVisible && LiveInternalWidget && IsBattleTabActive)
	{
		const DWORD BattleUiNow = GetTickCount();
		const bool RunBattleSliderPoll =
			(sLastBattleSliderPollTick == 0) || ((BattleUiNow - sLastBattleSliderPollTick) >= 16);
		if (RunBattleSliderPoll)
		{
			sLastBattleSliderPollTick = BattleUiNow;
			PollVolumeItemsButtonsAndText();
		}
	}
	else
	{
		sLastBattleSliderPollTick = 0;
	}

	const bool CanReadTab1FromUI =
		GInternalWidgetVisible &&
		LiveInternalWidget &&
		IsItemsTabActive;
	PollAndApplyTab1Features(CanReadTab1FromUI);

	const bool CanReadTab2FromUI =
		GInternalWidgetVisible &&
		LiveInternalWidget &&
		IsBattleTabActive;
	PollAndApplyTab2Features(CanReadTab2FromUI);

	bool HoverGridValid = false;
	if (GItemBrowser.GridPanel)
	{
		auto* GridObj = static_cast<UObject*>(GItemBrowser.GridPanel);
		HoverGridValid = IsSafeLiveObject(GridObj);
	}
	const bool HasActiveHoverTips =
		(GItemBrowser.HoveredSlot >= 0) ||
		(GItemBrowser.HoverTipsWidget && IsSafeLiveObject(static_cast<UObject*>(GItemBrowser.HoverTipsWidget)));
	const bool CanPollHover =
		GInternalWidgetVisible &&
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
		}
	}

	if (GInternalWidgetVisible && LiveConfigView)
	{
		auto* CV = LiveConfigView;
		auto IsLiveTabBtn = [](UBP_JHConfigTabBtn_C* Btn) -> bool
		{
			return IsSafeLiveObject(static_cast<UObject*>(Btn));
		};

		static int32 sActiveDynTab = -1;
		static int32 sPendingNativeRestoreIdx = -1;
		static int32 sPendingNativeHighlightSettleFrames = 0;
		static int32 sPendingNativeInactiveSettleFrames = 0;
		auto ApplyNativeTabHighlight = [&](int32 ActiveIdx)
		{
			if (IsLiveTabBtn(CV->BTN_Sound))   CV->BTN_Sound->EVT_UpdateActiveStatus(ActiveIdx == 0);
			if (IsLiveTabBtn(CV->BTN_Video))   CV->BTN_Video->EVT_UpdateActiveStatus(ActiveIdx == 1);
			if (IsLiveTabBtn(CV->BTN_Keys))    CV->BTN_Keys->EVT_UpdateActiveStatus(ActiveIdx == 2);
			if (IsLiveTabBtn(CV->BTN_Lan))     CV->BTN_Lan->EVT_UpdateActiveStatus(ActiveIdx == 3);
			if (IsLiveTabBtn(CV->BTN_Others))  CV->BTN_Others->EVT_UpdateActiveStatus(ActiveIdx == 4);
			if (IsLiveTabBtn(CV->BTN_Gamepad)) CV->BTN_Gamepad->EVT_UpdateActiveStatus(ActiveIdx == 5);
		};
		if (HomeNeedDynTabRestore)
		{
			HomeNeedDynTabRestore = false;
			const int32 LastClosedTab = GetLastClosedTabIndex();
			if (LastClosedTab >= 6 && LastClosedTab <= 8)
			{
				sPendingNativeRestoreIdx = -1;
				sPendingNativeHighlightSettleFrames = 0;
				sPendingNativeInactiveSettleFrames = 30;
				ShowDynamicTab(CV, LastClosedTab);
				if (IsLiveTabBtn(GDynTab.Btn6)) GDynTab.Btn6->EVT_UpdateActiveStatus(LastClosedTab == 6);
				if (IsLiveTabBtn(GDynTab.Btn7)) GDynTab.Btn7->EVT_UpdateActiveStatus(LastClosedTab == 7);
				if (IsLiveTabBtn(GDynTab.Btn8)) GDynTab.Btn8->EVT_UpdateActiveStatus(LastClosedTab == 8);
				sActiveDynTab = LastClosedTab;
				LOGI_STREAM("FrameHook") << "[SDK] Restore dynamic tab on HOME-show: idx=" << LastClosedTab << "\n";
			}
			else if (LastClosedTab >= 0 && LastClosedTab <= 5)
			{
				sPendingNativeRestoreIdx = LastClosedTab;
				sPendingNativeHighlightSettleFrames = 0;
				sPendingNativeInactiveSettleFrames = 0;
				sActiveDynTab = -1;
				LOGI_STREAM("FrameHook") << "[SDK] Restore native tab on HOME-show: idx=" << LastClosedTab << "\n";
			}
			else
			{
				sPendingNativeRestoreIdx = -1;
				sPendingNativeHighlightSettleFrames = 0;
				sPendingNativeInactiveSettleFrames = 0;
				sActiveDynTab = -1;
			}
		}

		if ((sPendingNativeRestoreIdx >= 0 && sPendingNativeRestoreIdx <= 5) || sPendingNativeHighlightSettleFrames > 0)
		{
			const int32 TargetIdx = (sPendingNativeRestoreIdx >= 0 && sPendingNativeRestoreIdx <= 5)
				? sPendingNativeRestoreIdx
				: GetLastClosedTabIndex();
			ApplyNativeTabHighlight(TargetIdx);

			int32 CurrentIdx = -1;
			if (CV->CT_Contents &&
				IsSafeLiveObject(static_cast<UObject*>(CV->CT_Contents)))
			{
				CurrentIdx = CV->CT_Contents->GetActiveWidgetIndex();
			}

			if (sPendingNativeRestoreIdx >= 0 && sPendingNativeRestoreIdx <= 5 && CurrentIdx != sPendingNativeRestoreIdx)
			{
				static uint64 sSyncTabInvokeCount = 0;
				++sSyncTabInvokeCount;
				LOGI_STREAM("FrameHook") << "[SDK] EVT_SyncTabIndex call#" << sSyncTabInvokeCount
					<< ": target=" << sPendingNativeRestoreIdx
					<< " current=" << CurrentIdx
					<< "\n";
				CV->EVT_SyncTabIndex(sPendingNativeRestoreIdx);
				ShowOriginalTab(CV);
				if (IsLiveTabBtn(GDynTab.Btn6)) GDynTab.Btn6->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTab.Btn7)) GDynTab.Btn7->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTab.Btn8)) GDynTab.Btn8->EVT_UpdateActiveStatus(false);
				sActiveDynTab = -1;
			}
			else
			{
				if (sPendingNativeRestoreIdx >= 0 && sPendingNativeRestoreIdx <= 5)
				{
					sPendingNativeRestoreIdx = -1;
					sPendingNativeHighlightSettleFrames = 30;
				}
				else if (sPendingNativeHighlightSettleFrames > 0)
				{
					--sPendingNativeHighlightSettleFrames;
				}
			}
		}

		if (sActiveDynTab >= 6 || sPendingNativeInactiveSettleFrames > 0)
		{
			ApplyNativeTabHighlight(-1);
			if (sPendingNativeInactiveSettleFrames > 0)
				--sPendingNativeInactiveSettleFrames;
		}

		int32 dynHoverIdx = -1;
		if      (IsLiveTabBtn(GDynTab.Btn6) && GDynTab.Btn6->IsHovered()) dynHoverIdx = 6;
		else if (IsLiveTabBtn(GDynTab.Btn7) && GDynTab.Btn7->IsHovered()) dynHoverIdx = 7;
		else if (IsLiveTabBtn(GDynTab.Btn8) && GDynTab.Btn8->IsHovered()) dynHoverIdx = 8;

		if (dynHoverIdx >= 6 && dynHoverIdx != sActiveDynTab)
		{
			ShowDynamicTab(CV, dynHoverIdx);
			sPendingNativeInactiveSettleFrames = 30;
			if (IsLiveTabBtn(GDynTab.Btn6)) GDynTab.Btn6->EVT_UpdateActiveStatus(dynHoverIdx == 6);
			if (IsLiveTabBtn(GDynTab.Btn7)) GDynTab.Btn7->EVT_UpdateActiveStatus(dynHoverIdx == 7);
			if (IsLiveTabBtn(GDynTab.Btn8)) GDynTab.Btn8->EVT_UpdateActiveStatus(dynHoverIdx == 8);
			sActiveDynTab = dynHoverIdx;
		}
		else if (sActiveDynTab >= 6 && dynHoverIdx == -1)
		{
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
				sPendingNativeInactiveSettleFrames = 0;
				if (IsLiveTabBtn(GDynTab.Btn6)) GDynTab.Btn6->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTab.Btn7)) GDynTab.Btn7->EVT_UpdateActiveStatus(false);
				if (IsLiveTabBtn(GDynTab.Btn8)) GDynTab.Btn8->EVT_UpdateActiveStatus(false);
				sActiveDynTab = -1;
			}
		}
	}

	if (GInternalWidgetVisible && LiveInternalWidget && !LiveInternalWidget->IsInViewport())
	{
		APlayerController* PC = GetFirstLocalPlayerController();
		HideInternalWidget(PC);
		LOGI_STREAM("FrameHook") << "[SDK] Widget closed externally, cached instance kept\n";
	}
}
