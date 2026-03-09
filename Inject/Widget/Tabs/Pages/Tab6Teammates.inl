#include "SDK/JHGHC_RegionEle_NPC_classes.hpp"

namespace
{

struct FTeammateNpcPrototypeSample
{
	int32 NpcId = 0;
	int32 CommonId = 0;
	std::wstring DisplayName;
	bool bShowDlcMark = false;
	UPaperFlipbook* IdleFlipbook = nullptr;
};

struct FTeammateNpcPrototypeCardBinding
{
	int32 NpcId = 0;
	std::wstring DisplayName;
	bool bShowDlcMark = false;
	UPaperFlipbook* IdleFlipbook = nullptr;
	UButton* ClickButton = nullptr;
	UBorder* FrameBorder = nullptr;
	USizeBox* CardHost = nullptr;
	UJHNeoUIGHC_RegionElement_NPC* CardWidget = nullptr;
	bool bVisualsPrimed = false;
	bool WasPressed = false;
};

std::vector<UObject*> GTeammateNpcPrototypeWidgets;
std::vector<FTeammateNpcPrototypeSample> GTeammateNpcPrototypeAllSamples;
std::vector<int32> GTeammateNpcPrototypeFilteredIndices;
std::vector<FTeammateNpcPrototypeCardBinding> GTeammateNpcPrototypeCards;
UTextBlock* GTeammateNpcPrototypeSelectionLabel = nullptr;
UTextBlock* GTeammateNpcPrototypePageLabel = nullptr;
UEditableTextBox* GTeammateNpcPrototypeSearchEdit = nullptr;
UWrapBox* GTeammateNpcPrototypeCardWrap = nullptr;
APlayerController* GTeammateNpcPrototypeOwnerPC = nullptr;
UObject* GTeammateNpcPrototypeOwnerOuter = nullptr;
UJHCommon_Btn_Free_C* GTeammateNpcPrototypePrevPageBtn = nullptr;
UJHCommon_Btn_Free_C* GTeammateNpcPrototypeNextPageBtn = nullptr;
std::wstring GTeammateNpcPrototypeSearchKeyword;
int32 GTeammateNpcPrototypeSelectedNpcId = 0;
int32 GTeammateNpcPrototypeCurrentPage = 0;
int32 GTeammateNpcPrototypeTotalPages = 0;
bool GTeammateNpcPrototypePrevWasPressed = false;
bool GTeammateNpcPrototypeNextWasPressed = false;
ULONGLONG GTeammateNpcPrototypeLastSelectionPollTick = 0;
ULONGLONG GTeammateNpcPrototypeLastPagerPollTick = 0;
ULONGLONG GTeammateNpcPrototypeLastLayoutLogTick = 0;
bool GTeammateNpcResourceTableSchemaLogged = false;
bool GTeammateNpcNeoUISubsystemLogged = false;
bool GTeammateNpcSimpleAlertProbeLogged = false;
bool GTeammateNpcPrototypePollWasActive = false;
UJHNeoUIAlertContentSimple* GTeammateNpcAddConfirmAlert = nullptr;
UJHNeoUIAlertModuleView* GTeammateNpcAddConfirmView = nullptr;
UButton* GTeammateNpcAddConfirmYesBtn = nullptr;
UButton* GTeammateNpcAddConfirmNoBtn = nullptr;
int32 GTeammateNpcAddConfirmPendingNpcId = 0;
bool GTeammateNpcAddConfirmPending = false;
bool GTeammateNpcAddConfirmYesWasPressed = false;
bool GTeammateNpcAddConfirmNoWasPressed = false;
bool GTeammateNpcAddConfirmYesWasHovered = false;
bool GTeammateNpcAddConfirmNoWasHovered = false;
bool GTeammateNpcAddConfirmLmbWasDown = false;
bool GTeammateNpcAddConfirmPanelZLowered = false;
bool GTeammateNpcAddConfirmInputRerouted = false;
bool GTeammateNpcAddConfirmPanelInputMuted = false;
ESlateVisibility GTeammateNpcAddConfirmPanelPrevVisibility = ESlateVisibility::Visible;
bool GTeammateNpcAddConfirmPanelPrevEnabled = true;
UFunction* GTeammateNpcAlertShow2Func = nullptr;
int32 GTeammateNpcAlertShow2FuncIdx = -1;
std::string GTeammateNpcAlertShow2OwnerClass;

constexpr int32 kTeammateNpcPrototypePerPage = 12;
constexpr float kTeammateNpcCardScaleNormal = 1.0f;
constexpr float kTeammateNpcCardScaleSelected = 1.0f;
constexpr float kTeammateNpcCardHostWidth = 84.0f;
constexpr float kTeammateNpcCardHostHeight = 122.0f;
constexpr float kTeammateNpcMainImageScale = 2.40f;
constexpr float kTeammateNpcMainImageOffsetX = -16.0f;
constexpr float kTeammateNpcMainImageLiftY = -108.0f;
constexpr int32 kTeammateNpcTitleFontSize = 14;
constexpr float kTeammateNpcTitleFollowDeltaX = 0.0f;
constexpr float kTeammateNpcTitleFollowDeltaY = -12.0f;
constexpr float kTeammateNpcTitleOffsetX = kTeammateNpcMainImageOffsetX + kTeammateNpcTitleFollowDeltaX;
constexpr float kTeammateNpcTitleOffsetY = kTeammateNpcMainImageLiftY + kTeammateNpcTitleFollowDeltaY;
constexpr bool kTeammateNpcHideFrameHighlight = true;
constexpr ULONGLONG kTeammateNpcSelectionPollIntervalMs = 33ULL;
constexpr ULONGLONG kTeammateNpcPagerPollIntervalMs = 33ULL;
constexpr bool kTab6NpcPrototypeVerboseLog = true;
constexpr bool kTeammateNpcDumpAlertFunctions = false;
constexpr const wchar_t* kTab6NpcPrototypeBuildTag = L"tab6-npc-page-search-20260309-2012-confirm-show2-priority-2";
constexpr int32 kTeammateNpcNeoUISubsystemHintIdx = 45025;
constexpr int32 kTeammateNpcNeoUISubsystemHintRadius = 10;
constexpr int32 kTeammateNpcConfirmDialogZOrder = 20000;
constexpr int32 kTeammateNpcInternalPanelNormalZOrder = 10000;
constexpr int32 kTeammateNpcInternalPanelBackgroundZOrder = -10000;

int HandleTab6NpcProtoSehException(const char* Stage, int32 NpcId, unsigned int Code)
{
	LOGE_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] SEH guarded crash stage="
		<< (Stage ? Stage : "unknown")
		<< " npcId=" << NpcId
		<< " code=0x" << std::hex << Code << std::dec
		<< "\n";
	return EXCEPTION_EXECUTE_HANDLER;
}

void LogTeammateNpcResourceTableSchema(UDataTable* Table);
void RebuildTeammateNpcPrototypeFilteredIndices();
void RefreshTeammateNpcPrototypePage();
void LogTeammateNpcPrototypeLayout(const char* StageTag);
std::wstring GetTeammateNpcPrototypeName(int32 NpcId, const wchar_t* FallbackName);
UJHNeoUISubsystem* GetJHNeoUISubsystemForTab6(bool bLogSearch);
UFunction* FindTeammateNpcAlertFunctionByFullName(const char* FullFuncName, std::string* OutOwnerClass, int32* OutFuncIdx);
void DumpTeammateNpcAlertUFunctions(UObject* AlertObj);
void ProbeTeammateNpcSimpleAlert();
UButton* ResolveTeammateNpcConfirmInnerButton(UNeoUI1Btn1TxtComp* BtnComp);
void RestoreTeammateNpcAddConfirmUiState();
void EnsureTeammateNpcAddConfirmDialogPriority(UJHNeoUIAlertContentSimple* AlertContent);
void ResetTeammateNpcAddConfirmState();
bool ShowTeammateNpcAddConfirmDialog(int32 NpcId);
void PollTeammateNpcAddConfirmDialog();

std::wstring GetTeammateNpcPrototypeText(const FText& Text)
{
	if (!Text.TextData)
		return {};

	const wchar_t* WS = Text.GetStringRef().CStr();
	if (!WS || !WS[0])
		return {};

	return WS;
}

bool IsTeammateNpcPrototypeUsableName(const std::wstring& Name)
{
	if (Name.empty())
		return false;
	if (Name == L"测试用" || Name == L"未命名NPC" || Name == L"文本块")
		return false;
	return true;
}

std::wstring GetTeammateNpcPrototypeBindingName(int32 NpcId)
{
	for (const auto& Binding : GTeammateNpcPrototypeCards)
	{
		if (Binding.NpcId == NpcId && !Binding.DisplayName.empty())
			return Binding.DisplayName;
	}
	return {};
}

void RememberTeammateNpcPrototypeWidget(UObject* Obj)
{
	if (!Obj)
		return;
	GTeammateNpcPrototypeWidgets.push_back(Obj);
}

void ReleaseTeammateNpcPrototypeWidgets()
{
	for (UObject* Obj : GTeammateNpcPrototypeWidgets)
	{
		if (!Obj)
			continue;
		ClearGCRoot(Obj);
	}
	GTeammateNpcPrototypeWidgets.clear();
	GTeammateNpcPrototypeAllSamples.clear();
	GTeammateNpcPrototypeFilteredIndices.clear();
	GTeammateNpcPrototypeCards.clear();
	GTeammateNpcPrototypeSelectionLabel = nullptr;
	GTeammateNpcPrototypePageLabel = nullptr;
	GTeammateNpcPrototypeSearchEdit = nullptr;
	GTeammateNpcPrototypeCardWrap = nullptr;
	GTeammateNpcPrototypeOwnerPC = nullptr;
	GTeammateNpcPrototypeOwnerOuter = nullptr;
	GTeammateNpcPrototypePrevPageBtn = nullptr;
	GTeammateNpcPrototypeNextPageBtn = nullptr;
	GTeammateNpcPrototypeSearchKeyword.clear();
	GTeammateNpcPrototypeSelectedNpcId = 0;
	GTeammateNpcPrototypeCurrentPage = 0;
	GTeammateNpcPrototypeTotalPages = 0;
	GTeammateNpcPrototypePrevWasPressed = false;
	GTeammateNpcPrototypeNextWasPressed = false;
	GTeammateNpcPrototypeLastSelectionPollTick = 0;
	GTeammateNpcPrototypeLastPagerPollTick = 0;
	GTeammateNpcPrototypeLastLayoutLogTick = 0;
	GTeammateNpcNeoUISubsystemLogged = false;
	GTeammateNpcSimpleAlertProbeLogged = false;
	GTeammateNpcPrototypePollWasActive = false;
	ResetTeammateNpcAddConfirmState();
}

UGameInstance* GetCurrentGameInstanceForTab6()
{
	UWorld* World = UWorld::GetWorld();
	if (!World)
		return nullptr;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (PC && IsSafeLiveObject(static_cast<UObject*>(PC)))
		return UGameplayStatics::GetGameInstance(PC);

	return World->OwningGameInstance;
}

UJHNeoUISubsystem* GetJHNeoUISubsystemForTab6(bool bLogSearch)
{
	static UJHNeoUISubsystem* Cached = nullptr;
	UGameInstance* CurrentGI = GetCurrentGameInstanceForTab6();

	auto IsRuntimeSubsystem = [&](UJHNeoUISubsystem* Subsystem) -> bool
	{
		if (!Subsystem)
			return false;
		auto* Obj = static_cast<UObject*>(Subsystem);
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

	auto LogSubsystem = [&](const char* SourceTag, UJHNeoUISubsystem* Sub, int32 ScanIdx)
	{
		if (!bLogSearch || !Sub)
			return;
		UObject* Obj = static_cast<UObject*>(Sub);
		std::string ClassName = (Obj && Obj->Class) ? Obj->Class->GetName() : std::string("<null>");
		const int32 ObjIdx = Obj ? Obj->Index : -1;
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] NeoUISubsystem found source=" << (SourceTag ? SourceTag : "unknown")
			<< " ptr=" << Sub
			<< " class=" << ClassName.c_str()
			<< " idx=" << ObjIdx
			<< " scanIdx=" << ScanIdx
			<< "\n";
	};

	if (IsRuntimeSubsystem(Cached))
	{
		LogSubsystem("cached", Cached, Cached ? static_cast<UObject*>(Cached)->Index : -1);
		return Cached;
	}

	Cached = nullptr;

	if (CurrentGI)
	{
		auto* SubObj = USubsystemBlueprintLibrary::GetGameInstanceSubsystem(
			static_cast<UObject*>(CurrentGI),
			UJHNeoUISubsystem::StaticClass());
		auto* Sub = static_cast<UJHNeoUISubsystem*>(SubObj);
		if (IsRuntimeSubsystem(Sub))
		{
			Cached = Sub;
			LogSubsystem("game_instance_subsystem", Cached, Cached ? static_cast<UObject*>(Cached)->Index : -1);
			return Cached;
		}
	}

	if (UWorld* World = UWorld::GetWorld())
	{
		auto* SubObj = USubsystemBlueprintLibrary::GetGameInstanceSubsystem(
			static_cast<UObject*>(World),
			UJHNeoUISubsystem::StaticClass());
		auto* Sub = static_cast<UJHNeoUISubsystem*>(SubObj);
		if (IsRuntimeSubsystem(Sub))
		{
			Cached = Sub;
			LogSubsystem("world_subsystem", Cached, Cached ? static_cast<UObject*>(Cached)->Index : -1);
			return Cached;
		}
	}

	auto* ObjArray = UObject::GObjects.GetTypedPtr();
	if (ObjArray)
	{
		const int32 Num = ObjArray->Num();
		const int32 HintBegin = (kTeammateNpcNeoUISubsystemHintIdx - kTeammateNpcNeoUISubsystemHintRadius) > 0
			                        ? (kTeammateNpcNeoUISubsystemHintIdx - kTeammateNpcNeoUISubsystemHintRadius)
			                        : 0;
		const int32 HintEnd = (kTeammateNpcNeoUISubsystemHintIdx + kTeammateNpcNeoUISubsystemHintRadius) < (Num - 1)
			                      ? (kTeammateNpcNeoUISubsystemHintIdx + kTeammateNpcNeoUISubsystemHintRadius)
			                      : (Num - 1);
		for (int32 i = HintBegin; i <= HintEnd; ++i)
		{
			UObject* Obj = ObjArray->GetByIndex(i);
			if (!Obj || !Obj->IsA(UJHNeoUISubsystem::StaticClass()))
				continue;

			auto* Candidate = static_cast<UJHNeoUISubsystem*>(Obj);
			if (!IsRuntimeSubsystem(Candidate))
				continue;

			Cached = Candidate;
			LogSubsystem("gobjects_hint_range", Cached, i);
			return Cached;
		}

		for (int32 i = 0; i < Num; ++i)
		{
			UObject* Obj = ObjArray->GetByIndex(i);
			if (!Obj || !Obj->IsA(UJHNeoUISubsystem::StaticClass()))
				continue;

			auto* Candidate = static_cast<UJHNeoUISubsystem*>(Obj);
			if (!IsRuntimeSubsystem(Candidate))
				continue;

			Cached = Candidate;
			LogSubsystem("gobjects_scan", Cached, i);
			return Cached;
		}
	}

	if (bLogSearch)
	{
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] NeoUISubsystem not found\n";
	}
	return nullptr;
}

UFunction* FindTeammateNpcAlertFunctionByFullName(const char* FullFuncName, std::string* OutOwnerClass, int32* OutFuncIdx)
{
	if (OutOwnerClass)
		OutOwnerClass->clear();
	if (OutFuncIdx)
		*OutFuncIdx = -1;

	if (!FullFuncName || !FullFuncName[0])
		return nullptr;

	auto* ObjArray = UObject::GObjects.GetTypedPtr();
	if (!ObjArray)
		return nullptr;

	const int32 Num = ObjArray->Num();
	for (int32 i = 0; i < Num; ++i)
	{
		UObject* Obj = ObjArray->GetByIndex(i);
		if (!Obj || !IsSafeLiveObject(Obj) || !Obj->IsA(UFunction::StaticClass()))
			continue;

		const std::string FullName = Obj->GetFullName();
		if (FullName != FullFuncName)
			continue;

		if (OutFuncIdx)
			*OutFuncIdx = Obj->Index;

		if (OutOwnerClass)
		{
			if (Obj->Outer && IsSafeLiveObject(Obj->Outer))
				*OutOwnerClass = Obj->Outer->GetName();
			else
				*OutOwnerClass = "<null>";
		}

		return static_cast<UFunction*>(Obj);
	}

	return nullptr;
}

