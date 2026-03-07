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
bool GTeammateNpcResourceTableSchemaLogged = false;

constexpr int32 kTeammateNpcPrototypePerPage = 8;
constexpr float kTeammateNpcCardScaleNormal = 0.76f;
constexpr float kTeammateNpcCardScaleSelected = 0.84f;
constexpr float kTeammateNpcCardHostWidth = 142.0f;
constexpr float kTeammateNpcCardHostHeight = 212.0f;
constexpr ULONGLONG kTeammateNpcSelectionPollIntervalMs = 33ULL;
constexpr ULONGLONG kTeammateNpcPagerPollIntervalMs = 33ULL;
constexpr bool kTab6NpcPrototypeVerboseLog = false;
constexpr const wchar_t* kTab6NpcPrototypeBuildTag = L"tab6-npc-page-search-20260308-0105";

void LogTeammateNpcResourceTableSchema(UDataTable* Table);
void RebuildTeammateNpcPrototypeFilteredIndices();
void RefreshTeammateNpcPrototypePage();

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
	}

	UpdateTeammateNpcPrototypeSelectionSummary();
}

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

void RefreshTeammateNpcPrototypeCardContent(FTeammateNpcPrototypeCardBinding& Binding)
{
	if (!Binding.CardWidget ||
		!IsSafeLiveObjectOfClass(static_cast<UObject*>(Binding.CardWidget), UJHNeoUIGHC_RegionElement_NPC::StaticClass()))
	{
		return;
	}

	Binding.CardWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	Binding.CardWidget->SetRenderTransformPivot(FVector2D{ 0.5f, 0.0f });

	if (Binding.CardWidget->IsA(UJHGHC_RegionEle_NPC_C::StaticClass()))
	{
		auto* RegionNpcCard = static_cast<UJHGHC_RegionEle_NPC_C*>(Binding.CardWidget);
		if (RegionNpcCard->JHGPCBtn_ActiveBG)
		{
			RegionNpcCard->JHGPCBtn_ActiveBG->SetRenderOpacity(0.0f);
			RegionNpcCard->JHGPCBtn_ActiveBG->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RegionNpcCard->NeoUIImageBase_36)
		{
			RegionNpcCard->NeoUIImageBase_36->SetRenderOpacity(0.0f);
			RegionNpcCard->NeoUIImageBase_36->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RegionNpcCard->NeoUIImageBase_44)
		{
			RegionNpcCard->NeoUIImageBase_44->SetRenderOpacity(0.0f);
			RegionNpcCard->NeoUIImageBase_44->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Binding.CardWidget->TXT_Title)
	{
		const wchar_t* Title = Binding.DisplayName.empty() ? L"未命名NPC" : Binding.DisplayName.c_str();
		Binding.CardWidget->TXT_Title->SetText(MakeText(Title));
		Binding.CardWidget->TXT_Title->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (Binding.CardWidget->IMG_Locked)
	{
		Binding.CardWidget->IMG_Locked->SetRenderOpacity(0.0f);
		Binding.CardWidget->IMG_Locked->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Binding.CardWidget->IMG_DLCMark)
	{
		Binding.CardWidget->IMG_DLCMark->SetVisibility(
			Binding.bShowDlcMark ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Binding.CardWidget->IMG_Main && Binding.IdleFlipbook && !Binding.bVisualsPrimed)
	{
		Binding.CardWidget->IMG_Main->SetFlipbook(Binding.IdleFlipbook);
		Binding.CardWidget->IMG_Main->ResetAnimation();
		Binding.CardWidget->IMG_Main->PlayAnimation();
		Binding.CardWidget->IMG_Main->SetRenderOpacity(1.0f);
		Binding.bVisualsPrimed = true;
	}
	else if (Binding.CardWidget->IMG_Main && !Binding.IdleFlipbook)
	{
		Binding.CardWidget->IMG_Main->SetRenderOpacity(0.0f);
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

void RefreshTeammateNpcPrototypePage()
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
			FrameBorder->SetPadding(FMargin{ 3.0f, 3.0f, 3.0f, 3.0f });
			FrameBorder->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
			FrameBorder->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
			FrameBorder->SetBrushColor(GetTeammateNpcPrototypeFrameColor(false));
			FrameBorder->SetContentColorAndOpacity(FLinearColor{ 1.0f, 1.0f, 1.0f, 1.0f });
			CardHost->SetWidthOverride(kTeammateNpcCardHostWidth);
			CardHost->SetHeightOverride(kTeammateNpcCardHostHeight);
			CardHost->AddChild(Card);
			ClickButton->AddChild(CardHost);
			FrameBorder->AddChild(ClickButton);

			if (UWrapBoxSlot* HostSlot = GTeammateNpcPrototypeCardWrap->AddChildToWrapBox(FrameBorder))
			{
				FMargin Pad{};
				Pad.Right = 0.0f;
				Pad.Bottom = 8.0f;
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
	if (!HasTeammateNpcPrototypeCardBinding(GTeammateNpcPrototypeSelectedNpcId) &&
		!GTeammateNpcPrototypeCards.empty())
	{
		GTeammateNpcPrototypeSelectedNpcId = GTeammateNpcPrototypeCards.front().NpcId;
	}
	ApplyTeammateNpcPrototypeSelectionVisuals();
}

} // end anonymous namespace

void PollTab6NpcPrototypeSelection(bool bTab6Active)
{
	if (!bTab6Active)
	{
		for (auto& Binding : GTeammateNpcPrototypeCards)
			Binding.WasPressed = false;
		GTeammateNpcPrototypeLastSelectionPollTick = 0;
		GTeammateNpcPrototypeLastPagerPollTick = 0;
		GTeammateNpcPrototypePrevWasPressed = false;
		GTeammateNpcPrototypeNextWasPressed = false;
		return;
	}

	const ULONGLONG Now = GetTickCount64();
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
		}

		Binding.WasPressed = bPressed;
	}

	if (bSelectionChanged)
		ApplyTeammateNpcPrototypeSelectionVisuals();
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

	auto* TeamPanel = CreateCollapsiblePanel(PC, L"队伍设置");
	auto* TeamBox = TeamPanel ? TeamPanel->CT_Contents : nullptr;
	GTeammate.FollowToggle = CreateToggleItem(PC, L"设置队友跟随数量");
	if (GTeammate.FollowToggle)
	{
		if (TeamBox) TeamBox->AddChild(GTeammate.FollowToggle);
		else GDynTab.Content6->AddChild(GTeammate.FollowToggle);
		Count++;
	}
	GTeammate.FollowCount = CreateVolumeNumericEditBoxItem(PC, Outer, TeamBox ? TeamBox : GDynTab.Content6, L"跟随数量", L"输入数字", L"3");
	if (GTeammate.FollowCount)
	{
		if (TeamBox) TeamBox->AddChild(GTeammate.FollowCount);
		else GDynTab.Content6->AddChild(GTeammate.FollowCount);
		Count++;
	}
	AddPanelWithFixedGap(TeamPanel, 0.0f, 10.0f);

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

	auto* PrototypePanel = CreateCollapsiblePanel(PC, L"NPC图鉴卡片原型");
	auto* PrototypeBox = PrototypePanel ? PrototypePanel->CT_Contents : nullptr;
	if (PrototypeBox)
	{
		auto* TipLabel = CreateRawTextLabel(
			Outer,
			L"分页原型：复用原版图鉴 NPC 卡片，支持搜索 + 上一页/下一页；点击只做高亮选择，不直接入队。");
		if (TipLabel)
		{
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(TipLabel));
			TipLabel->Font.Size = 15;
			TipLabel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (UPanelSlot* LabelSlot = PrototypeBox->AddChild(TipLabel))
			{
				if (LabelSlot->IsA(UVerticalBoxSlot::StaticClass()))
				{
					auto* VSlot = static_cast<UVerticalBoxSlot*>(LabelSlot);
					FMargin Pad{};
					Pad.Bottom = 10.0f;
					VSlot->SetPadding(Pad);
				}
			}
			Count++;
		}

		auto* SelectionLabel = CreateRawTextLabel(
			Outer,
			L"当前选择：未选择  |  点击卡片只做选择高亮。");
		if (SelectionLabel)
		{
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(SelectionLabel));
			SelectionLabel->Font.Size = 16;
			SelectionLabel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			GTeammateNpcPrototypeSelectionLabel = SelectionLabel;
			if (UPanelSlot* LabelSlot = PrototypeBox->AddChild(SelectionLabel))
			{
				if (LabelSlot->IsA(UVerticalBoxSlot::StaticClass()))
				{
					auto* VSlot = static_cast<UVerticalBoxSlot*>(LabelSlot);
					FMargin Pad{};
					Pad.Bottom = 12.0f;
					VSlot->SetPadding(Pad);
				}
			}
			Count++;
		}

		UEditableTextBox* SearchEdit = nullptr;
		if (false && SearchEdit)
		{
			RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(SearchEdit));
			SearchEdit->SetHintText(MakeText(L"输入姓名或NPCId搜索..."));
			SearchEdit->SetText(MakeText(L""));
			SearchEdit->SetJustification(ETextJustify::Left);
			SearchEdit->MinimumDesiredWidth = 380.0f;
			SearchEdit->SelectAllTextWhenFocused = true;
			SearchEdit->ClearKeyboardFocusOnCommit = false;
			SearchEdit->Font.Size = 16;
			SearchEdit->WidgetStyle.Font.Size = 16;
			SearchEdit->WidgetStyle.Padding.Left = 10.0f;
			SearchEdit->WidgetStyle.Padding.Top = 3.0f;
			SearchEdit->WidgetStyle.Padding.Right = 10.0f;
			SearchEdit->WidgetStyle.Padding.Bottom = 3.0f;
			ClearEditableTextBindings(SearchEdit);

			UWidget* SearchHost = SearchEdit;
			auto* SearchSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
			if (SearchSize)
			{
				RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(SearchSize));
				SearchSize->SetWidthOverride(420.0f);
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
					FMargin Pad{};
					Pad.Bottom = 10.0f;
					VSlot->SetPadding(Pad);
				}
			}
			Count++;
		}

		const wchar_t* SearchRowTitle = L"NPC卡片搜索";
		auto* SearchRow = CreateVolumeEditBoxItem(
			PC,
			Outer,
			PrototypeBox,
			SearchRowTitle,
			L"输入姓名或NPCId搜索...",
			L"");
		if (SearchRow)
		{
			if (SearchRow->TXT_Title)
			{
				SearchRow->TXT_Title->SetText(MakeText(L""));
				SearchRow->TXT_Title->SetVisibility(ESlateVisibility::Collapsed);
			}

			auto* SearchEdit2 = GetRuntimeEditBoxByTitle(SearchRowTitle);
			if (SearchEdit2 &&
				IsSafeLiveObjectOfClass(static_cast<UObject*>(SearchEdit2), UEditableTextBox::StaticClass()))
			{
				SearchEdit2->SetHintText(MakeText(L"输入姓名或NPCId搜索..."));
				SearchEdit2->SetJustification(ETextJustify::Left);
				SearchEdit2->MinimumDesiredWidth = 460.0f;
				SearchEdit2->SelectAllTextWhenFocused = true;
				SearchEdit2->ClearKeyboardFocusOnCommit = false;
				SearchEdit2->Font.Size = 16;
				SearchEdit2->WidgetStyle.Font.Size = 16;
				SearchEdit2->WidgetStyle.Padding.Left = 10.0f;
				SearchEdit2->WidgetStyle.Padding.Top = 4.0f;
				SearchEdit2->WidgetStyle.Padding.Right = 10.0f;
				SearchEdit2->WidgetStyle.Padding.Bottom = 4.0f;
				SearchEdit2->SetRenderTranslation(FVector2D{ 0.0f, -0.5f });
				GTeammateNpcPrototypeSearchEdit = SearchEdit2;
			}

			if (UPanelSlot* SearchSlot = PrototypeBox->AddChild(SearchRow))
			{
				if (SearchSlot->IsA(UVerticalBoxSlot::StaticClass()))
				{
					auto* VSlot = static_cast<UVerticalBoxSlot*>(SearchSlot);
					VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
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
			CardWrap->SetInnerSlotPadding(FVector2D{ 8.0f, 12.0f });
			CardWrap->WrapSize = 624.0f;
			CardWrap->bExplicitWrapSize = true;

			bool bMountedListContainer = false;
			if (CardViewport)
			{
				RememberTeammateNpcPrototypeWidget(static_cast<UObject*>(CardViewport));
				CardViewport->SetHeightOverride(470.0f);
				CardViewport->SetContent(CardWrap);
				if (UPanelSlot* RowSlot = PrototypeBox->AddChild(CardViewport))
				{
					if (RowSlot->IsA(UVerticalBoxSlot::StaticClass()))
					{
						auto* VSlot = static_cast<UVerticalBoxSlot*>(RowSlot);
						FMargin Pad{};
						Pad.Bottom = 8.0f;
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
						FMargin Pad{};
						Pad.Bottom = 8.0f;
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
						VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
						VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
						FMargin Pad{};
						Pad.Top = 6.0f;
						VSlot->SetPadding(Pad);
					}
				}
				Count++;
			}

			int32 BuiltCards = 0;
			int32 TotalSamples = 0;
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
	int32& FollowerCount = *reinterpret_cast<int32*>(HeroPtr + 0x0518);

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
	*reinterpret_cast<int32*>(HeroPtr + 0x0518) = GOriginalFollowerCount;
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