#if 0
void ProbeTeammateNpcSimpleAlert_Legacy()
{
	UJHNeoUISubsystem* Subsystem = GetJHNeoUISubsystemForTab6(false);
	if (!Subsystem)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe fail: subsystem null\n";
		return;
	}

	const TScriptInterface<IJHNeoUIAlert_Content_Simple> AlertIface = Subsystem->BPSimpleAlert();
	UObject* AlertObj = AlertIface.ObjectPointer;
	void* AlertInterface = AlertIface.InterfacePointer;
	std::string ClassName = "<null>";
	int32 ObjIdx = -1;
	int32 ObjValid = 0;
	int32 IsPcSimple = 0;
	int32 IsMobileSimple = 0;
	if (AlertObj)
	{
		if (AlertObj->Class)
			ClassName = AlertObj->Class->GetName();
		ObjIdx = AlertObj->Index;
		ObjValid = IsSafeLiveObject(AlertObj) ? 1 : 0;
		IsPcSimple = AlertObj->IsA(UJHNeoUIAlertContentSimple::StaticClass()) ? 1 : 0;
		IsMobileSimple = AlertObj->IsA(UJHNeoUIAlertContentSimple_mobile::StaticClass()) ? 1 : 0;
	}

	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] BPSimpleAlert probe sub=" << Subsystem
		<< " subClass=JHNeoUISubsystem"
		<< " subIdx=" << static_cast<UObject*>(Subsystem)->Index
		<< " retObj=" << AlertObj
		<< " retIface=" << AlertInterface
		<< " retClass=" << ClassName.c_str()
		<< " retIdx=" << ObjIdx
		<< " retObjValid=" << ObjValid
		<< " isPcSimple=" << IsPcSimple
		<< " isMobileSimple=" << IsMobileSimple
		<< "\n";

	DumpTeammateNpcAlertUFunctions(AlertObj);

	if (!AlertObj || !ObjValid)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe show1 skip: invalid retObj\n";
		return;
	}

	IJHNeoUIAlert_Content_Simple* AlertFromObj = reinterpret_cast<IJHNeoUIAlert_Content_Simple*>(AlertObj);
	if (!AlertFromObj)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe show1 skip: cast null\n";
		return;
	}

	std::string Show1OwnerClass = "<unknown>";
	UFunction* Show1Func = nullptr;
	if (AlertObj && AlertObj->Class)
	{
		Show1Func = AlertObj->Class->GetFunction("JHNeoUIAlert_Content_Simple", "BP_Show1");
		if (Show1Func)
			Show1OwnerClass = "JHNeoUIAlert_Content_Simple::BP_Show1";
	}
	if (!Show1Func && AlertObj && AlertObj->Class)
	{
		Show1Func = AlertObj->Class->GetFunction("JHNeoUIAlertContentSimple_mobile", "BP_Show1");
		if (Show1Func)
			Show1OwnerClass = "JHNeoUIAlertContentSimple_mobile::BP_Show1";
	}
	if (!Show1Func)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe show1 unresolved obj=" << AlertObj
			<< " class=" << ClassName.c_str()
			<< " iface=" << AlertInterface
			<< "\n";
		return;
	}

	const FText DescText = MakeText(L"[SDK] BPSimpleAlert 测试：Show1");
	const FText BtnText = MakeText(L"确定");
	if (Show1OwnerClass.rfind("JHNeoUIAlert_Content_Simple::", 0) == 0)
	{
		AlertFromObj->BP_Show1(DescText, BtnText);
	}
	else if (Show1OwnerClass.rfind("JHNeoUIAlertContentSimple_mobile::", 0) == 0 &&
		AlertObj->IsA(UJHNeoUIAlertContentSimple_mobile::StaticClass()))
	{
		auto* MobileSimple = static_cast<UJHNeoUIAlertContentSimple_mobile*>(AlertObj);
		MobileSimple->BP_Show1(DescText, BtnText);
	}
	else
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe show1 skip: unsupported owner="
			<< Show1OwnerClass.c_str() << "\n";
		return;
	}
	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] BPSimpleAlert probe show1 called obj=" << AlertObj
		<< " class=" << ClassName.c_str()
		<< " idx=" << ObjIdx
		<< " owner=" << Show1OwnerClass.c_str()
		<< "\n";
}
#endif

void ProbeTeammateNpcSimpleAlert()
{
	UJHNeoUISubsystem* Subsystem = GetJHNeoUISubsystemForTab6(false);
	if (!Subsystem)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe fail: subsystem null\n";
		return;
	}

	const TScriptInterface<IJHNeoUIAlert_Content_Simple> AlertIface = Subsystem->BPSimpleAlert();
	UObject* AlertObj = AlertIface.ObjectPointer;
	void* AlertInterface = AlertIface.InterfacePointer;
	std::string ClassName = "<null>";
	int32 ObjIdx = -1;
	int32 ObjValid = 0;
	int32 IsPcSimple = 0;
	int32 IsMobileSimple = 0;
	if (AlertObj)
	{
		if (AlertObj->Class)
			ClassName = AlertObj->Class->GetName();
		ObjIdx = AlertObj->Index;
		ObjValid = IsSafeLiveObject(AlertObj) ? 1 : 0;
		IsPcSimple = AlertObj->IsA(UJHNeoUIAlertContentSimple::StaticClass()) ? 1 : 0;
		IsMobileSimple = AlertObj->IsA(UJHNeoUIAlertContentSimple_mobile::StaticClass()) ? 1 : 0;
	}

	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] BPSimpleAlert probe sub=" << Subsystem
		<< " subClass=JHNeoUISubsystem"
		<< " subIdx=" << static_cast<UObject*>(Subsystem)->Index
		<< " retObj=" << AlertObj
		<< " retIface=" << AlertInterface
		<< " retClass=" << ClassName.c_str()
		<< " retIdx=" << ObjIdx
		<< " retObjValid=" << ObjValid
		<< " isPcSimple=" << IsPcSimple
		<< " isMobileSimple=" << IsMobileSimple
		<< "\n";

	if (kTeammateNpcDumpAlertFunctions)
		DumpTeammateNpcAlertUFunctions(AlertObj);

	if (!AlertObj || !ObjValid)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe skip: invalid retObj\n";
		return;
	}

	std::string Show1OwnerClass;
	int32 Show1FuncIdx = -1;
	UFunction* Show1Func = FindTeammateNpcAlertFunctionByFullName(
		"Function JH.JHNeoUIAlert_Content_Simple.BP_Show1",
		&Show1OwnerClass,
		&Show1FuncIdx);

	std::string Show2OwnerClass;
	int32 Show2FuncIdx = -1;
	UFunction* Show2Func = FindTeammateNpcAlertFunctionByFullName(
		"Function JH.JHNeoUIAlert_Content_Simple.BP_Show2",
		&Show2OwnerClass,
		&Show2FuncIdx);

	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] BPSimpleAlert probe resolve show1 ptr=" << Show1Func
		<< " idx=" << Show1FuncIdx
		<< " owner=" << (Show1OwnerClass.empty() ? "<null>" : Show1OwnerClass.c_str())
		<< " | show2 ptr=" << Show2Func
		<< " idx=" << Show2FuncIdx
		<< " owner=" << (Show2OwnerClass.empty() ? "<null>" : Show2OwnerClass.c_str())
		<< "\n";

	if (!Show2Func)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] BPSimpleAlert probe show2 unresolved obj=" << AlertObj
			<< " class=" << ClassName.c_str()
			<< " iface=" << AlertInterface
			<< "\n";
	}
	else
	{
		GTeammateNpcAlertShow2Func = Show2Func;
		GTeammateNpcAlertShow2FuncIdx = Show2FuncIdx;
		GTeammateNpcAlertShow2OwnerClass = Show2OwnerClass;
	}

	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] BPSimpleAlert probe ready(no-auto-show) obj=" << AlertObj
		<< " class=" << ClassName.c_str()
		<< " idx=" << ObjIdx
		<< " show2Owner=" << (Show2OwnerClass.empty() ? "<null>" : Show2OwnerClass.c_str())
		<< " show2Idx=" << Show2FuncIdx
		<< "\n";
}

void DumpTeammateNpcAlertUFunctions(UObject* AlertObj)
{
	if (!AlertObj || !AlertObj->Class || !IsSafeLiveObject(AlertObj))
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto][FuncDump] skip: alert object invalid\n";
		return;
	}

	constexpr int32 kMaxPerClass = 120;
	constexpr int32 kMaxTotal = 600;
	int32 TotalCount = 0;
	int32 Depth = 0;

	for (UStruct* Clss = AlertObj->Class; Clss && TotalCount < kMaxTotal; Clss = Clss->SuperStruct, ++Depth)
	{
		std::string ClassName = Clss->GetName();
		int32 ClassCount = 0;
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto][FuncDump] class depth=" << Depth
			<< " name=" << ClassName.c_str()
			<< " ptr=" << Clss
			<< "\n";

		for (UField* Field = Clss->Children; Field && TotalCount < kMaxTotal; Field = Field->Next)
		{
			if (!Field->HasTypeFlag(EClassCastFlags::Function))
				continue;

			std::string FuncName = Field->GetName();
			LOGI_STREAM("Tab6Teammates")
				<< "[SDK][Tab6NpcProto][FuncDump]  depth=" << Depth
				<< " class=" << ClassName.c_str()
				<< " fn=" << FuncName.c_str()
				<< "\n";

			++ClassCount;
			++TotalCount;
			if (ClassCount >= kMaxPerClass)
			{
				LOGI_STREAM("Tab6Teammates")
					<< "[SDK][Tab6NpcProto][FuncDump]  class-truncated depth=" << Depth
					<< " name=" << ClassName.c_str()
					<< " limit=" << kMaxPerClass
					<< "\n";
				break;
			}
		}

		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto][FuncDump] class-summary depth=" << Depth
			<< " name=" << ClassName.c_str()
			<< " fnCount=" << ClassCount
			<< " total=" << TotalCount
			<< "\n";
	}

	if (TotalCount >= kMaxTotal)
	{
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto][FuncDump] total-truncated limit=" << kMaxTotal
			<< "\n";
	}
}

UButton* ResolveTeammateNpcConfirmInnerButton(UNeoUI1Btn1TxtComp* BtnComp)
{
	if (!BtnComp || !IsSafeLiveObjectOfClass(static_cast<UObject*>(BtnComp), UNeoUI1Btn1TxtComp::StaticClass()))
		return nullptr;

	UNeoUIButtonBase* Inner = BtnComp->Btn;
	if (!Inner || !IsSafeLiveObjectOfClass(static_cast<UObject*>(Inner), UButton::StaticClass()))
		return nullptr;

	return static_cast<UButton*>(Inner);
}

void ResetTeammateNpcAddConfirmState()
{
	RestoreTeammateNpcAddConfirmUiState();

	GTeammateNpcAddConfirmAlert = nullptr;
	GTeammateNpcAddConfirmView = nullptr;
	GTeammateNpcAddConfirmYesBtn = nullptr;
	GTeammateNpcAddConfirmNoBtn = nullptr;
	GTeammateNpcAddConfirmPendingNpcId = 0;
	GTeammateNpcAddConfirmPending = false;
	GTeammateNpcAddConfirmYesWasPressed = false;
	GTeammateNpcAddConfirmNoWasPressed = false;
	GTeammateNpcAddConfirmYesWasHovered = false;
	GTeammateNpcAddConfirmNoWasHovered = false;
	GTeammateNpcAddConfirmLmbWasDown = false;
}

void RestoreTeammateNpcAddConfirmUiState()
{
	if (GTeammateNpcAddConfirmPanelInputMuted &&
		GInternalWidget &&
		IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)))
	{
		GInternalWidget->SetVisibility(GTeammateNpcAddConfirmPanelPrevVisibility);
		GInternalWidget->SetIsEnabled(GTeammateNpcAddConfirmPanelPrevEnabled);
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog ui restore: panel visibility/enabled restored\n";
	}
	GTeammateNpcAddConfirmPanelInputMuted = false;

	if (GTeammateNpcAddConfirmInputRerouted &&
		GTeammateNpcPrototypeOwnerPC &&
		IsSafeLiveObject(static_cast<UObject*>(GTeammateNpcPrototypeOwnerPC)))
	{
		if (GInternalWidget &&
			IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)) &&
			GInternalWidget->IsInViewport())
		{
			UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(
				GTeammateNpcPrototypeOwnerPC,
				GInternalWidget,
				EMouseLockMode::DoNotLock,
				false);
			GInternalWidget->SetKeyboardFocus();
		}
		GTeammateNpcAddConfirmInputRerouted = false;
	}

	if (GTeammateNpcAddConfirmPanelZLowered &&
		GInternalWidget &&
		IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)) &&
		GInternalWidget->IsInViewport())
	{
		GInternalWidget->RemoveFromParent();
		GInternalWidget->AddToViewport(kTeammateNpcInternalPanelNormalZOrder);
	}
	GTeammateNpcAddConfirmPanelZLowered = false;
}

void EnsureTeammateNpcAddConfirmDialogPriority(UJHNeoUIAlertContentSimple* AlertContent)
{
	if (!AlertContent || !IsSafeLiveObjectOfClass(static_cast<UObject*>(AlertContent), UJHNeoUIAlertContentSimple::StaticClass()))
		return;

	UNeoUIUniversalModuleVMBase* AlertVMBase = AlertContent->PendingVM;

	GTeammateNpcAddConfirmView = nullptr;

	if (AlertVMBase && IsSafeLiveObject(static_cast<UObject*>(AlertVMBase)))
	{
		UJHNeoUIAlertModuleView* AlertView = nullptr;
		if (AlertVMBase->IsA(UJHNeoUIAlertModuleVM::StaticClass()))
		{
			auto* AlertVM = static_cast<UJHNeoUIAlertModuleVM*>(AlertVMBase);
			AlertView = AlertVM->GetAlertView();
		}

		if (!AlertView)
		{
			UNeoUIBlueprintableVisualBase* PairView = AlertVMBase->GetPairView();
			if (PairView && PairView->IsA(UJHNeoUIAlertModuleView::StaticClass()))
				AlertView = static_cast<UJHNeoUIAlertModuleView*>(PairView);
		}

		if (AlertView && IsSafeLiveObjectOfClass(static_cast<UObject*>(AlertView), UJHNeoUIAlertModuleView::StaticClass()))
		{
			GTeammateNpcAddConfirmView = AlertView;
			AlertView->SetVisibility(ESlateVisibility::Visible);
			UCanvasPanelSlot* LayerSlot = AlertView->GetLayerSlot();
			if (LayerSlot && IsSafeLiveObjectOfClass(static_cast<UObject*>(LayerSlot), UCanvasPanelSlot::StaticClass()))
				LayerSlot->SetZOrder(kTeammateNpcConfirmDialogZOrder);
		}
	}

	if (GInternalWidget &&
		IsSafeLiveObject(static_cast<UObject*>(GInternalWidget)) &&
		GInternalWidget->IsInViewport())
	{
		if (!GTeammateNpcAddConfirmPanelInputMuted)
		{
			GTeammateNpcAddConfirmPanelPrevVisibility = GInternalWidget->GetVisibility();
			GTeammateNpcAddConfirmPanelPrevEnabled = GInternalWidget->GetIsEnabled();
		}
		GInternalWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		GInternalWidget->SetIsEnabled(false);
		GTeammateNpcAddConfirmPanelInputMuted = true;

		GInternalWidget->RemoveFromParent();
		GInternalWidget->AddToViewport(kTeammateNpcInternalPanelBackgroundZOrder);
		GTeammateNpcAddConfirmPanelZLowered = true;
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog ui mute: panel demoted z=" << kTeammateNpcInternalPanelBackgroundZOrder
			<< " visibility=SelfHitTestInvisible enabled=0\n";
	}

	if (GTeammateNpcPrototypeOwnerPC &&
		IsSafeLiveObject(static_cast<UObject*>(GTeammateNpcPrototypeOwnerPC)))
	{
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(
			GTeammateNpcPrototypeOwnerPC,
			AlertContent,
			EMouseLockMode::DoNotLock,
			false);
		AlertContent->SetKeyboardFocus();
		GTeammateNpcPrototypeOwnerPC->bShowMouseCursor = true;
		GTeammateNpcAddConfirmInputRerouted = true;
	}
}

bool AddTeammateNpcToTeamById(int32 NpcId)
{
	if (NpcId <= 0)
		return false;

	UTeamManager* TeamMgr = UManagerFuncLib::GetTeamManager();
	if (!TeamMgr || !IsSafeLiveObject(static_cast<UObject*>(TeamMgr)))
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog add fail: team manager invalid npcId=" << NpcId << "\n";
		return false;
	}

	TeamMgr->AddTeamInfo(NpcId, true, true, true);
	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] ConfirmDialog add ok npcId=" << NpcId << "\n";
	return true;
}

bool ShowTeammateNpcAddConfirmDialog(int32 NpcId)
{
	if (NpcId <= 0)
		return false;

	UJHNeoUISubsystem* Subsystem = GetJHNeoUISubsystemForTab6(false);
	if (!Subsystem)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog fail: subsystem null npcId=" << NpcId << "\n";
		return false;
	}

	const TScriptInterface<IJHNeoUIAlert_Content_Simple> AlertIface = Subsystem->BPSimpleAlert();
	UObject* AlertObj = AlertIface.ObjectPointer;
	void* AlertIfacePtr = AlertIface.InterfacePointer;
	if (!AlertObj || !IsSafeLiveObject(AlertObj))
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog fail: alert object invalid npcId=" << NpcId << "\n";
		return false;
	}

	if (!GTeammateNpcAlertShow2Func || !IsSafeLiveObject(static_cast<UObject*>(GTeammateNpcAlertShow2Func)))
	{
		GTeammateNpcAlertShow2Func = FindTeammateNpcAlertFunctionByFullName(
			"Function JH.JHNeoUIAlert_Content_Simple.BP_Show2",
			&GTeammateNpcAlertShow2OwnerClass,
			&GTeammateNpcAlertShow2FuncIdx);
	}

	if (!GTeammateNpcAlertShow2Func)
	{
		std::string ObjClassName = AlertObj->Class ? AlertObj->Class->GetName() : std::string("<null>");
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog fail: BP_Show2 unresolved obj=" << AlertObj
			<< " class=" << ObjClassName.c_str()
			<< " iface=" << AlertIfacePtr
			<< "\n";
		return false;
	}

	ResetTeammateNpcAddConfirmState();

	std::wstring DisplayName = GetTeammateNpcPrototypeName(NpcId, L"NPC");
	if (DisplayName.empty())
		DisplayName = L"NPC";
	std::wstring Desc = L"\u662F\u5426\u6DFB\u52A0 ";
	Desc += DisplayName;
	Desc += L" \u5230\u961F\u4F0D\uFF1F";

	struct FAlertShow2Params
	{
		FText Desc;
		FText BtnTitle1;
		FText BtnTitle2;
	};

	FAlertShow2Params Parms{};
	Parms.Desc = MakeText(Desc.c_str());
	Parms.BtnTitle1 = MakeText(L"\u662F");
	Parms.BtnTitle2 = MakeText(L"\u5426");

	auto Flgs = GTeammateNpcAlertShow2Func->FunctionFlags;
	GTeammateNpcAlertShow2Func->FunctionFlags |= 0x400;
	AlertObj->ProcessEvent(GTeammateNpcAlertShow2Func, &Parms);
	GTeammateNpcAlertShow2Func->FunctionFlags = Flgs;

	if (!AlertObj->IsA(UJHNeoUIAlertContentSimple::StaticClass()))
	{
		std::string ObjClassName = AlertObj->Class ? AlertObj->Class->GetName() : std::string("<null>");
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog fail: alert class not UJHNeoUIAlertContentSimple class=" << ObjClassName.c_str() << "\n";
		return false;
	}

	auto* AlertContent = static_cast<UJHNeoUIAlertContentSimple*>(AlertObj);
	EnsureTeammateNpcAddConfirmDialogPriority(AlertContent);
	GTeammateNpcAddConfirmAlert = AlertContent;
	GTeammateNpcAddConfirmYesBtn = ResolveTeammateNpcConfirmInnerButton(AlertContent->Btn_1);
	GTeammateNpcAddConfirmNoBtn = ResolveTeammateNpcConfirmInnerButton(AlertContent->Btn_2);
	if (!GTeammateNpcAddConfirmYesBtn || !GTeammateNpcAddConfirmNoBtn)
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog fail: yes/no inner button unresolved\n";
		ResetTeammateNpcAddConfirmState();
		return false;
	}

	GTeammateNpcAddConfirmPendingNpcId = NpcId;
	GTeammateNpcAddConfirmPending = true;
	GTeammateNpcAddConfirmYesWasPressed = GTeammateNpcAddConfirmYesBtn->IsPressed();
	GTeammateNpcAddConfirmNoWasPressed = GTeammateNpcAddConfirmNoBtn->IsPressed();
	GTeammateNpcAddConfirmYesWasHovered = GTeammateNpcAddConfirmYesBtn->IsHovered();
	GTeammateNpcAddConfirmNoWasHovered = GTeammateNpcAddConfirmNoBtn->IsHovered();
	GTeammateNpcAddConfirmLmbWasDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] ConfirmDialog show2 ok npcId=" << NpcId
		<< " obj=" << AlertObj
		<< " iface=" << AlertIfacePtr
		<< " owner=" << (GTeammateNpcAlertShow2OwnerClass.empty() ? "<null>" : GTeammateNpcAlertShow2OwnerClass.c_str())
		<< " idx=" << GTeammateNpcAlertShow2FuncIdx
		<< " z=" << kTeammateNpcConfirmDialogZOrder
		<< "\n";
	return true;
}

void PollTeammateNpcAddConfirmDialog()
{
	if (!GTeammateNpcAddConfirmPending)
		return;

	const bool bLmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	const bool bLmbReleased = GTeammateNpcAddConfirmLmbWasDown && !bLmbDown;

	if (!GTeammateNpcAddConfirmAlert ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcAddConfirmAlert), UJHNeoUIAlertContentSimple::StaticClass()))
	{
		if (bLmbReleased && GTeammateNpcAddConfirmYesWasHovered)
		{
			const int32 TargetNpcId = GTeammateNpcAddConfirmPendingNpcId;
			ResetTeammateNpcAddConfirmState();
			(void)AddTeammateNpcToTeamById(TargetNpcId);
			GTeammateNpcAddConfirmLmbWasDown = bLmbDown;
			return;
		}
		ResetTeammateNpcAddConfirmState();
		GTeammateNpcAddConfirmLmbWasDown = bLmbDown;
		return;
	}

	if (!GTeammateNpcAddConfirmYesBtn ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcAddConfirmYesBtn), UButton::StaticClass()))
	{
		GTeammateNpcAddConfirmYesBtn = ResolveTeammateNpcConfirmInnerButton(GTeammateNpcAddConfirmAlert->Btn_1);
	}
	if (!GTeammateNpcAddConfirmNoBtn ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcAddConfirmNoBtn), UButton::StaticClass()))
	{
		GTeammateNpcAddConfirmNoBtn = ResolveTeammateNpcConfirmInnerButton(GTeammateNpcAddConfirmAlert->Btn_2);
	}

	if (!GTeammateNpcAddConfirmYesBtn || !GTeammateNpcAddConfirmNoBtn)
		return;

	const bool bYesPressed = GTeammateNpcAddConfirmYesBtn->IsPressed();
	const bool bNoPressed = GTeammateNpcAddConfirmNoBtn->IsPressed();
	const bool bYesHovered = GTeammateNpcAddConfirmYesBtn->IsHovered();
	const bool bNoHovered = GTeammateNpcAddConfirmNoBtn->IsHovered();

	const bool bYesTriggered =
		(!GTeammateNpcAddConfirmYesWasPressed && bYesPressed) ||
		(GTeammateNpcAddConfirmYesWasPressed && !bYesPressed);
	const bool bNoTriggered =
		(!GTeammateNpcAddConfirmNoWasPressed && bNoPressed) ||
		(GTeammateNpcAddConfirmNoWasPressed && !bNoPressed);
	const bool bYesMouseTriggered = bLmbReleased && (bYesHovered || GTeammateNpcAddConfirmYesWasHovered);
	const bool bNoMouseTriggered = bLmbReleased && (bNoHovered || GTeammateNpcAddConfirmNoWasHovered);

	if (bYesTriggered || bYesMouseTriggered)
	{
		const int32 TargetNpcId = GTeammateNpcAddConfirmPendingNpcId;
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog yes trigger: source="
			<< (bYesMouseTriggered ? "mouse_release_hover" : "button_press_state")
			<< " npcId=" << TargetNpcId << "\n";
		ResetTeammateNpcAddConfirmState();
		(void)AddTeammateNpcToTeamById(TargetNpcId);
		GTeammateNpcAddConfirmLmbWasDown = bLmbDown;
		return;
	}

	if (bNoTriggered || bNoMouseTriggered)
	{
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog no trigger: source="
			<< (bNoMouseTriggered ? "mouse_release_hover" : "button_press_state")
			<< "\n";
		ResetTeammateNpcAddConfirmState();
		GTeammateNpcAddConfirmLmbWasDown = bLmbDown;
		return;
	}

	GTeammateNpcAddConfirmYesWasPressed = bYesPressed;
	GTeammateNpcAddConfirmNoWasPressed = bNoPressed;
	GTeammateNpcAddConfirmYesWasHovered = bYesHovered;
	GTeammateNpcAddConfirmNoWasHovered = bNoHovered;
	GTeammateNpcAddConfirmLmbWasDown = bLmbDown;
}

std::wstring GetTeammateNpcPrototypeName(int32 NpcId, const wchar_t* FallbackName)
{
	const FText NameText = UNPCFuncLib::GetNPCNameById(NpcId);
	if (NameText.TextData)
	{
		const wchar_t* WS = NameText.GetStringRef().CStr();
		if (WS && WS[0])
			return WS;
	}
	return FallbackName ? std::wstring(FallbackName) : std::wstring();
}

FLinearColor GetTeammateNpcPrototypeFrameColor(bool bSelected)
{
	if (kTeammateNpcHideFrameHighlight)
		return FLinearColor{ 0.0f, 0.0f, 0.0f, 0.0f };

	if (bSelected)
		return FLinearColor{ 0.52f, 0.70f, 0.92f, 0.96f };
	return FLinearColor{ 0.0f, 0.0f, 0.0f, 0.0f };
}

bool HasTeammateNpcPrototypeCardBinding(int32 NpcId)
{
	for (const auto& Binding : GTeammateNpcPrototypeCards)
	{
		if (Binding.NpcId == NpcId)
			return true;
	}
	return false;
}

void EnsureTeammateNpcPrototypeCardTitleStyle(FTeammateNpcPrototypeCardBinding& Binding)
{
	if (!Binding.CardWidget ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(Binding.CardWidget), UJHNeoUIGHC_RegionElement_NPC::StaticClass()))
	{
		return;
	}

	auto* Title = Binding.CardWidget->TXT_Title;
	if (!Title || !IsSafeLiveObject(static_cast<UObject*>(Title)))
		return;

	Title->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FSlateFontInfo TitleFont = Title->Font;
	TitleFont.Size = kTeammateNpcTitleFontSize;
	Title->SetFont(TitleFont);
	Title->SetRenderTransformPivot(FVector2D{ 0.5f, 1.0f });
	Title->SetRenderTranslation(FVector2D{ kTeammateNpcTitleOffsetX, kTeammateNpcTitleOffsetY });
}

void UpdateTeammateNpcPrototypeSelectionSummary()
{
	if (!GTeammateNpcPrototypeSelectionLabel ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcPrototypeSelectionLabel), UTextBlock::StaticClass()))
	{
		return;
	}

	std::wstring Text = L"当前选择：";
	if (HasTeammateNpcPrototypeCardBinding(GTeammateNpcPrototypeSelectedNpcId))
	{
		std::wstring DisplayName = GetTeammateNpcPrototypeBindingName(GTeammateNpcPrototypeSelectedNpcId);
		if (DisplayName.empty())
			DisplayName = GetTeammateNpcPrototypeName(GTeammateNpcPrototypeSelectedNpcId, L"未命名NPC");
		Text += DisplayName;
		Text += L"  |  当前只是高亮选择，不会直接入队。";
	}
	else
	{
		Text += L"未选择  |  点击卡片只做选择高亮。";
	}
	Text += L"  |  页码 ";
	Text += std::to_wstring(GTeammateNpcPrototypeCurrentPage + 1);
	Text += L"/";
	Text += std::to_wstring((GTeammateNpcPrototypeTotalPages > 0) ? GTeammateNpcPrototypeTotalPages : 1);
	Text += L"  |  结果 ";
	Text += std::to_wstring(static_cast<int32>(GTeammateNpcPrototypeFilteredIndices.size()));
	Text += L"/";
	Text += std::to_wstring(static_cast<int32>(GTeammateNpcPrototypeAllSamples.size()));

	GTeammateNpcPrototypeSelectionLabel->SetText(MakeText(Text.c_str()));
}

void ApplyTeammateNpcPrototypeSelectionVisuals()
{
	for (auto& Binding : GTeammateNpcPrototypeCards)
	{
		const bool bSelected = (Binding.NpcId == GTeammateNpcPrototypeSelectedNpcId);

		if (Binding.FrameBorder &&
			IsSafeLiveObjectOfClass(static_cast<UObject*>(Binding.FrameBorder), UBorder::StaticClass()))
		{
			Binding.FrameBorder->SetBrushColor(GetTeammateNpcPrototypeFrameColor(bSelected));
			Binding.FrameBorder->SetContentColorAndOpacity(FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		if (Binding.CardWidget &&
			IsSafeLiveObjectOfClass(static_cast<UObject*>(Binding.CardWidget), UJHNeoUIGHC_RegionElement_NPC::StaticClass()))
		{
			FVector2D Scale{};
			Scale.X = bSelected ? kTeammateNpcCardScaleSelected : kTeammateNpcCardScaleNormal;
			Scale.Y = bSelected ? kTeammateNpcCardScaleSelected : kTeammateNpcCardScaleNormal;
			Binding.CardWidget->SetRenderScale(Scale);
			Binding.CardWidget->SetRenderOpacity(bSelected ? 1.0f : 0.88f);
		}

		EnsureTeammateNpcPrototypeCardTitleStyle(Binding);
	}

	UpdateTeammateNpcPrototypeSelectionSummary();
}

#if 0 // 已禁用：点击 NPC 确认弹窗逻辑
bool ShowTeammateNpcAddConfirmDialog(int32 NpcId)
{
	std::wstring DisplayName = GetTeammateNpcPrototypeName(NpcId, L"NPC");
	if (DisplayName.empty())
		DisplayName = L"NPC";

	std::wstring Desc = L"\u662F\u5426\u6DFB\u52A0 ";
	Desc += DisplayName;
	Desc += L" \u5230\u961F\u4F0D\uFF1F";

	FText DescText = MakeText(Desc.c_str());
	FText YesText = MakeText(L"\u662F");
	FText NoText = MakeText(L"\u5426");

	struct FAlertShow2Params
	{
		FText Desc;
		FText BtnTitle1;
		FText BtnTitle2;
	};

	std::string Show2OwnerClass = "<unknown>";
	UFunction* Show2Func = ResolveTeammateNpcAlertShow2Function(AlertObj, &Show2OwnerClass);

	if (!Show2Func)
	{
		std::string ObjClassName = "<null>";
		if (AlertObj->Class)
			ObjClassName = AlertObj->Class->GetName();
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog fail: BP_Show2 unresolved obj=" << AlertObj
			<< " class=" << ObjClassName.c_str()
			<< " iface=" << AlertIfacePtr
			<< "\n";
		return false;
	}

	FAlertShow2Params Parms{};
	Parms.Desc = DescText;
	Parms.BtnTitle1 = YesText;
	Parms.BtnTitle2 = NoText;

	bool bShow2CalledBySdkWrapper = false;
	if (Show2OwnerClass.rfind("JHNeoUIAlert_Content_Simple::", 0) == 0)
	{
		IJHNeoUIAlert_Content_Simple* AlertIfaceFromObj = reinterpret_cast<IJHNeoUIAlert_Content_Simple*>(AlertObj);
		if (AlertIfaceFromObj)
		{
			AlertIfaceFromObj->BP_Show2(DescText, YesText, NoText);
			bShow2CalledBySdkWrapper = true;
		}
	}
	else if (Show2OwnerClass.rfind("JHNeoUIAlertContentSimple_mobile::", 0) == 0 &&
		AlertObj->IsA(UJHNeoUIAlertContentSimple_mobile::StaticClass()))
	{
		UJHNeoUIAlertContentSimple_mobile* MobileSimple = static_cast<UJHNeoUIAlertContentSimple_mobile*>(AlertObj);
		MobileSimple->BP_Show2(DescText, YesText, NoText);
		bShow2CalledBySdkWrapper = true;
	}

	if (!bShow2CalledBySdkWrapper)
	{
		auto Flgs = Show2Func->FunctionFlags;
		Show2Func->FunctionFlags |= 0x400;
		AlertObj->ProcessEvent(Show2Func, &Parms);
		Show2Func->FunctionFlags = Flgs;
	}

	// BP_Show2 的 this 需要 UObject 语义，不能传 TScriptInterface 的 InterfacePtr。

	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] ConfirmDialog show2 ok owner=" << Show2OwnerClass.c_str()
		<< " obj=" << AlertObj
		<< " iface=" << AlertIfacePtr
		<< " sdkCall=" << (bShow2CalledBySdkWrapper ? 1 : 0)
		<< "\n";

	UJHNeoUIAlertContentSimple* AlertContent = nullptr;
	if (AlertObj->IsA(UJHNeoUIAlertContentSimple::StaticClass()))
		AlertContent = static_cast<UJHNeoUIAlertContentSimple*>(AlertObj);

	GTeammateNpcAddConfirmAlert = AlertContent;
	GTeammateNpcAddConfirmYesBtn = AlertContent ? ResolveTeammateNpcConfirmInnerButton(AlertContent->Btn_1) : nullptr;
	GTeammateNpcAddConfirmNoBtn = AlertContent ? ResolveTeammateNpcConfirmInnerButton(AlertContent->Btn_2) : nullptr;
	GTeammateNpcAddConfirmYesWasPressed = false;
	GTeammateNpcAddConfirmNoWasPressed = false;
	GTeammateNpcAddConfirmPending = true;
	GTeammateNpcAddConfirmPendingNpcId = NpcId;

	if (!GTeammateNpcAddConfirmYesBtn || !GTeammateNpcAddConfirmNoBtn)
	{
		LOGE_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ConfirmDialog warn: yes/no button unresolved\n";
	}

	return true;
}

void PollTeammateNpcAddConfirmDialog()
{
	if (!GTeammateNpcAddConfirmPending)
		return;

	if (!GTeammateNpcAddConfirmAlert ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcAddConfirmAlert), UJHNeoUIAlertContentSimple::StaticClass()) ||
		!GTeammateNpcAddConfirmYesBtn ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcAddConfirmYesBtn), UButton::StaticClass()) ||
		!GTeammateNpcAddConfirmNoBtn ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcAddConfirmNoBtn), UButton::StaticClass()))
	{
		return;
	}

	const bool bYesPressed = GTeammateNpcAddConfirmYesBtn->IsPressed();
	const bool bNoPressed = GTeammateNpcAddConfirmNoBtn->IsPressed();

	if (!GTeammateNpcAddConfirmYesWasPressed && bYesPressed)
	{
		const int32 TargetNpcId = GTeammateNpcAddConfirmPendingNpcId;
		AddTeammateNpcToTeamById(TargetNpcId);
		return;
	}

	if (!GTeammateNpcAddConfirmNoWasPressed && bNoPressed)
	{
		ResetTeammateNpcAddConfirmState();
		return;
	}

	GTeammateNpcAddConfirmYesWasPressed = bYesPressed;
	GTeammateNpcAddConfirmNoWasPressed = bNoPressed;
}

#endif

bool TryGetTeammateNpcResourceTable(UDataTable*& OutTable)
{
	OutTable = nullptr;

	UNPCResManager* ResMgr = UManagerFuncLib::GetNPCResManager();
	if (!ResMgr || !IsSafeLiveObject(static_cast<UObject*>(ResMgr)))
	{
		if (kTab6NpcPrototypeVerboseLog)
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] TryGetResourceTable fail: ResMgr invalid\n";
		return false;
	}

	UDataTable* Table = ResMgr->NPCResourceTable;
	if (!Table || !IsSafeLiveObject(static_cast<UObject*>(Table)))
	{
		if (kTab6NpcPrototypeVerboseLog)
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] TryGetResourceTable fail: table invalid\n";
		return false;
	}

	OutTable = Table;
	if (kTab6NpcPrototypeVerboseLog)
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] TryGetResourceTable ok table=" << (void*)Table << "\n";
	LogTeammateNpcResourceTableSchema(Table);
	return true;
}

bool TryGetTeammateNpcTable(UDataTable*& OutTable)
{
	OutTable = nullptr;

	UNPCResManager* ResMgr = UManagerFuncLib::GetNPCResManager();
	if (!ResMgr || !IsSafeLiveObject(static_cast<UObject*>(ResMgr)))
	{
		if (kTab6NpcPrototypeVerboseLog)
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] TryGetNpcTable fail: ResMgr invalid\n";
		return false;
	}

	UDataTable* Table = ResMgr->NPCTable;
	if (!Table || !IsSafeLiveObject(static_cast<UObject*>(Table)))
	{
		if (kTab6NpcPrototypeVerboseLog)
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] TryGetNpcTable fail: table invalid\n";
		return false;
	}

	OutTable = Table;
	if (kTab6NpcPrototypeVerboseLog)
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] TryGetNpcTable ok table=" << (void*)Table << "\n";
	return true;
}

bool TryGetTeammateNpcPictorialTable(UDataTable*& OutTable)
{
	OutTable = nullptr;

	UPictorialResManager* ResMgr = UManagerFuncLib::GetPictorialResManager();
	if (!ResMgr || !IsSafeLiveObject(static_cast<UObject*>(ResMgr)))
		return false;

	UDataTable* Table = ResMgr->PictorialResourceTable;
	if (!Table || !IsSafeLiveObject(static_cast<UObject*>(Table)))
		return false;

	OutTable = Table;
	return true;
}

template<typename RowType>
bool TryGetTeammateNpcTableRowBySlot(
	UDataTable* Table,
	int32 SlotIndex,
	FName* OutRowName,
	const RowType** OutRow)
{
	if (!Table || !IsSafeLiveObject(static_cast<UObject*>(Table)) || !OutRow)
		return false;

	auto& RowMap = Table->RowMap;
	if (!RowMap.IsValid() || !RowMap.IsValidIndex(SlotIndex))
		return false;

	auto& Pair = RowMap[SlotIndex];
	uint8* RowData = Pair.Value();
	if (!RowData)
		return false;

	if (OutRowName)
		*OutRowName = Pair.Key();
	*OutRow = reinterpret_cast<const RowType*>(RowData);
	return true;
}

bool TryGetTeammateNpcResourceRowByRowName(UDataTable* Table, const FName& RowName, FNPCResourceSetting* OutRow)
{
	if (!OutRow || !Table || !IsSafeLiveObject(static_cast<UObject*>(Table)) || RowName.IsNone())
		return false;

	auto& RowMap = Table->RowMap;
	if (!RowMap.IsValid())
		return false;

	const int32 AllocatedSlots = RowMap.NumAllocated();
	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		FName CurrentRowName{};
		const FNPCResourceSetting* RowPtr = nullptr;
		if (!TryGetTeammateNpcTableRowBySlot<FNPCResourceSetting>(Table, i, &CurrentRowName, &RowPtr))
			continue;
		if (!(CurrentRowName == RowName))
			continue;

		*OutRow = *RowPtr;
		return true;
	}

	return false;
}

std::string NormalizeTeammateNpcResourceKey(std::string Key)
{
	for (char& Ch : Key)
	{
		if (Ch >= 'A' && Ch <= 'Z')
			Ch = static_cast<char>(Ch - 'A' + 'a');
	}

	if (!Key.empty())
	{
		const size_t UnderlinePos = Key.find_last_of('_');
		if (UnderlinePos != std::string::npos && (UnderlinePos + 1) < Key.size())
		{
			bool bSuffixAllDigits = true;
			for (size_t i = UnderlinePos + 1; i < Key.size(); ++i)
			{
				if (Key[i] < '0' || Key[i] > '9')
				{
					bSuffixAllDigits = false;
					break;
				}
			}
			if (bSuffixAllDigits)
				Key.resize(UnderlinePos);
		}
	}

	return Key;
}

void AppendTeammateNpcResourceToken(std::vector<std::string>& Tokens, const std::string& Token)
{
	if (Token.empty())
		return;

	for (const std::string& Existing : Tokens)
	{
		if (Existing == Token)
			return;
	}
	Tokens.push_back(Token);
}

void BuildTeammateNpcResourceTokens(const std::string& RawKey, std::vector<std::string>& OutTokens)
{
	OutTokens.clear();
	if (RawKey.empty())
		return;

	const std::string LowerRaw = NormalizeTeammateNpcResourceKey(RawKey);
	AppendTeammateNpcResourceToken(OutTokens, LowerRaw);

	const size_t DelimPos = RawKey.find_first_of("_-.");
	if (DelimPos != std::string::npos)
	{
		const std::string Prefix = RawKey.substr(0, DelimPos);
		AppendTeammateNpcResourceToken(OutTokens, NormalizeTeammateNpcResourceKey(Prefix));
	}

	std::string Segment;
	Segment.reserve(RawKey.size());
	for (size_t i = 0; i < RawKey.size(); ++i)
	{
		const char Ch = RawKey[i];
		const bool bSep = (Ch == '_' || Ch == '-' || Ch == '.');
		if (!bSep)
		{
			Segment.push_back(Ch);
			continue;
		}

		if (!Segment.empty())
		{
			AppendTeammateNpcResourceToken(OutTokens, NormalizeTeammateNpcResourceKey(Segment));
			Segment.clear();
		}
	}
	if (!Segment.empty())
		AppendTeammateNpcResourceToken(OutTokens, NormalizeTeammateNpcResourceKey(Segment));
}

int32 ParseTeammateNpcResourceKeyTrailingId(const std::string& RawKey)
{
	if (RawKey.empty())
		return -1;

	size_t StartPos = RawKey.find_last_of('_');
	if (StartPos == std::string::npos)
		StartPos = RawKey.find_last_of('-');
	if (StartPos == std::string::npos)
		StartPos = 0;
	else
		StartPos += 1;

	if (StartPos >= RawKey.size())
		return -1;

	int32 Value = 0;
	bool bHasDigit = false;
	for (size_t i = StartPos; i < RawKey.size(); ++i)
	{
		const char Ch = RawKey[i];
		if (Ch < '0' || Ch > '9')
			return -1;
		bHasDigit = true;
		const int32 Digit = static_cast<int32>(Ch - '0');
		if (Value > 214748364)
			return -1;
		if (Value == 214748364 && Digit > 7)
			return -1;
		Value = Value * 10 + Digit;
	}

	if (!bHasDigit || Value <= 0)
		return -1;
	return Value;
}

std::string GetTeammateNpcFlipbookSoftPathKey(const TSoftObjectPtr<UPaperFlipbook>& SoftFlipbook)
{
	const std::string RawAssetPath = SoftFlipbook.ObjectID.AssetPathName.ToString();
	if (RawAssetPath.empty())
		return {};
	return NormalizeTeammateNpcResourceKey(RawAssetPath);
}

int32 ScoreTeammateNpcResourceRowByTokens(
	const FNPCResourceSetting& Row,
	const std::vector<std::string>& Tokens)
{
	if (Tokens.empty())
		return 0;

	std::vector<std::string> CandidateKeys;
	CandidateKeys.reserve(4);
	CandidateKeys.push_back(GetTeammateNpcFlipbookSoftPathKey(Row.IdleRightFlipbook));
	CandidateKeys.push_back(GetTeammateNpcFlipbookSoftPathKey(Row.IdleLeftFlipbook));
	CandidateKeys.push_back(GetTeammateNpcFlipbookSoftPathKey(Row.IdleUpFlipbook));
	CandidateKeys.push_back(GetTeammateNpcFlipbookSoftPathKey(Row.IdleDownFlipbook));

	int32 Score = 0;
	for (const std::string& Token : Tokens)
	{
		if (Token.empty() || Token.size() <= 1)
			continue;

		for (const std::string& Candidate : CandidateKeys)
		{
			if (Candidate.empty())
				continue;

			if (Candidate.find(Token) == std::string::npos)
				continue;

			Score += static_cast<int32>(Token.size());
			break;
		}
	}
	return Score;
}

void LogTeammateNpcResourceTableSchema(UDataTable* Table)
{
	if (!kTab6NpcPrototypeVerboseLog || GTeammateNpcResourceTableSchemaLogged)
		return;
	GTeammateNpcResourceTableSchemaLogged = true;

	if (!Table || !IsSafeLiveObject(static_cast<UObject*>(Table)))
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResourceTable schema dump skipped: table invalid\n";
		return;
	}

	auto& RowMap = Table->RowMap;
	const int32 RowCount = RowMap.IsValid() ? RowMap.Num() : 0;
	LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResourceTable schema rows=" << RowCount << "\n";
	if (!RowMap.IsValid() || RowMap.Num() <= 0)
		return;

	const int32 AllocatedSlots = RowMap.NumAllocated();
	int32 Dumped = 0;
	for (int32 i = 0; i < AllocatedSlots && Dumped < 12; ++i)
	{
		FName RowName{};
		const FNPCResourceSetting* Row = nullptr;
		if (!TryGetTeammateNpcTableRowBySlot<FNPCResourceSetting>(Table, i, &RowName, &Row))
			continue;

		const std::string RowNameString = RowName.ToString();

		std::string IdleKey = GetTeammateNpcFlipbookSoftPathKey(Row->IdleRightFlipbook);
		if (IdleKey.empty())
			IdleKey = GetTeammateNpcFlipbookSoftPathKey(Row->IdleLeftFlipbook);
		if (IdleKey.empty())
			IdleKey = GetTeammateNpcFlipbookSoftPathKey(Row->IdleDownFlipbook);
		if (IdleKey.empty())
			IdleKey = GetTeammateNpcFlipbookSoftPathKey(Row->IdleUpFlipbook);

		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResourceTable row[" << Dumped
		          << "]=" << RowNameString.c_str()
		          << " id=" << Row->ID
		          << " idleKey=" << (IdleKey.empty() ? "<empty>" : IdleKey.c_str()) << "\n";
		++Dumped;
	}
}

bool TryGetTeammateNpcResourceRowById(UDataTable* Table, int32 ResourceId, FNPCResourceSetting* OutRow)
{
	if (!OutRow || !Table || !IsSafeLiveObject(static_cast<UObject*>(Table)) || ResourceId <= 0)
		return false;

	auto& RowMap = Table->RowMap;
	if (!RowMap.IsValid() || RowMap.Num() <= 0)
		return false;

	const int32 AllocatedSlots = RowMap.NumAllocated();
	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		const FNPCResourceSetting* Row = nullptr;
		if (!TryGetTeammateNpcTableRowBySlot<FNPCResourceSetting>(Table, i, nullptr, &Row))
			continue;

		if (Row->ID != ResourceId)
			continue;

		*OutRow = *Row;
		return true;
	}

	return false;
}

bool TryGetTeammateNpcResourceRowByLooseKey(
	UDataTable* Table,
	const FName& ResourceKey,
	FNPCResourceSetting* OutRow,
	std::string* OutMatchedRowName = nullptr)
{
	if (!OutRow || !Table || !IsSafeLiveObject(static_cast<UObject*>(Table)) || ResourceKey.IsNone())
		return false;

	if (TryGetTeammateNpcResourceRowByRowName(Table, ResourceKey, OutRow))
	{
		if (OutMatchedRowName)
			*OutMatchedRowName = ResourceKey.ToString();
		return true;
	}

	const std::string RawKey = ResourceKey.ToString();
	const std::string NormalizedKey = NormalizeTeammateNpcResourceKey(RawKey);
	if (RawKey.empty())
		return false;

	auto& RowMap = Table->RowMap;
	if (!RowMap.IsValid() || RowMap.Num() <= 0)
		return false;

	const int32 AllocatedSlots = RowMap.NumAllocated();
	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		FName CurrentRowName{};
		const FNPCResourceSetting* RowPtr = nullptr;
		if (!TryGetTeammateNpcTableRowBySlot<FNPCResourceSetting>(Table, i, &CurrentRowName, &RowPtr))
			continue;

		const std::string RowNameString = CurrentRowName.ToString();
		if (RowNameString.empty())
			continue;

		const std::string NormalizedRowName = NormalizeTeammateNpcResourceKey(RowNameString);
		const bool bMatched =
			(NormalizedRowName == NormalizedKey) ||
			(NormalizedRowName.find(NormalizedKey) != std::string::npos) ||
			(NormalizedKey.find(NormalizedRowName) != std::string::npos);
		if (!bMatched)
			continue;

		*OutRow = *RowPtr;
		if (OutMatchedRowName)
			*OutMatchedRowName = RowNameString;
		return true;
	}

	const int32 TrailingId = ParseTeammateNpcResourceKeyTrailingId(RawKey);
	if (TrailingId > 0 && TryGetTeammateNpcResourceRowById(Table, TrailingId, OutRow))
	{
		if (OutMatchedRowName)
			*OutMatchedRowName = ("#id=" + std::to_string(TrailingId));
		return true;
	}

	std::vector<std::string> Tokens;
	BuildTeammateNpcResourceTokens(RawKey, Tokens);
	int32 BestScore = 0;
	FNPCResourceSetting BestRow{};
	std::string BestRowName;
	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		FName CurrentRowName{};
		const FNPCResourceSetting* RowPtr = nullptr;
		if (!TryGetTeammateNpcTableRowBySlot<FNPCResourceSetting>(Table, i, &CurrentRowName, &RowPtr))
			continue;

		const int32 Score = ScoreTeammateNpcResourceRowByTokens(*RowPtr, Tokens);
		if (Score <= BestScore)
			continue;

		BestScore = Score;
		BestRow = *RowPtr;
		BestRowName = CurrentRowName.ToString();
	}

	if (BestScore > 0)
	{
		*OutRow = BestRow;
		if (OutMatchedRowName)
			*OutMatchedRowName = (BestRowName + "#path");
		return true;
	}

	return false;
}

bool TryGetTeammateNpcPictorialRow(int32 NpcId, FGHCNPC_RawData* OutRow)
{
	if (!OutRow || NpcId <= 0)
		return false;

	UDataTable* Table = nullptr;
	if (!TryGetTeammateNpcPictorialTable(Table))
		return false;

	auto& RowMap = Table->RowMap;
	if (!RowMap.IsValid() || RowMap.Num() <= 0)
		return false;

	const int32 AllocatedSlots = RowMap.NumAllocated();
	for (int32 i = 0; i < AllocatedSlots; ++i)
	{
		const FGHCNPC_RawData* Row = nullptr;
		if (!TryGetTeammateNpcTableRowBySlot<FGHCNPC_RawData>(Table, i, nullptr, &Row))
			continue;

		if (Row->ID != NpcId)
			continue;

		*OutRow = *Row;
		return true;
	}

	return false;
}

UPaperFlipbook* ResolveTeammateNpcPrototypeFlipbook(const TSoftObjectPtr<UPaperFlipbook>& SoftFlipbook)
{
	UPaperFlipbook* Flipbook = SoftFlipbook.Get();
	if (!Flipbook)
	{
		static_assert(
			sizeof(TSoftObjectPtr<UObject>) == sizeof(TSoftObjectPtr<UPaperFlipbook>),
			"TSoftObjectPtr size mismatch");
		TSoftObjectPtr<UObject> SoftObject{};
		std::memcpy(&SoftObject, &SoftFlipbook, sizeof(SoftObject));
		UObject* Loaded = UKismetSystemLibrary::LoadAsset_Blocking(SoftObject);
		if (Loaded && Loaded->IsA(UPaperFlipbook::StaticClass()))
			Flipbook = static_cast<UPaperFlipbook*>(Loaded);
	}

	if (!IsSafeLiveObjectOfClass(static_cast<UObject*>(Flipbook), UPaperFlipbook::StaticClass()))
		return nullptr;

	return Flipbook;
}

UPaperFlipbook* ResolveTeammateNpcPrototypeIdleFlipbookFromResourceRow(const FNPCResourceSetting& ResourceRow)
{
	UPaperFlipbook* Flipbook = ResolveTeammateNpcPrototypeFlipbook(ResourceRow.IdleRightFlipbook);
	if (!Flipbook)
		Flipbook = ResolveTeammateNpcPrototypeFlipbook(ResourceRow.IdleDownFlipbook);
	if (!Flipbook)
		Flipbook = ResolveTeammateNpcPrototypeFlipbook(ResourceRow.IdleLeftFlipbook);
	if (!Flipbook)
		Flipbook = ResolveTeammateNpcPrototypeFlipbook(ResourceRow.IdleUpFlipbook);
	return Flipbook;
}

UPaperFlipbook* ResolveTeammateNpcPrototypeIdleFlipbookByResourceName(const FName& ResourceName)
{
	if (ResourceName.IsNone())
		return nullptr;

	UDataTable* ResourceTable = nullptr;
	if (!TryGetTeammateNpcResourceTable(ResourceTable))
		return nullptr;

	FNPCResourceSetting ResourceRow{};
	if (!TryGetTeammateNpcResourceRowByLooseKey(ResourceTable, ResourceName, &ResourceRow))
		return nullptr;

	return ResolveTeammateNpcPrototypeIdleFlipbookFromResourceRow(ResourceRow);
}

UPaperFlipbook* ResolveTeammateNpcPrototypeIdleFlipbook(int32 NpcId)
{
	UDataTable* ResourceTable = nullptr;
	if (!TryGetTeammateNpcResourceTable(ResourceTable))
		return nullptr;

	FNPCResourceSetting ResourceRow{};
	bool Found = false;
	const char* FoundSource = "None";
	std::string MatchedRowName;

	auto TryResolveByKey = [&](const FName& Key, const char* SourceTag) -> bool
	{
		if (Key.IsNone())
		{
			if (kTab6NpcPrototypeVerboseLog)
				LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResolveFlipbook npcId=" << NpcId
				          << " source=" << SourceTag << " resourceName=None\n";
			return false;
		}

		std::string LocalMatchedRowName;
		if (TryGetTeammateNpcResourceRowByLooseKey(ResourceTable, Key, &ResourceRow, &LocalMatchedRowName))
		{
			MatchedRowName = std::move(LocalMatchedRowName);
			FoundSource = SourceTag;
			if (kTab6NpcPrototypeVerboseLog)
			{
				LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResolveFlipbook npcId=" << NpcId
				          << " source=" << SourceTag
				          << " resourceName=" << Key.ToString().c_str()
				          << " matchedRow=" << MatchedRowName.c_str() << "\n";
			}
			return true;
		}

		if (kTab6NpcPrototypeVerboseLog)
		{
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResolveFlipbook npcId=" << NpcId
			          << " source=" << SourceTag
			          << " resourceName=" << Key.ToString().c_str()
			          << " rowMissing\n";
		}
		return false;
	};

	UTeamInfo* TeamInfo = UNPCFuncLib::GetNPCInfoById(NpcId);
	if (TeamInfo && IsSafeLiveObject(static_cast<UObject*>(TeamInfo)))
		Found = TryResolveByKey(TeamInfo->AvatarResourceName, "TeamInfoAvatar");

	if (!Found)
	{
		UNPCManager* NpcMgr = UManagerFuncLib::GetNPCManager();
		if (NpcMgr && IsSafeLiveObject(static_cast<UObject*>(NpcMgr)))
		{
			Found = TryResolveByKey(NpcMgr->GetAvatarResource(NpcId), "NPCManagerAvatar");
			if (!Found)
			{
				TArray<FName> AvatarNames = NpcMgr->GetAvailableAvatarNames(NpcId);
				if (kTab6NpcPrototypeVerboseLog)
				{
					LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResolveFlipbook npcId=" << NpcId
					          << " source=AvailableAvatarNames count=" << AvatarNames.Num() << "\n";
				}
				for (int32 i = 0; i < AvatarNames.Num() && !Found; ++i)
					Found = TryResolveByKey(AvatarNames[i], "AvailableAvatar");
			}
		}
	}

	if (!Found)
	{
		UNPCResManager* NpcResMgr = UManagerFuncLib::GetNPCResManager();
		if (NpcResMgr && IsSafeLiveObject(static_cast<UObject*>(NpcResMgr)))
		{
			const FNPCSetting NpcSetting = NpcResMgr->BP_GetNPCSetting(NpcId);
			Found = TryResolveByKey(NpcSetting.ResourceName, "NPCSetting");
		}
	}

	if (!Found)
	{
		FGHCNPC_RawData PictorialRow{};
		if (TryGetTeammateNpcPictorialRow(NpcId, &PictorialRow) && PictorialRow.CommonId > 0)
		{
			Found = TryGetTeammateNpcResourceRowById(ResourceTable, PictorialRow.CommonId, &ResourceRow);
			if (Found)
				FoundSource = "PictorialCommonId";
		}
	}

	if (!Found && NpcId > 0)
	{
		Found = TryGetTeammateNpcResourceRowById(ResourceTable, NpcId, &ResourceRow);
		if (Found)
		{
			FoundSource = "NpcIdAsResourceId";
			MatchedRowName = ("#id=" + std::to_string(NpcId));
		}
	}

	if (!Found && NpcId == 0)
	{
		Found = TryGetTeammateNpcResourceRowById(ResourceTable, 1, &ResourceRow);
		if (Found)
		{
			FoundSource = "MainRoleFallbackId1";
			MatchedRowName = "#id=1";
		}
	}

	if (!Found)
	{
		if (kTab6NpcPrototypeVerboseLog)
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResolveFlipbook fail npcId=" << NpcId << "\n";
		return nullptr;
	}

	UPaperFlipbook* Flipbook = ResolveTeammateNpcPrototypeIdleFlipbookFromResourceRow(ResourceRow);
	if (kTab6NpcPrototypeVerboseLog)
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] ResolveFlipbook npcId=" << NpcId
		          << " source=" << FoundSource
		          << " matchedRow=" << (MatchedRowName.empty() ? "<none>" : MatchedRowName.c_str())
		          << " resourceId=" << ResourceRow.ID
		          << " ok=" << (Flipbook ? 1 : 0) << "\n";
	}
	return Flipbook;
}

std::wstring ResolveTeammateNpcPrototypeDisplayName(const FGHCNPC_RawData* PictorialRow, int32 NpcId)
{
	if (PictorialRow)
	{
		std::wstring Name = GetTeammateNpcPrototypeText(PictorialRow->PictorialName);
		if (IsTeammateNpcPrototypeUsableName(Name))
			return Name;
	}

	std::wstring Name = GetTeammateNpcPrototypeName(NpcId, nullptr);
	if (IsTeammateNpcPrototypeUsableName(Name))
		return Name;

	if (PictorialRow)
	{
		Name = GetTeammateNpcPrototypeText(PictorialRow->PictorialName);
		if (!Name.empty())
			return Name;
	}

	if (!Name.empty())
		return Name;

	return L"未命名NPC";
}

std::vector<FTeammateNpcPrototypeSample> CollectTeammateNpcPrototypeSamples()
{
	std::vector<FTeammateNpcPrototypeSample> Samples;
	UDataTable* NpcTable = nullptr;
	if (TryGetTeammateNpcTable(NpcTable))
	{
		auto& RowMap = NpcTable->RowMap;
		const int32 RowCount = RowMap.IsValid() ? RowMap.Num() : 0;
		if (kTab6NpcPrototypeVerboseLog)
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] NPCTable rows=" << RowCount << "\n";
		if (RowMap.IsValid() && RowMap.Num() > 0)
		{
			Samples.reserve(RowMap.Num());
			const int32 AllocatedSlots = RowMap.NumAllocated();
			for (int32 i = 0; i < AllocatedSlots; ++i)
			{
				const FNPCSetting* NpcRow = nullptr;
				if (!TryGetTeammateNpcTableRowBySlot<FNPCSetting>(NpcTable, i, nullptr, &NpcRow))
					continue;

				FTeammateNpcPrototypeSample Sample{};
				Sample.NpcId = NpcRow->ID;

				std::wstring DisplayName = GetTeammateNpcPrototypeText(NpcRow->Name);
				if (!IsTeammateNpcPrototypeUsableName(DisplayName))
					DisplayName = GetTeammateNpcPrototypeName(NpcRow->ID, nullptr);
				if (DisplayName.empty())
					DisplayName = std::wstring(L"NPC#") + std::to_wstring(NpcRow->ID);

				Sample.DisplayName = std::move(DisplayName);
				Samples.push_back(std::move(Sample));
			}
		}
	}
	else if (kTab6NpcPrototypeVerboseLog)
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] NPCTable scan skipped\n";
	}

	if (kTab6NpcPrototypeVerboseLog)
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] CollectSamples done count=" << Samples.size() << "\n";

	return Samples;
}

bool CanBuildTeammateNpcPrototypeCards(APlayerController* PC)
{
	const bool bPcOk = PC && IsSafeLiveObject(static_cast<UObject*>(PC));
	UDataTable* NpcTable = nullptr;
	UDataTable* ResourceTable = nullptr;
	const bool bNpcTableOk = TryGetTeammateNpcTable(NpcTable);
	const bool bResourceTableOk = TryGetTeammateNpcResourceTable(ResourceTable);
	if (kTab6NpcPrototypeVerboseLog)
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] CanBuild pc=" << (bPcOk ? 1 : 0)
		          << " npcTable=" << (bNpcTableOk ? 1 : 0)
		          << " resourceTable=" << (bResourceTableOk ? 1 : 0)
		          << " pcPtr=" << (void*)PC << "\n";
	}

	if (!bPcOk)
		return false;
	return bNpcTableOk && bResourceTableOk;
}

void RefreshTeammateNpcPrototypeCardContent_Impl(FTeammateNpcPrototypeCardBinding& Binding)
{
	if (!Binding.CardWidget ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(Binding.CardWidget), UJHNeoUIGHC_RegionElement_NPC::StaticClass()))
	{
		return;
	}

	if (Binding.IdleFlipbook &&
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(Binding.IdleFlipbook), UPaperFlipbook::StaticClass()))
	{
		Binding.IdleFlipbook = nullptr;
		Binding.bVisualsPrimed = false;
	}

	auto* MainImage = Binding.CardWidget->IMG_Main;
	if (MainImage && !IsSafeLiveObject(static_cast<UObject*>(MainImage)))
	{
		MainImage = nullptr;
		Binding.bVisualsPrimed = false;
	}

	Binding.CardWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	Binding.CardWidget->SetRenderTransformPivot(FVector2D{ 0.5f, 0.5f });
	Binding.CardWidget->SetRenderTranslation(FVector2D{ 0.0f, 0.0f });

	if (Binding.CardWidget->IsA(UJHGHC_RegionEle_NPC_C::StaticClass()))
	{
		auto* RegionNpcCard = static_cast<UJHGHC_RegionEle_NPC_C*>(Binding.CardWidget);
		if (RegionNpcCard->JHGPCBtn_ActiveBG &&
			IsSafeLiveObject(static_cast<UObject*>(RegionNpcCard->JHGPCBtn_ActiveBG)))
		{
			RegionNpcCard->JHGPCBtn_ActiveBG->SetRenderOpacity(0.0f);
			RegionNpcCard->JHGPCBtn_ActiveBG->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RegionNpcCard->NeoUIImageBase_36 &&
			IsSafeLiveObject(static_cast<UObject*>(RegionNpcCard->NeoUIImageBase_36)))
		{
			RegionNpcCard->NeoUIImageBase_36->SetRenderOpacity(0.0f);
			RegionNpcCard->NeoUIImageBase_36->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RegionNpcCard->NeoUIImageBase_44 &&
			IsSafeLiveObject(static_cast<UObject*>(RegionNpcCard->NeoUIImageBase_44)))
		{
			RegionNpcCard->NeoUIImageBase_44->SetRenderOpacity(0.0f);
			RegionNpcCard->NeoUIImageBase_44->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Binding.CardWidget->TXT_Title &&
		IsSafeLiveObject(static_cast<UObject*>(Binding.CardWidget->TXT_Title)))
	{
		const wchar_t* Title = Binding.DisplayName.empty() ? L"未命名NPC" : Binding.DisplayName.c_str();
		Binding.CardWidget->TXT_Title->SetText(MakeText(Title));
		EnsureTeammateNpcPrototypeCardTitleStyle(Binding);
	}

	if (Binding.CardWidget->IMG_Locked &&
		IsSafeLiveObject(static_cast<UObject*>(Binding.CardWidget->IMG_Locked)))
	{
		Binding.CardWidget->IMG_Locked->SetRenderOpacity(0.0f);
		Binding.CardWidget->IMG_Locked->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Binding.CardWidget->IMG_DLCMark &&
		IsSafeLiveObject(static_cast<UObject*>(Binding.CardWidget->IMG_DLCMark)))
	{
		Binding.CardWidget->IMG_DLCMark->SetVisibility(
			Binding.bShowDlcMark ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (MainImage && Binding.IdleFlipbook && !Binding.bVisualsPrimed)
	{
		MainImage->SetFlipbook(Binding.IdleFlipbook);
		MainImage->ResetAnimation();
		MainImage->PlayAnimation();
		MainImage->SetRenderOpacity(1.0f);
		Binding.bVisualsPrimed = true;
	}
	else if (MainImage && !Binding.IdleFlipbook)
	{
		MainImage->SetRenderOpacity(0.0f);
	}

	if (MainImage)
	{
		MainImage->SetRenderTransformPivot(FVector2D{ 0.5f, 1.0f });
		MainImage->SetRenderScale(FVector2D{ kTeammateNpcMainImageScale, kTeammateNpcMainImageScale });
		MainImage->SetRenderTranslation(FVector2D{ kTeammateNpcMainImageOffsetX, kTeammateNpcMainImageLiftY });
		if (MainImage->Slot &&
			IsSafeLiveObject(static_cast<UObject*>(MainImage->Slot)) &&
			MainImage->Slot->IsA(UCanvasPanelSlot::StaticClass()))
		{
			auto* MainSlot = static_cast<UCanvasPanelSlot*>(MainImage->Slot);
			FAnchorData Layout = MainSlot->GetLayout();
			Layout.Anchors.Minimum = FVector2D{ 0.5f, 1.0f };
			Layout.Anchors.Maximum = FVector2D{ 0.5f, 1.0f };
			Layout.Alignment = FVector2D{ 0.5f, 1.0f };
			Layout.Offsets.Left = 0.0f;
			Layout.Offsets.Top = 0.0f;
			MainSlot->SetLayout(Layout);
			MainSlot->SetAutoSize(true);
		}
	}
}

void RefreshTeammateNpcPrototypeCardContent(FTeammateNpcPrototypeCardBinding& Binding)
{
	const int32 SafeNpcId = Binding.NpcId;
	__try
	{
		RefreshTeammateNpcPrototypeCardContent_Impl(Binding);
	}
	__except (HandleTab6NpcProtoSehException("RefreshCardContent", SafeNpcId, GetExceptionCode()))
	{
		Binding.bVisualsPrimed = false;
		Binding.WasPressed = false;
	}
}

UJHNeoUIGHC_RegionElement_NPC* BuildTeammateNpcPrototypeCard(APlayerController* PC)
{
	if (!PC)
		return nullptr;

	UClass* CardClass = UObject::FindClassFast("JHGHC_RegionEle_NPC_C");
	if (!CardClass)
		CardClass = UObject::FindClass("JHGHC_RegionEle_NPC_C");
	if (!CardClass)
		return nullptr;

	auto* Card = static_cast<UJHNeoUIGHC_RegionElement_NPC*>(
		UWidgetBlueprintLibrary::Create(PC, CardClass, PC));
	if (!Card || !IsSafeLiveObjectOfClass(static_cast<UObject*>(Card), UJHNeoUIGHC_RegionElement_NPC::StaticClass()))
		return nullptr;

	MarkAsGCRoot(static_cast<UObject*>(Card));
	RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(Card));

	Card->SetRenderOpacity(0.88f);
	Card->SetRenderScale(FVector2D{ kTeammateNpcCardScaleNormal, kTeammateNpcCardScaleNormal });

	return Card;
}

std::wstring FoldTeammateNpcSearchText(const std::wstring& Src)
{
	std::wstring Out;
	Out.reserve(Src.size());
	for (wchar_t Ch : Src)
	{
		if (Ch >= L'A' && Ch <= L'Z')
			Out.push_back(static_cast<wchar_t>(Ch - L'A' + L'a'));
		else
			Out.push_back(Ch);
	}
	return Out;
}

bool UpdateTeammateNpcSearchKeywordFromEdit()
{
	std::wstring NewKeyword;
	if (GTeammateNpcPrototypeSearchEdit &&
		IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcPrototypeSearchEdit), UEditableTextBox::StaticClass()))
	{
		const FText Text = GTeammateNpcPrototypeSearchEdit->GetText();
		const FString Raw = UKismetTextLibrary::Conv_TextToString(Text);
		const wchar_t* WS = Raw.CStr();
		if (WS)
			NewKeyword = WS;
	}

	if (NewKeyword == GTeammateNpcPrototypeSearchKeyword)
		return false;

	GTeammateNpcPrototypeSearchKeyword = std::move(NewKeyword);
	return true;
}

void ClearTeammateNpcPrototypeCards()
{
	for (auto& Binding : GTeammateNpcPrototypeCards)
	{
		if (Binding.FrameBorder && IsSafeLiveObject(static_cast<UObject*>(Binding.FrameBorder)))
			Binding.FrameBorder->RemoveFromParent();
		if (Binding.ClickButton && IsSafeLiveObject(static_cast<UObject*>(Binding.ClickButton)))
			Binding.ClickButton->RemoveFromParent();
		if (Binding.CardWidget && IsSafeLiveObject(static_cast<UObject*>(Binding.CardWidget)))
		{
			Binding.CardWidget->RemoveFromParent();
			ClearGCRoot(static_cast<UObject*>(Binding.CardWidget));
		}
	}
	GTeammateNpcPrototypeCards.clear();
}

void UpdateTeammateNpcPrototypePageLabel()
{
	if (!GTeammateNpcPrototypePageLabel ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcPrototypePageLabel), UTextBlock::StaticClass()))
	{
		return;
	}

	wchar_t Buf[64] = {};
	const int32 PageShow = (GTeammateNpcPrototypeTotalPages > 0) ? (GTeammateNpcPrototypeCurrentPage + 1) : 1;
	const int32 PageTotal = (GTeammateNpcPrototypeTotalPages > 0) ? GTeammateNpcPrototypeTotalPages : 1;
	swprintf_s(Buf, 64, L"%d/%d", PageShow, PageTotal);
	GTeammateNpcPrototypePageLabel->SetText(MakeText(Buf));
}

void UpdateTeammateNpcPrototypePagerButtons()
{
	auto SetBtnEnabled = [](UJHCommon_Btn_Free_C* W, bool bEnabled)
	{
		if (!W)
			return;
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

	const bool bHasPages = (GTeammateNpcPrototypeTotalPages > 0);
	const bool bCanPrev = bHasPages && (GTeammateNpcPrototypeCurrentPage > 0);
	const bool bCanNext = bHasPages && ((GTeammateNpcPrototypeCurrentPage + 1) < GTeammateNpcPrototypeTotalPages);
	SetBtnEnabled(GTeammateNpcPrototypePrevPageBtn, bCanPrev);
	SetBtnEnabled(GTeammateNpcPrototypeNextPageBtn, bCanNext);
}

void RebuildTeammateNpcPrototypeFilteredIndices()
{
	GTeammateNpcPrototypeFilteredIndices.clear();
	const std::wstring KeywordFold = FoldTeammateNpcSearchText(GTeammateNpcPrototypeSearchKeyword);
	const bool HasKeyword = !KeywordFold.empty();

	const int32 Total = static_cast<int32>(GTeammateNpcPrototypeAllSamples.size());
	GTeammateNpcPrototypeFilteredIndices.reserve(Total);

	for (int32 i = 0; i < Total; ++i)
	{
		const auto& Sample = GTeammateNpcPrototypeAllSamples[i];
		if (!HasKeyword)
		{
			GTeammateNpcPrototypeFilteredIndices.push_back(i);
			continue;
		}

		std::wstring Hay = FoldTeammateNpcSearchText(Sample.DisplayName);
		Hay += L" ";
		Hay += std::to_wstring(Sample.NpcId);
		if (Hay.find(KeywordFold) != std::wstring::npos)
			GTeammateNpcPrototypeFilteredIndices.push_back(i);
	}

	const int32 FilteredCount = static_cast<int32>(GTeammateNpcPrototypeFilteredIndices.size());
	GTeammateNpcPrototypeTotalPages = (FilteredCount > 0)
		? ((FilteredCount + kTeammateNpcPrototypePerPage - 1) / kTeammateNpcPrototypePerPage)
		: 0;
}

void RefreshTeammateNpcPrototypePage_Impl()
{
	if (!GTeammateNpcPrototypeCardWrap ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcPrototypeCardWrap), UWrapBox::StaticClass()) ||
		!GTeammateNpcPrototypeOwnerPC ||
		!IsSafeLiveObject(static_cast<UObject*>(GTeammateNpcPrototypeOwnerPC)) ||
		!GTeammateNpcPrototypeOwnerOuter ||
		!IsSafeLiveObject(GTeammateNpcPrototypeOwnerOuter))
	{
		return;
	}

	const int32 FilteredCount = static_cast<int32>(GTeammateNpcPrototypeFilteredIndices.size());
	if (GTeammateNpcPrototypeTotalPages <= 0)
	{
		GTeammateNpcPrototypeCurrentPage = 0;
	}
	else
	{
		if (GTeammateNpcPrototypeCurrentPage < 0)
			GTeammateNpcPrototypeCurrentPage = 0;
		if (GTeammateNpcPrototypeCurrentPage >= GTeammateNpcPrototypeTotalPages)
			GTeammateNpcPrototypeCurrentPage = GTeammateNpcPrototypeTotalPages - 1;
	}

	ClearTeammateNpcPrototypeCards();
	while (GTeammateNpcPrototypeCardWrap->GetChildrenCount() > 0)
	{
		UWidget* Child = GTeammateNpcPrototypeCardWrap->GetChildAt(0);
		if (!Child)
			break;
		Child->RemoveFromParent();
	}

	if (FilteredCount <= 0)
	{
		UpdateTeammateNpcPrototypePageLabel();
		UpdateTeammateNpcPrototypePagerButtons();
		UpdateTeammateNpcPrototypeSelectionSummary();
		return;
	}

	const int32 Start = GTeammateNpcPrototypeCurrentPage * kTeammateNpcPrototypePerPage;
	int32 End = Start + kTeammateNpcPrototypePerPage;
	if (End > FilteredCount)
		End = FilteredCount;

	for (int32 i = Start; i < End; ++i)
	{
		const int32 SampleIdx = GTeammateNpcPrototypeFilteredIndices[i];
		if (SampleIdx < 0 || SampleIdx >= static_cast<int32>(GTeammateNpcPrototypeAllSamples.size()))
			continue;

		auto& Sample = GTeammateNpcPrototypeAllSamples[SampleIdx];
		if (Sample.IdleFlipbook &&
			!IsSafeLiveObjectOfClass(static_cast<UObject*>(Sample.IdleFlipbook), UPaperFlipbook::StaticClass()))
		{
			Sample.IdleFlipbook = nullptr;
		}
		if (!Sample.IdleFlipbook)
			Sample.IdleFlipbook = ResolveTeammateNpcPrototypeIdleFlipbook(Sample.NpcId);

		auto* Card = BuildTeammateNpcPrototypeCard(GTeammateNpcPrototypeOwnerPC);
		if (!Card)
			continue;

		auto* ClickButton = static_cast<UButton*>(CreateRawWidget(UButton::StaticClass(), GTeammateNpcPrototypeOwnerOuter));
		auto* FrameBorder = static_cast<UBorder*>(CreateRawWidget(UBorder::StaticClass(), GTeammateNpcPrototypeOwnerOuter));
		auto* CardHost = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), GTeammateNpcPrototypeOwnerOuter));
		if (ClickButton && FrameBorder && CardHost)
		{
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(ClickButton));
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(FrameBorder));
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(CardHost));
			ClickButton->IsFocusable = false;
			ClickButton->SetBackgroundColor(FLinearColor{ 0.0f, 0.0f, 0.0f, 0.0f });
			ClickButton->SetColorAndOpacity(FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f });
			{
				FButtonStyle BtnStyle = ClickButton->WidgetStyle;
				BtnStyle.NormalPadding = FMargin{ 0.0f, 0.0f, 0.0f, 0.0f };
				BtnStyle.PressedPadding = FMargin{ 0.0f, 0.0f, 0.0f, 0.0f };
				ClickButton->SetStyle(BtnStyle);
			}
			FrameBorder->SetPadding(FMargin{ 0.0f, 0.0f, 0.0f, 0.0f });
			FrameBorder->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
			FrameBorder->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
			FrameBorder->SetBrushColor(GetTeammateNpcPrototypeFrameColor(false));
			FrameBorder->SetContentColorAndOpacity(FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f });
			CardHost->SetWidthOverride(kTeammateNpcCardHostWidth);
			CardHost->SetHeightOverride(kTeammateNpcCardHostHeight);
			if (UPanelSlot* CardSlot = CardHost->AddChild(Card))
			{
				if (CardSlot->IsA(USizeBoxSlot::StaticClass()))
				{
					auto* SizeSlot = static_cast<USizeBoxSlot*>(CardSlot);
					SizeSlot->SetPadding(FMargin{ 0.0f, 0.0f, 0.0f, 0.0f });
					SizeSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
					SizeSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
				}
			}
			if (UPanelSlot* BtnSlotRaw = ClickButton->AddChild(CardHost))
			{
				if (BtnSlotRaw->IsA(UButtonSlot::StaticClass()))
				{
					auto* BtnSlot = static_cast<UButtonSlot*>(BtnSlotRaw);
					BtnSlot->SetPadding(FMargin{ 0.0f, 0.0f, 0.0f, 0.0f });
					BtnSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
					BtnSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
				}
			}
			if (UPanelSlot* BorderSlotRaw = FrameBorder->AddChild(ClickButton))
			{
				if (BorderSlotRaw->IsA(UBorderSlot::StaticClass()))
				{
					auto* BorderSlot = static_cast<UBorderSlot*>(BorderSlotRaw);
					BorderSlot->SetPadding(FMargin{ 0.0f, 0.0f, 0.0f, 0.0f });
					BorderSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
					BorderSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
				}
			}

			if (UWrapBoxSlot* HostSlot = GTeammateNpcPrototypeCardWrap->AddChildToWrapBox(FrameBorder))
			{
				FMargin Pad{};
				Pad.Right = 0.0f;
				Pad.Bottom = 0.0f;
				HostSlot->SetPadding(Pad);
				HostSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
				HostSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
			}

			FTeammateNpcPrototypeCardBinding Binding{};
			Binding.NpcId = Sample.NpcId;
			Binding.DisplayName = Sample.DisplayName;
			Binding.bShowDlcMark = Sample.bShowDlcMark;
			Binding.IdleFlipbook = Sample.IdleFlipbook;
			Binding.ClickButton = ClickButton;
			Binding.FrameBorder = FrameBorder;
			Binding.CardHost = CardHost;
			Binding.CardWidget = Card;
			RefreshTeammateNpcPrototypeCardContent(Binding);
			GTeammateNpcPrototypeCards.push_back(Binding);
		}
		else
		{
			GTeammateNpcPrototypeCardWrap->AddChildToWrapBox(Card);
		}
	}

	UpdateTeammateNpcPrototypePageLabel();
	UpdateTeammateNpcPrototypePagerButtons();
	if (!HasTeammateNpcPrototypeCardBinding(GTeammateNpcPrototypeSelectedNpcId) &&
		!GTeammateNpcPrototypeCards.empty())
	{
		GTeammateNpcPrototypeSelectedNpcId = GTeammateNpcPrototypeCards.front().NpcId;
	}
	ApplyTeammateNpcPrototypeSelectionVisuals();
	GTeammateNpcPrototypeLastLayoutLogTick = 0;
	if (kTab6NpcPrototypeVerboseLog)
		LogTeammateNpcPrototypeLayout("refresh-immediate");
}

void RefreshTeammateNpcPrototypePage()
{
	__try
	{
		RefreshTeammateNpcPrototypePage_Impl();
	}
	__except (HandleTab6NpcProtoSehException("RefreshPage", GTeammateNpcPrototypeSelectedNpcId, GetExceptionCode()))
	{
		ClearTeammateNpcPrototypeCards();
		GTeammateNpcPrototypeLastSelectionPollTick = 0;
		GTeammateNpcPrototypeLastPagerPollTick = 0;
	}
}

const char* GetTeammateNpcPrototypeSlotTypeName(UPanelSlot* Slot)
{
	if (!Slot)
		return "null";
	if (Slot->IsA(UWrapBoxSlot::StaticClass()))
		return "WrapBoxSlot";
	if (Slot->IsA(UCanvasPanelSlot::StaticClass()))
		return "CanvasPanelSlot";
	if (Slot->IsA(UButtonSlot::StaticClass()))
		return "ButtonSlot";
	if (Slot->IsA(UBorderSlot::StaticClass()))
		return "BorderSlot";
	if (Slot->IsA(UOverlaySlot::StaticClass()))
		return "OverlaySlot";
	if (Slot->IsA(USizeBoxSlot::StaticClass()))
		return "SizeBoxSlot";
	if (Slot->IsA(UVerticalBoxSlot::StaticClass()))
		return "VerticalBoxSlot";
	if (Slot->IsA(UHorizontalBoxSlot::StaticClass()))
		return "HorizontalBoxSlot";
	return "PanelSlot";
}

void LogTeammateNpcPrototypePanelSlotInfo(const char* Prefix, UPanelSlot* Slot)
{
	if (!kTab6NpcPrototypeVerboseLog)
		return;
	if (!Prefix)
		Prefix = "slot";
	if (!Slot || !IsSafeLiveObject(static_cast<UObject*>(Slot)))
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " slot=null\n";
		return;
	}

	LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
		<< Prefix << " slotType=" << GetTeammateNpcPrototypeSlotTypeName(Slot) << "\n";

	if (Slot->IsA(UWrapBoxSlot::StaticClass()))
	{
		auto* S = static_cast<UWrapBoxSlot*>(Slot);
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " wrapPad=(" << S->Padding.Left << "," << S->Padding.Top << "," << S->Padding.Right << "," << S->Padding.Bottom << ")"
			<< " hAlign=" << static_cast<int32>(S->HorizontalAlignment)
			<< " vAlign=" << static_cast<int32>(S->VerticalAlignment)
			<< " fillEmpty=" << (S->bFillEmptySpace ? 1 : 0)
			<< " fillSpanLt=" << S->FillSpanWhenLessThan
			<< "\n";
		return;
	}
	if (Slot->IsA(UCanvasPanelSlot::StaticClass()))
	{
		auto* S = static_cast<UCanvasPanelSlot*>(Slot);
		const FAnchorData Layout = S->GetLayout();
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " canvasOffsets=(" << Layout.Offsets.Left << "," << Layout.Offsets.Top << "," << Layout.Offsets.Right << "," << Layout.Offsets.Bottom << ")"
			<< " anchorsMin=(" << Layout.Anchors.Minimum.X << "," << Layout.Anchors.Minimum.Y << ")"
			<< " anchorsMax=(" << Layout.Anchors.Maximum.X << "," << Layout.Anchors.Maximum.Y << ")"
			<< " align=(" << Layout.Alignment.X << "," << Layout.Alignment.Y << ")"
			<< " autoSize=" << (S->GetAutoSize() ? 1 : 0)
			<< " z=" << S->GetZOrder()
			<< "\n";
		return;
	}
	if (Slot->IsA(UButtonSlot::StaticClass()))
	{
		auto* S = static_cast<UButtonSlot*>(Slot);
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " buttonPad=(" << S->Padding.Left << "," << S->Padding.Top << "," << S->Padding.Right << "," << S->Padding.Bottom << ")"
			<< " hAlign=" << static_cast<int32>(S->HorizontalAlignment)
			<< " vAlign=" << static_cast<int32>(S->VerticalAlignment)
			<< "\n";
		return;
	}
	if (Slot->IsA(UBorderSlot::StaticClass()))
	{
		auto* S = static_cast<UBorderSlot*>(Slot);
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " borderPad=(" << S->Padding.Left << "," << S->Padding.Top << "," << S->Padding.Right << "," << S->Padding.Bottom << ")"
			<< " hAlign=" << static_cast<int32>(S->HorizontalAlignment)
			<< " vAlign=" << static_cast<int32>(S->VerticalAlignment)
			<< "\n";
		return;
	}
	if (Slot->IsA(UOverlaySlot::StaticClass()))
	{
		auto* S = static_cast<UOverlaySlot*>(Slot);
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " overlayPad=(" << S->Padding.Left << "," << S->Padding.Top << "," << S->Padding.Right << "," << S->Padding.Bottom << ")"
			<< " hAlign=" << static_cast<int32>(S->HorizontalAlignment)
			<< " vAlign=" << static_cast<int32>(S->VerticalAlignment)
			<< "\n";
		return;
	}
	if (Slot->IsA(USizeBoxSlot::StaticClass()))
	{
		auto* S = static_cast<USizeBoxSlot*>(Slot);
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " sizeBoxPad=(" << S->Padding.Left << "," << S->Padding.Top << "," << S->Padding.Right << "," << S->Padding.Bottom << ")"
			<< " hAlign=" << static_cast<int32>(S->HorizontalAlignment)
			<< " vAlign=" << static_cast<int32>(S->VerticalAlignment)
			<< "\n";
		return;
	}
	if (Slot->IsA(UVerticalBoxSlot::StaticClass()))
	{
		auto* S = static_cast<UVerticalBoxSlot*>(Slot);
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " vboxPad=(" << S->Padding.Left << "," << S->Padding.Top << "," << S->Padding.Right << "," << S->Padding.Bottom << ")"
			<< " hAlign=" << static_cast<int32>(S->HorizontalAlignment)
			<< " vAlign=" << static_cast<int32>(S->VerticalAlignment)
			<< "\n";
		return;
	}
	if (Slot->IsA(UHorizontalBoxSlot::StaticClass()))
	{
		auto* S = static_cast<UHorizontalBoxSlot*>(Slot);
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " hboxPad=(" << S->Padding.Left << "," << S->Padding.Top << "," << S->Padding.Right << "," << S->Padding.Bottom << ")"
			<< " hAlign=" << static_cast<int32>(S->HorizontalAlignment)
			<< " vAlign=" << static_cast<int32>(S->VerticalAlignment)
			<< "\n";
		return;
	}
}

void LogTeammateNpcPrototypeWidgetGeometry(const char* Prefix, UObject* WorldCtx, UWidget* Widget)
{
	if (!kTab6NpcPrototypeVerboseLog)
		return;
	if (!Prefix)
		Prefix = "widget";
	if (!Widget || !IsSafeLiveObject(static_cast<UObject*>(Widget)))
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
			<< Prefix << " widget=null\n";
		return;
	}

	const FGeometry Geo = Widget->GetCachedGeometry();
	const FVector2D Local = USlateBlueprintLibrary::GetLocalSize(Geo);
	const FVector2D Desired = Widget->GetDesiredSize();
	FVector2D PixelTL{}, ViewTL{}, PixelBR{}, ViewBR{};
	if (WorldCtx)
	{
		USlateBlueprintLibrary::LocalToViewport(WorldCtx, Geo, FVector2D{ 0.0f, 0.0f }, &PixelTL, &ViewTL);
		USlateBlueprintLibrary::LocalToViewport(WorldCtx, Geo, Local, &PixelBR, &ViewBR);
	}
	float W = ViewBR.X - ViewTL.X;
	float H = ViewBR.Y - ViewTL.Y;
	float X = ViewTL.X;
	float Y = ViewTL.Y;
	if (W < 0.0f) { W = -W; X = ViewBR.X; }
	if (H < 0.0f) { H = -H; Y = ViewBR.Y; }

	LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] "
		<< Prefix
		<< " pos=(" << X << "," << Y << ")"
		<< " size=(" << W << "," << H << ")"
		<< " local=(" << Local.X << "," << Local.Y << ")"
		<< " desired=(" << Desired.X << "," << Desired.Y << ")"
		<< " renderPivot=(" << Widget->RenderTransformPivot.X << "," << Widget->RenderTransformPivot.Y << ")"
		<< " renderTrans=(" << Widget->RenderTransform.Translation.X << "," << Widget->RenderTransform.Translation.Y << ")"
		<< " renderScale=(" << Widget->RenderTransform.Scale.X << "," << Widget->RenderTransform.Scale.Y << ")"
		<< " renderShear=(" << Widget->RenderTransform.Shear.X << "," << Widget->RenderTransform.Shear.Y << ")"
		<< " renderAngle=" << Widget->RenderTransform.Angle
		<< "\n";
}

void LogTeammateNpcPrototypeLayout(const char* StageTag)
{
	if (!kTab6NpcPrototypeVerboseLog)
		return;

	if (!GTeammateNpcPrototypeCardWrap ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcPrototypeCardWrap), UWrapBox::StaticClass()))
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout] stage="
			<< (StageTag ? StageTag : "null")
			<< " wrapInvalid=1\n";
		return;
	}

	GTeammateNpcPrototypeCardWrap->ForceLayoutPrepass();
	UPanelWidget* P0 = GTeammateNpcPrototypeCardWrap->GetParent();
	if (P0 && IsSafeLiveObject(static_cast<UObject*>(P0)))
	{
		P0->ForceLayoutPrepass();
		UPanelWidget* P1 = P0->GetParent();
		if (P1 && IsSafeLiveObject(static_cast<UObject*>(P1)))
			P1->ForceLayoutPrepass();
	}

	UObject* WorldCtx = nullptr;
	if (UWorld* World = UWorld::GetWorld())
		WorldCtx = static_cast<UObject*>(World);
	else if (GTeammateNpcPrototypeOwnerPC && IsSafeLiveObject(static_cast<UObject*>(GTeammateNpcPrototypeOwnerPC)))
		WorldCtx = static_cast<UObject*>(GTeammateNpcPrototypeOwnerPC);

	const FGeometry WrapGeo = GTeammateNpcPrototypeCardWrap->GetCachedGeometry();
	const FVector2D WrapLocal = USlateBlueprintLibrary::GetLocalSize(WrapGeo);

	FVector2D WrapPixelTL{}, WrapViewTL{}, WrapPixelBR{}, WrapViewBR{};
	if (WorldCtx)
	{
		USlateBlueprintLibrary::LocalToViewport(WorldCtx, WrapGeo, FVector2D{ 0.0f, 0.0f }, &WrapPixelTL, &WrapViewTL);
		USlateBlueprintLibrary::LocalToViewport(WorldCtx, WrapGeo, WrapLocal, &WrapPixelBR, &WrapViewBR);
	}

	float WrapViewW = WrapViewBR.X - WrapViewTL.X;
	float WrapViewH = WrapViewBR.Y - WrapViewTL.Y;
	if (WrapViewW < 0.0f) WrapViewW = -WrapViewW;
	if (WrapViewH < 0.0f) WrapViewH = -WrapViewH;

	LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout] stage="
		<< (StageTag ? StageTag : "null")
		<< " wrapCfg=(" << GTeammateNpcPrototypeCardWrap->WrapSize << "," << (GTeammateNpcPrototypeCardWrap->bExplicitWrapSize ? 1 : 0) << ")"
		<< " wrapChildren=" << GTeammateNpcPrototypeCardWrap->GetChildrenCount()
		<< " bindings=" << GTeammateNpcPrototypeCards.size()
		<< " local=(" << WrapLocal.X << "," << WrapLocal.Y << ")"
		<< " view=(" << WrapViewW << "," << WrapViewH << ")"
		<< " page=" << (GTeammateNpcPrototypeCurrentPage + 1) << "/" << (GTeammateNpcPrototypeTotalPages > 0 ? GTeammateNpcPrototypeTotalPages : 1)
		<< " filtered=" << GTeammateNpcPrototypeFilteredIndices.size()
		<< "\n";

	struct FCardGeom
	{
		int32 Index = -1;
		int32 NpcId = 0;
		float X = 0.0f;
		float Y = 0.0f;
		float W = 0.0f;
		float H = 0.0f;
		float LocalW = 0.0f;
		float LocalH = 0.0f;
		float DesiredW = 0.0f;
		float DesiredH = 0.0f;
	};
	std::vector<FCardGeom> Geoms;
	Geoms.reserve(GTeammateNpcPrototypeCards.size());

	for (int32 i = 0; i < static_cast<int32>(GTeammateNpcPrototypeCards.size()); ++i)
	{
		auto& Binding = GTeammateNpcPrototypeCards[i];
		if (!Binding.FrameBorder ||
			!IsSafeLiveObjectOfClass(static_cast<UObject*>(Binding.FrameBorder), UBorder::StaticClass()))
		{
			continue;
		}

		const FGeometry Geo = Binding.FrameBorder->GetCachedGeometry();
		const FVector2D Local = USlateBlueprintLibrary::GetLocalSize(Geo);
		const FVector2D Desired = Binding.FrameBorder->GetDesiredSize();

		FVector2D PixelTL{}, ViewTL{}, PixelBR{}, ViewBR{};
		if (WorldCtx)
		{
			USlateBlueprintLibrary::LocalToViewport(WorldCtx, Geo, FVector2D{ 0.0f, 0.0f }, &PixelTL, &ViewTL);
			USlateBlueprintLibrary::LocalToViewport(WorldCtx, Geo, Local, &PixelBR, &ViewBR);
		}

		float W = ViewBR.X - ViewTL.X;
		float H = ViewBR.Y - ViewTL.Y;
		float X = ViewTL.X;
		float Y = ViewTL.Y;
		if (W < 0.0f) { W = -W; X = ViewBR.X; }
		if (H < 0.0f) { H = -H; Y = ViewBR.Y; }

		FCardGeom G{};
		G.Index = i;
		G.NpcId = Binding.NpcId;
		G.X = X;
		G.Y = Y;
		G.W = W;
		G.H = H;
		G.LocalW = Local.X;
		G.LocalH = Local.Y;
		G.DesiredW = Desired.X;
		G.DesiredH = Desired.Y;
		Geoms.push_back(G);
	}

	std::sort(Geoms.begin(), Geoms.end(),
		[](const FCardGeom& A, const FCardGeom& B)
		{
			const float DY = A.Y - B.Y;
			if (DY < -1.0f || DY > 1.0f)
				return A.Y < B.Y;
			return A.X < B.X;
		});

	auto AbsF = [](float V) -> float { return V < 0.0f ? -V : V; };
	std::vector<float> RowAnchors;
	std::vector<int32> RowCounts;
	const float RowEpsilon = 18.0f;
	for (const auto& G : Geoms)
	{
		int32 RowIdx = -1;
		for (int32 i = 0; i < static_cast<int32>(RowAnchors.size()); ++i)
		{
			if (AbsF(G.Y - RowAnchors[i]) <= RowEpsilon)
			{
				RowIdx = i;
				break;
			}
		}
		if (RowIdx < 0)
		{
			RowAnchors.push_back(G.Y);
			RowCounts.push_back(0);
			RowIdx = static_cast<int32>(RowCounts.size()) - 1;
		}
		RowCounts[RowIdx] += 1;
	}

	std::string RowDist;
	for (int32 i = 0; i < static_cast<int32>(RowCounts.size()); ++i)
	{
		if (i > 0) RowDist += "+";
		RowDist += std::to_string(RowCounts[i]);
	}
	if (RowDist.empty())
		RowDist = "none";

	LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout] stage="
		<< (StageTag ? StageTag : "null")
		<< " rows=" << RowCounts.size()
		<< " rowDist=" << RowDist.c_str()
		<< " cardsLogged=" << Geoms.size()
		<< "\n";

	for (const auto& G : Geoms)
	{
		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout] card"
			<< " idx=" << G.Index
			<< " npcId=" << G.NpcId
			<< " pos=(" << G.X << "," << G.Y << ")"
			<< " size=(" << G.W << "," << G.H << ")"
			<< " local=(" << G.LocalW << "," << G.LocalH << ")"
			<< " desired=(" << G.DesiredW << "," << G.DesiredH << ")"
			<< "\n";
	}

	const bool bDetailed = (StageTag && StageTag[0] == 'r');
	if (!bDetailed)
		return;

	int32 LoggedDetailCards = 0;
	for (int32 i = 0; i < static_cast<int32>(GTeammateNpcPrototypeCards.size()); ++i)
	{
		auto& Binding = GTeammateNpcPrototypeCards[i];
		if (!Binding.FrameBorder || !IsSafeLiveObject(static_cast<UObject*>(Binding.FrameBorder)))
			continue;

		LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] card"
			<< " idx=" << i
			<< " npcId=" << Binding.NpcId
			<< "\n";

		if (Binding.ClickButton && IsSafeLiveObject(static_cast<UObject*>(Binding.ClickButton)))
		{
			const FButtonStyle& BtnStyle = Binding.ClickButton->WidgetStyle;
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto][Layout][Detail] buttonStyle"
				<< " normalPad=(" << BtnStyle.NormalPadding.Left << "," << BtnStyle.NormalPadding.Top << "," << BtnStyle.NormalPadding.Right << "," << BtnStyle.NormalPadding.Bottom << ")"
				<< " pressedPad=(" << BtnStyle.PressedPadding.Left << "," << BtnStyle.PressedPadding.Top << "," << BtnStyle.PressedPadding.Right << "," << BtnStyle.PressedPadding.Bottom << ")"
				<< "\n";
		}

		LogTeammateNpcPrototypeWidgetGeometry("frameBorder", WorldCtx, Binding.FrameBorder);
		LogTeammateNpcPrototypePanelSlotInfo("frameBorder.slot", Binding.FrameBorder ? Binding.FrameBorder->Slot : nullptr);

		LogTeammateNpcPrototypeWidgetGeometry("clickButton", WorldCtx, Binding.ClickButton);
		LogTeammateNpcPrototypePanelSlotInfo("clickButton.slot", Binding.ClickButton ? Binding.ClickButton->Slot : nullptr);
		if (Binding.ClickButton)
			LogTeammateNpcPrototypePanelSlotInfo("clickButton.contentSlot", Binding.ClickButton->GetContentSlot());

		LogTeammateNpcPrototypeWidgetGeometry("cardHost", WorldCtx, Binding.CardHost);
		LogTeammateNpcPrototypePanelSlotInfo("cardHost.slot", Binding.CardHost ? Binding.CardHost->Slot : nullptr);
		if (Binding.CardHost)
			LogTeammateNpcPrototypePanelSlotInfo("cardHost.contentSlot", Binding.CardHost->GetContentSlot());

		LogTeammateNpcPrototypeWidgetGeometry("cardWidget", WorldCtx, Binding.CardWidget);
		LogTeammateNpcPrototypePanelSlotInfo("cardWidget.slot", Binding.CardWidget ? Binding.CardWidget->Slot : nullptr);

		if (Binding.CardWidget && Binding.CardWidget->IMG_Main)
		{
			LogTeammateNpcPrototypeWidgetGeometry("imgMain", WorldCtx, Binding.CardWidget->IMG_Main);
			LogTeammateNpcPrototypePanelSlotInfo("imgMain.slot", Binding.CardWidget->IMG_Main->Slot);
		}
		if (Binding.CardWidget && Binding.CardWidget->TXT_Title)
		{
			LogTeammateNpcPrototypeWidgetGeometry("txtTitle", WorldCtx, Binding.CardWidget->TXT_Title);
			LogTeammateNpcPrototypePanelSlotInfo("txtTitle.slot", Binding.CardWidget->TXT_Title->Slot);
		}

		if (Binding.CardWidget && Binding.CardWidget->IsA(UJHGHC_RegionEle_NPC_C::StaticClass()))
		{
			auto* RegionNpcCard = static_cast<UJHGHC_RegionEle_NPC_C*>(Binding.CardWidget);
			if (RegionNpcCard->JHGPCBtn_ActiveBG &&
				IsSafeLiveObjectOfClass(static_cast<UObject*>(RegionNpcCard->JHGPCBtn_ActiveBG), UWidget::StaticClass()))
			{
				auto* ActiveBgWidget = static_cast<UWidget*>(RegionNpcCard->JHGPCBtn_ActiveBG);
				LogTeammateNpcPrototypeWidgetGeometry("activeBg", WorldCtx, ActiveBgWidget);
				LogTeammateNpcPrototypePanelSlotInfo("activeBg.slot", ActiveBgWidget->Slot);
			}
		}

		++LoggedDetailCards;
		if (LoggedDetailCards >= 2)
			break;
	}
}

} // end anonymous namespace

void PollTab6NpcPrototypeSelection_Impl(bool bTab6Active)
{
	if (!bTab6Active)
	{
		if (!GTeammateNpcPrototypePollWasActive)
			return;
		GTeammateNpcPrototypePollWasActive = false;

		for (auto& Binding : GTeammateNpcPrototypeCards)
			Binding.WasPressed = false;
		GTeammateNpcPrototypeLastSelectionPollTick = 0;
		GTeammateNpcPrototypeLastPagerPollTick = 0;
		GTeammateNpcPrototypePrevWasPressed = false;
		GTeammateNpcPrototypeNextWasPressed = false;
		GTeammateNpcPrototypeLastLayoutLogTick = 0;
		ResetTeammateNpcAddConfirmState();
		return;
	}
	GTeammateNpcPrototypePollWasActive = true;

	const ULONGLONG Now = GetTickCount64();
	// 关闭轮询阶段布局日志，避免持续刷屏。
	const bool bDoPagerPoll =
		!GTeammateNpcPrototypeLastPagerPollTick ||
		(Now - GTeammateNpcPrototypeLastPagerPollTick) >= kTeammateNpcPagerPollIntervalMs;
	if (bDoPagerPoll)
	{
		GTeammateNpcPrototypeLastPagerPollTick = Now;

		if (UpdateTeammateNpcSearchKeywordFromEdit())
		{
			GTeammateNpcPrototypeCurrentPage = 0;
			RebuildTeammateNpcPrototypeFilteredIndices();
			RefreshTeammateNpcPrototypePage();
		}

		auto GetClickableButton = [](UJHCommon_Btn_Free_C* W) -> UButton*
		{
			if (!W) return nullptr;
			if (W->Btn) return W->Btn;
			if (W->JHGPCBtn)
				return static_cast<UJHNeoUIGamepadConfirmButton*>(W->JHGPCBtn)->BtnMain;
			return nullptr;
		};

		UButton* PrevInner = GetClickableButton(GTeammateNpcPrototypePrevPageBtn);
		const bool PrevPressed = PrevInner &&
			IsSafeLiveObject(static_cast<UObject*>(PrevInner)) &&
			PrevInner->IsPressed();
		if (GTeammateNpcPrototypePrevWasPressed && !PrevPressed && GTeammateNpcPrototypeCurrentPage > 0)
		{
			--GTeammateNpcPrototypeCurrentPage;
			RefreshTeammateNpcPrototypePage();
		}
		GTeammateNpcPrototypePrevWasPressed = PrevPressed;

		UButton* NextInner = GetClickableButton(GTeammateNpcPrototypeNextPageBtn);
		const bool NextPressed = NextInner &&
			IsSafeLiveObject(static_cast<UObject*>(NextInner)) &&
			NextInner->IsPressed();
		if (GTeammateNpcPrototypeNextWasPressed && !NextPressed &&
			(GTeammateNpcPrototypeCurrentPage + 1) < GTeammateNpcPrototypeTotalPages)
		{
			++GTeammateNpcPrototypeCurrentPage;
			RefreshTeammateNpcPrototypePage();
		}
		GTeammateNpcPrototypeNextWasPressed = NextPressed;
	}

	PollTeammateNpcAddConfirmDialog();

	for (auto& Binding : GTeammateNpcPrototypeCards)
		EnsureTeammateNpcPrototypeCardTitleStyle(Binding);

	const bool bDoSelectionPoll =
		!GTeammateNpcPrototypeLastSelectionPollTick ||
		(Now - GTeammateNpcPrototypeLastSelectionPollTick) >= kTeammateNpcSelectionPollIntervalMs;
	if (!bDoSelectionPoll)
		return;
	GTeammateNpcPrototypeLastSelectionPollTick = Now;

	bool bSelectionChanged = false;
	for (auto& Binding : GTeammateNpcPrototypeCards)
	{
		UButton* Button = Binding.ClickButton;
		if (!Button || !IsSafeLiveObjectOfClass(static_cast<UObject*>(Button), UButton::StaticClass()))
		{
			Binding.WasPressed = false;
			continue;
		}

		const bool bPressed = Button->IsPressed();
		if (Binding.WasPressed && !bPressed && Binding.NpcId >= 0)
		{
			GTeammateNpcPrototypeSelectedNpcId = Binding.NpcId;
			bSelectionChanged = true;
			(void)ShowTeammateNpcAddConfirmDialog(Binding.NpcId);
		}

		Binding.WasPressed = bPressed;
	}

	if (bSelectionChanged)
		ApplyTeammateNpcPrototypeSelectionVisuals();
}

void PollTab6NpcPrototypeSelection(bool bTab6Active)
{
	__try
	{
		PollTab6NpcPrototypeSelection_Impl(bTab6Active);
	}
	__except (HandleTab6NpcProtoSehException("PollSelection", GTeammateNpcPrototypeSelectedNpcId, GetExceptionCode()))
	{
		GTeammateNpcPrototypePollWasActive = false;
		for (auto& Binding : GTeammateNpcPrototypeCards)
			Binding.WasPressed = false;
		GTeammateNpcPrototypeLastSelectionPollTick = 0;
		GTeammateNpcPrototypeLastPagerPollTick = 0;
		ResetTeammateNpcAddConfirmState();
	}
}

UObject* GetTab6NpcConfirmAlertForProcessEventHook()
{
	if (!GTeammateNpcAddConfirmPending)
		return nullptr;
	if (!GTeammateNpcAddConfirmAlert ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(GTeammateNpcAddConfirmAlert), UJHNeoUIAlertContentSimple::StaticClass()))
	{
		return nullptr;
	}
	return static_cast<UObject*>(GTeammateNpcAddConfirmAlert);
}

void HandleTab6NpcConfirmProcessEventAction(bool bConfirmButton)
{
	if (!GTeammateNpcAddConfirmPending)
		return;

	const int32 TargetNpcId = GTeammateNpcAddConfirmPendingNpcId;
	if (bConfirmButton)
	{
		LOGI_STREAM("Tab6Teammates")
			<< "[SDK][Tab6NpcProto] ConfirmDialog yes trigger: source=process_event_handlebtn1"
			<< " npcId=" << TargetNpcId << "\n";
		ResetTeammateNpcAddConfirmState();
		(void)AddTeammateNpcToTeamById(TargetNpcId);
		return;
	}

	LOGI_STREAM("Tab6Teammates")
		<< "[SDK][Tab6NpcProto] ConfirmDialog no trigger: source=process_event_handlebtn2\n";
	ResetTeammateNpcAddConfirmState();
}

void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTab.Content6) return;
	GDynTab.Content6->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GTeammate.FollowToggle = nullptr;
	GTeammate.FollowCount = nullptr;
	GTeammate.AddDD = nullptr;
	GTeammate.ReplaceToggle = nullptr;
	GTeammate.ReplaceDD = nullptr;
	ReleaseTeammateNpcPrototypeWidgets();

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTab.Content6->AddChild(Panel);
		if (Slot && Slot->IsA(UVerticalBoxSlot::StaticClass()))
		{
			auto* VSlot = static_cast<UVerticalBoxSlot*>(Slot);
			FMargin Pad{};
			Pad.Top = TopGap;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto* OperatePanel = CreateCollapsiblePanel(PC, L"队友操作");
	auto* OperateBox = OperatePanel ? OperatePanel->CT_Contents : nullptr;
	GTeammate.AddDD = CreateVideoItemWithOptions(PC, L"添加队友",
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammate.AddDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.AddDD);
		else GDynTab.Content6->AddChild(GTeammate.AddDD);
		Count++;
	}
	GTeammate.ReplaceToggle = CreateToggleItem(PC, L"替换指定队友");
	if (GTeammate.ReplaceToggle)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.ReplaceToggle);
		else GDynTab.Content6->AddChild(GTeammate.ReplaceToggle);
		Count++;
	}
	GTeammate.ReplaceDD = CreateVideoItemWithOptions(PC, L"指定队友",
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢渊", L"唐婉莹", L"徐小七", L"向天歌" });
	if (GTeammate.ReplaceDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammate.ReplaceDD);
		else GDynTab.Content6->AddChild(GTeammate.ReplaceDD);
		Count++;
	}
	AddPanelWithFixedGap(OperatePanel, 0.0f, 8.0f);

	auto* PrototypePanel = CreateCollapsiblePanel(PC, L"NPC浏览器");
	auto* PrototypeBox = PrototypePanel ? PrototypePanel->CT_Contents : nullptr;
	if (PrototypeBox)
	{
		UEditableTextBox* SearchEdit = static_cast<UEditableTextBox*>(
			CreateRawWidget(UEditableTextBox::StaticClass(), Outer));
		if (SearchEdit)
		{
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(SearchEdit));
			SearchEdit->SetHintText(MakeText(L"输入以搜索..."));
			SearchEdit->SetText(MakeText(L""));
			SearchEdit->SetJustification(ETextJustify::Left);
			SearchEdit->MinimumDesiredWidth = 500.0f;
			SearchEdit->SelectAllTextWhenFocused = true;
			SearchEdit->ClearKeyboardFocusOnCommit = false;
			SearchEdit->Font.Size = 16;
			SearchEdit->WidgetStyle.Font.Size = 16;
			SearchEdit->WidgetStyle.Padding.Left = 10.0f;
			SearchEdit->WidgetStyle.Padding.Top = 4.0f;
			SearchEdit->WidgetStyle.Padding.Right = 10.0f;
			SearchEdit->WidgetStyle.Padding.Bottom = 4.0f;
			const FSlateColor SearchBgTint(FLinearColor{ 0.0f, 0.0f, 0.0f, 0.0f });
			const FSlateColor SearchTextTint(FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f });
			SearchEdit->WidgetStyle.BackgroundImageNormal.TintColor = SearchBgTint;
			SearchEdit->WidgetStyle.BackgroundImageHovered.TintColor = SearchBgTint;
			SearchEdit->WidgetStyle.BackgroundImageFocused.TintColor = SearchBgTint;
			SearchEdit->WidgetStyle.BackgroundImageReadOnly.TintColor = SearchBgTint;
			SearchEdit->WidgetStyle.ForegroundColor = SearchTextTint;
			SearchEdit->WidgetStyle.ReadOnlyForegroundColor = SearchTextTint;
			SearchEdit->ForegroundColor = FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			SearchEdit->ReadOnlyForegroundColor = FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			ClearEditableTextBindings(SearchEdit);

			UWidget* SearchHost = SearchEdit;
			auto* SearchSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
			if (SearchSize)
			{
				RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(SearchSize));
				SearchSize->SetWidthOverride(560.0f);
				SearchSize->SetHeightOverride(44.0f);
				SearchSize->SetContent(SearchEdit);
				SearchHost = SearchSize;
			}

			GTeammateNpcPrototypeSearchEdit = SearchEdit;
			if (UPanelSlot* SearchSlot = PrototypeBox->AddChild(SearchHost))
			{
				if (SearchSlot->IsA(UVerticalBoxSlot::StaticClass()))
				{
					auto* VSlot = static_cast<UVerticalBoxSlot*>(SearchSlot);
					VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
					VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
					FMargin Pad{};
					Pad.Bottom = 10.0f;
					VSlot->SetPadding(Pad);
				}
			}
			Count++;
		}

		UWrapBox* CardWrap = static_cast<UWrapBox*>(CreateRawWidget(UWrapBox::StaticClass(), Outer));
		auto* CardViewport = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
		if (CardWrap)
		{
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(CardWrap));
			CardWrap->SetInnerSlotPadding(FVector2D{ 25.0f, 33.5f });
			CardWrap->WrapSize = 640.0f;
			CardWrap->bExplicitWrapSize = true;

			bool bMountedListContainer = false;
			if (CardViewport)
			{
				RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(CardViewport));
				CardViewport->SetWidthOverride(640.0f);
				CardViewport->SetHeightOverride(340.0f);
				CardViewport->SetContent(CardWrap);
				if (UPanelSlot* RowSlot = PrototypeBox->AddChild(CardViewport))
				{
					if (RowSlot->IsA(UVerticalBoxSlot::StaticClass()))
					{
						auto* VSlot = static_cast<UVerticalBoxSlot*>(RowSlot);
						VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
						VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
						FMargin Pad{};
						Pad.Bottom = 0.0f;
						VSlot->SetPadding(Pad);
					}
				}
				bMountedListContainer = true;
			}

			if (!bMountedListContainer)
			{
				if (UPanelSlot* RowSlot = PrototypeBox->AddChild(CardWrap))
				{
					if (RowSlot->IsA(UVerticalBoxSlot::StaticClass()))
					{
						auto* VSlot = static_cast<UVerticalBoxSlot*>(RowSlot);
						VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
						VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
						FMargin Pad{};
						Pad.Bottom = 0.0f;
						VSlot->SetPadding(Pad);
					}
				}
			}

			auto* PagerRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
			if (PagerRow)
			{
				RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(PagerRow));

				UWidget* PrevLayout = nullptr;
				GTeammateNpcPrototypePrevPageBtn = CreateGameStyleButton(
					PC,
					L"上一页",
					"Tab6NpcPrevPage",
					136.0f,
					48.0f,
					&PrevLayout);
				if (PrevLayout)
				{
					auto* PrevSlot = PagerRow->AddChildToHorizontalBox(PrevLayout);
					if (PrevSlot)
					{
						PrevSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
						PrevSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
					}
				}

				GTeammateNpcPrototypePageLabel = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
				if (GTeammateNpcPrototypePageLabel)
				{
					RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(GTeammateNpcPrototypePageLabel));
					GTeammateNpcPrototypePageLabel->SetText(MakeText(L"1/1"));
					GTeammateNpcPrototypePageLabel->SetJustification(ETextJustify::Center);
					GTeammateNpcPrototypePageLabel->SetMinDesiredWidth(92.0f);
					GTeammateNpcPrototypePageLabel->Font.Size = 18;
					auto* LabelSlot = PagerRow->AddChildToHorizontalBox(GTeammateNpcPrototypePageLabel);
					if (LabelSlot)
					{
						LabelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
						LabelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
					}
				}

				UWidget* NextLayout = nullptr;
				GTeammateNpcPrototypeNextPageBtn = CreateGameStyleButton(
					PC,
					L"下一页",
					"Tab6NpcNextPage",
					136.0f,
					48.0f,
					&NextLayout);
				if (NextLayout)
				{
					auto* NextSlot = PagerRow->AddChildToHorizontalBox(NextLayout);
					if (NextSlot)
					{
						NextSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
						NextSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
					}
				}

				if (UPanelSlot* PagerSlot = PrototypeBox->AddChild(PagerRow))
				{
					if (PagerSlot->IsA(UVerticalBoxSlot::StaticClass()))
					{
						auto* VSlot = static_cast<UVerticalBoxSlot*>(PagerSlot);
						VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
						VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
						FMargin Pad{};
						Pad.Top = 2.0f;
						VSlot->SetPadding(Pad);
					}
				}
				Count++;
			}

			int32 BuiltCards = 0;
			int32 TotalSamples = 0;
			if (!GTeammateNpcNeoUISubsystemLogged)
			{
				(void)GetJHNeoUISubsystemForTab6(true);
				GTeammateNpcNeoUISubsystemLogged = true;
			}
			if (!GTeammateNpcSimpleAlertProbeLogged)
			{
				ProbeTeammateNpcSimpleAlert();
				GTeammateNpcSimpleAlertProbeLogged = true;
			}
			const bool bCanBuildPrototypeCards = CanBuildTeammateNpcPrototypeCards(PC);
			if (kTab6NpcPrototypeVerboseLog)
			{
				LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] PopulateTab_Teammates begin canBuild="
				          << (bCanBuildPrototypeCards ? 1 : 0)
				          << " buildTag=" << kTab6NpcPrototypeBuildTag << "\n";
			}
			if (bCanBuildPrototypeCards)
			{
				GTeammateNpcPrototypeAllSamples = CollectTeammateNpcPrototypeSamples();
				TotalSamples = static_cast<int32>(GTeammateNpcPrototypeAllSamples.size());
				GTeammateNpcPrototypeCardWrap = CardWrap;
				GTeammateNpcPrototypeOwnerPC = PC;
				GTeammateNpcPrototypeOwnerOuter = Outer;
				GTeammateNpcPrototypeCurrentPage = 0;
				GTeammateNpcPrototypeTotalPages = 0;

				if (kTab6NpcPrototypeVerboseLog)
				{
					LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] PopulateTab_Teammates samples=" << TotalSamples << "\n";
				}

				RebuildTeammateNpcPrototypeFilteredIndices();
				RefreshTeammateNpcPrototypePage();
				BuiltCards = static_cast<int32>(GTeammateNpcPrototypeCards.size());
			}
			LOGI_STREAM("Tab6Teammates") << "[SDK][Tab6NpcProto] PopulateTab_Teammates summary totalSamples="
				<< TotalSamples
				<< " filtered=" << GTeammateNpcPrototypeFilteredIndices.size()
				<< " page=" << (GTeammateNpcPrototypeCurrentPage + 1) << "/" << (GTeammateNpcPrototypeTotalPages > 0 ? GTeammateNpcPrototypeTotalPages : 1)
				<< " pageCards=" << BuiltCards
				<< " bindings=" << GTeammateNpcPrototypeCards.size()
				<< " buildTag=" << kTab6NpcPrototypeBuildTag << "\n";

			if (BuiltCards > 0 && !GTeammateNpcPrototypeCards.empty())
			{
				if (!HasTeammateNpcPrototypeCardBinding(GTeammateNpcPrototypeSelectedNpcId))
					GTeammateNpcPrototypeSelectedNpcId = GTeammateNpcPrototypeCards.front().NpcId;
				ApplyTeammateNpcPrototypeSelectionVisuals();
			}

			if (BuiltCards == 0 || GTeammateNpcPrototypeCards.empty())
			{
				auto* FailLabel = CreateRawTextLabel(
					Outer,
					L"当前未匹配到可用 NPC 图鉴卡片资源，先显示占位；请查看日志 [SDK][Tab6NpcProto] 的 resource/flipbook 诊断信息。");
				if (FailLabel)
				{
					RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(FailLabel));
					FailLabel->Font.Size = 15;
					if (UPanelSlot* FailSlot = PrototypeBox->AddChild(FailLabel))
					{
						if (FailSlot->IsA(UVerticalBoxSlot::StaticClass()))
						{
							auto* VSlot = static_cast<UVerticalBoxSlot*>(FailSlot);
							FMargin Pad{};
							Pad.Top = 6.0f;
							VSlot->SetPadding(Pad);
						}
					}
					Count++;
				}
			}
			else
			{
				Count++;
			}
		}
	}
	AddPanelWithFixedGap(PrototypePanel, 0.0f, 8.0f);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  Tab6 Enable/Disable implementations
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

namespace
{

// ── 设置队友跟随数量 (SDK直写) ──
int32 GOriginalFollowerCount = -1; // -1 = not saved yet
constexpr uintptr_t kFollowerCountOffset = 0x0518;

bool IsTab6WritableAddress(const void* Address, size_t Size)
{
	if (!Address || Size == 0)
		return false;

	const uint8_t* Cursor = reinterpret_cast<const uint8_t*>(Address);
	size_t Remaining = Size;
	while (Remaining > 0)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		if (::VirtualQuery(Cursor, &mbi, sizeof(mbi)) == 0)
			return false;
		if (mbi.State != MEM_COMMIT)
			return false;
		if ((mbi.Protect & PAGE_GUARD) != 0 || (mbi.Protect & PAGE_NOACCESS) != 0)
			return false;

		const DWORD Protect = (mbi.Protect & 0xFF);
		const bool Writable =
			(Protect == PAGE_READWRITE) ||
			(Protect == PAGE_WRITECOPY) ||
			(Protect == PAGE_EXECUTE_READWRITE) ||
			(Protect == PAGE_EXECUTE_WRITECOPY);
		if (!Writable)
			return false;

		const uint8_t* RegionEnd = reinterpret_cast<const uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;
		if (RegionEnd <= Cursor)
			return false;

		const size_t Chunk = static_cast<size_t>(RegionEnd - Cursor);
		if (Chunk >= Remaining)
			return true;
		Cursor += Chunk;
		Remaining -= Chunk;
	}

	return true;
}

// ── 替换指定队友 (AOB hook) ──
uint32_t GReplaceTeammateHookId = UINT32_MAX;
uintptr_t GReplaceTeammateOffset = 0;
volatile LONG GReplaceTeammateId = 30;
const char* kReplaceTeammatePattern = "48 33 C4 48 89 45 ?? 44 88 44 24 ??";

const unsigned char kReplaceTeammateTrampolineTemplate[] = {
	0x41, 0x53,                                       // push r11
	0x49, 0xBB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov r11, imm64 (&GReplaceTeammateId)
	0x41, 0x8B, 0x13,                                 // mov edx, dword [r11]
	0x41, 0x5B,                                       // pop r11
};
constexpr size_t kReplaceTeammateImm64Offset = 4;

constexpr int32 kRoleIds[] = { 0, 30, 31, 32, 33, 34, 35, 36 };

} // end anonymous namespace

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  设置队友跟随数量 — SDK approach (ASceneHero::FollowerCount at +0x0518)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void ApplyFollowerCountSDK(int32 Value)
{
	if (Value < 1) Value = 1;
	if (Value > 99) Value = 99;

	UWorld* World = UWorld::GetWorld();
	if (!World) return;

	AActor* Hero = UNPCFuncLib::GetSceneHero(static_cast<UObject*>(World));
	if (!Hero || !IsSafeLiveObject(static_cast<UObject*>(Hero)))
		return;

	uint8* HeroPtr = reinterpret_cast<uint8*>(Hero);
	int32* FollowerCountPtr = reinterpret_cast<int32*>(HeroPtr + kFollowerCountOffset);
	if (!IsTab6WritableAddress(FollowerCountPtr, sizeof(int32)))
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK] FollowerCount write guard blocked hero=" << Hero
			<< " ptr=" << FollowerCountPtr
			<< " offset=0x" << std::hex << kFollowerCountOffset << std::dec << "\n";
		return;
	}

	int32& FollowerCount = *FollowerCountPtr;
	// Save original on first call
	if (GOriginalFollowerCount < 0)
		GOriginalFollowerCount = FollowerCount;

	if (FollowerCount != Value)
	{
		FollowerCount = Value;
		LOGI_STREAM("Tab6Teammates") << "[SDK] FollowerCount set to " << Value << "\n";
	}
}

void DisableFollowerCountSDK()
{
	if (GOriginalFollowerCount < 0)
		return; // nothing to restore

	UWorld* World = UWorld::GetWorld();
	if (!World) return;

	AActor* Hero = UNPCFuncLib::GetSceneHero(static_cast<UObject*>(World));
	if (!Hero || !IsSafeLiveObject(static_cast<UObject*>(Hero)))
	{
		GOriginalFollowerCount = -1;
		return;
	}

	uint8* HeroPtr = reinterpret_cast<uint8*>(Hero);
	int32* FollowerCountPtr = reinterpret_cast<int32*>(HeroPtr + kFollowerCountOffset);
	if (!IsTab6WritableAddress(FollowerCountPtr, sizeof(int32)))
	{
		LOGE_STREAM("Tab6Teammates")
			<< "[SDK] FollowerCount restore guard blocked hero=" << Hero
			<< " ptr=" << FollowerCountPtr
			<< " offset=0x" << std::hex << kFollowerCountOffset << std::dec << "\n";
		GOriginalFollowerCount = -1;
		return;
	}

	*FollowerCountPtr = GOriginalFollowerCount;
	LOGI_STREAM("Tab6Teammates") << "[SDK] FollowerCount restored to " << GOriginalFollowerCount << "\n";
	GOriginalFollowerCount = -1;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  替换指定队友
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void SetReplaceTeammateId(int32 Value)
{
	InterlockedExchange(&GReplaceTeammateId, static_cast<LONG>(Value));
}

void EnableReplaceTeammateHook()
{
	if (GReplaceTeammateHookId != UINT32_MAX) return;

	if (GReplaceTeammateOffset == 0)
	{
		const uintptr_t found = InlineHook::HookManager::ScanModulePatternRobust(
			"JH-Win64-Shipping.exe", kReplaceTeammatePattern);
		if (found == 0)
		{
			LOGE_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate AobScan failed\n";
			return;
		}
		HMODULE hModule = GetModuleHandleA("JH-Win64-Shipping.exe");
		if (!hModule) return;
		GReplaceTeammateOffset = found - reinterpret_cast<uintptr_t>(hModule);
	}

	unsigned char code[sizeof(kReplaceTeammateTrampolineTemplate)];
	std::memcpy(code, kReplaceTeammateTrampolineTemplate, sizeof(code));
	const uintptr_t idAddr = reinterpret_cast<uintptr_t>(&GReplaceTeammateId);
	std::memcpy(code + kReplaceTeammateImm64Offset, &idAddr, sizeof(idAddr));

	uint32_t hookId = UINT32_MAX;
	if (!InlineHook::HookManager::InstallHook(
		"JH-Win64-Shipping.exe",
		static_cast<uint32_t>(GReplaceTeammateOffset),
		code, sizeof(code), hookId))
	{
		LOGE_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate hook install failed\n";
		return;
	}
	GReplaceTeammateHookId = hookId;
	LOGI_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate hook enabled, ID: " << hookId << "\n";
}

void DisableReplaceTeammateHook()
{
	if (GReplaceTeammateHookId == UINT32_MAX) return;
	InlineHook::HookManager::UninstallHook(GReplaceTeammateHookId);
	GReplaceTeammateHookId = UINT32_MAX;
	LOGI_STREAM("Tab6Teammates") << "[SDK] ReplaceTeammate hook disabled\n";
}
