namespace
{
	enum class ETab0Field : uint8
	{
		Money,
		SkillExp,
		JingMaiPoint,
		GuildHonor,
		InheritPoint,
		FishingLevelInt,
		AttrLevel,
		AttrHealth,
		AttrMaxHealth,
		AttrMana,
		AttrMaxMana,
		AttrJingLi,
		AttrMaxJingLi,
		AttrStrength,
		AttrConstitution,
		AttrAgility,
		AttrNeiLi,
		AttrAttackPower,
		AttrDefense,
		AttrCrit,
		AttrCritResistance,
		AttrDodge,
		AttrAccuracy,
		AttrWorldMoveSpeed,
		AttrTurnRate,
		AttrMagicShield,
		AttrHealthShield1,
		AttrHonor,
		AttrCritDamagePercent,
		AttrHealthRestoreRate,
		AttrHealthReGenRate,
		AttrManaRestoreRate,
		AttrManaReGeneRate,
		AttrBoxingLevel,
		AttrBoxingExp,
		AttrFencingLevel,
		AttrFencingExp,
		AttrSabreLevel,
		AttrSabreExp,
		AttrSpearLevel,
		AttrSpearExp,
		AttrHiddenWeaponsLevel,
		AttrHiddenWeaponsExp,
		AttrOtherWeaponsLevel,
		AttrOtherWeaponsExp
	};

	// ── 静态数据表：统一 ETab0Field 的所有映射关系 ──

	using FAttrDataResolver = FGameplayAttributeData* (*)(UJHAttributeSet*);

	struct FTab0FieldMeta
	{
		ETab0Field Field;
		const char* Tag;            // 调试字符串
		const wchar_t* Title;       // 中文 UI 标题
		const wchar_t* AttrName;    // UE4 属性名
		FAttrDataResolver Resolver; // AttrSet 成员解析器
		bool bInteger;              // 是否为整数字段
	};

#define TAB0_ATTR(field, tag, title, attrName, member) \
	{ ETab0Field::field, tag, L##title, L##attrName, \
	  [](UJHAttributeSet* a) -> FGameplayAttributeData* { return &a->member; }, true }

#define TAB0_NON_ATTR(field, tag, title) \
	{ ETab0Field::field, tag, L##title, nullptr, nullptr, true }

	static const FTab0FieldMeta kTab0FieldMap[] =
	{
		TAB0_NON_ATTR(Money,            "Money",              "金钱"),
		TAB0_NON_ATTR(SkillExp,         "SkillExp",           "武学点"),
		TAB0_NON_ATTR(JingMaiPoint,     "JingMaiPoint",       "经脉点"),
		TAB0_NON_ATTR(GuildHonor,       "GuildHonor",         "门派贡献"),
		TAB0_NON_ATTR(InheritPoint,     "InheritPoint",       "继承点"),
		{ ETab0Field::FishingLevelInt,  "FishingLevelInt",    L"钓鱼等级",   L"FishingLevel", nullptr, true },
		TAB0_ATTR(AttrLevel,              "AttrLevel",              "等级",            "Level",              Level),
		TAB0_ATTR(AttrHealth,             "AttrHealth",             "气血",            "Health",             Health),
		TAB0_ATTR(AttrMaxHealth,          "AttrMaxHealth",          "气血上限",        "MaxHealth",          MaxHealth),
		TAB0_ATTR(AttrMana,               "AttrMana",               "真气",            "Mana",               Mana),
		TAB0_ATTR(AttrMaxMana,            "AttrMaxMana",            "真气上限",        "MaxMana",            MaxMana),
		TAB0_ATTR(AttrJingLi,             "AttrJingLi",             "精力",            "JingLi",             JingLi),
		TAB0_ATTR(AttrMaxJingLi,          "AttrMaxJingLi",          "精力上限",        "MaxJingLi",          MaxJingLi),
		TAB0_ATTR(AttrStrength,           "AttrStrength",           "力道",            "Strength",           Strength),
		TAB0_ATTR(AttrConstitution,       "AttrConstitution",       "根骨",            "Constitution",       Constitution),
		TAB0_ATTR(AttrAgility,            "AttrAgility",            "身法",            "Agility",            Agility),
		TAB0_ATTR(AttrNeiLi,              "AttrNeiLi",              "内功",            "NeiLi",              NeiLi),
		TAB0_ATTR(AttrAttackPower,        "AttrAttackPower",        "攻击",            "AttackPower",        AttackPower),
		TAB0_ATTR(AttrDefense,            "AttrDefense",            "防御",            "Defense",            Defense),
		TAB0_ATTR(AttrCrit,               "AttrCrit",               "暴击",            "Crit",               Crit),
		TAB0_ATTR(AttrCritResistance,     "AttrCritResistance",     "暴击抗性",        "CritResistance",     CritResistance),
		TAB0_ATTR(AttrDodge,              "AttrDodge",              "闪避",            "Dodge",              Dodge),
		TAB0_ATTR(AttrAccuracy,           "AttrAccuracy",           "命中",            "Accuracy",           Accuracy),
		TAB0_ATTR(AttrWorldMoveSpeed,     "AttrWorldMoveSpeed",     "移动",            "WorldMoveSpeed",     WorldMoveSpeed),
		TAB0_ATTR(AttrTurnRate,           "AttrTurnRate",           "聚气速率",        "TurnRate",           TurnRate),
		TAB0_ATTR(AttrMagicShield,        "AttrMagicShield",        "真气护盾",        "MagicShield",        MagicShield),
		TAB0_ATTR(AttrHealthShield1,      "AttrHealthShield1",      "气血护盾",        "HealthShield1",      HealthShield1),
		TAB0_ATTR(AttrHonor,              "AttrHonor",              "名声",            "Honor",              Honor),
		TAB0_ATTR(AttrCritDamagePercent,  "AttrCritDamagePercent",  "暴击伤害百分比",  "CritDamagePercent",  CritDamagePercent),
		TAB0_ATTR(AttrHealthRestoreRate,  "AttrHealthRestoreRate",  "气血恢复速率1",   "HealthRestoreRate",  HealthRestoreRate),
		TAB0_ATTR(AttrHealthReGenRate,    "AttrHealthReGenRate",    "气血恢复速率2",   "HealthReGenRate",    HealthReGenRate),
		TAB0_ATTR(AttrManaRestoreRate,    "AttrManaRestoreRate",    "真气恢复速率1",   "ManaRestoreRate",    ManaRestoreRate),
		TAB0_ATTR(AttrManaReGeneRate,     "AttrManaReGeneRate",     "真气恢复速率2",   "ManaReGeneRate",     ManaReGeneRate),
		TAB0_ATTR(AttrBoxingLevel,        "AttrBoxingLevel",        "拳掌精通",        "BoxingLevel",        BoxingLevel),
		TAB0_ATTR(AttrBoxingExp,          "AttrBoxingExp",          "拳掌经验",        "BoxingExp",          BoxingExp),
		TAB0_ATTR(AttrFencingLevel,       "AttrFencingLevel",       "剑法精通",        "FencingLevel",       FencingLevel),
		TAB0_ATTR(AttrFencingExp,         "AttrFencingExp",         "剑法经验",        "FencingExp",         FencingExp),
		TAB0_ATTR(AttrSabreLevel,         "AttrSabreLevel",         "刀法精通",        "SabreLevel",         SabreLevel),
		TAB0_ATTR(AttrSabreExp,           "AttrSabreExp",           "刀法经验",        "SabreExp",           SabreExp),
		TAB0_ATTR(AttrSpearLevel,         "AttrSpearLevel",         "枪棍精通",        "SpearLevel",         SpearLevel),
		TAB0_ATTR(AttrSpearExp,           "AttrSpearExp",           "枪棍经验",        "SpearExp",           SpearExp),
		TAB0_ATTR(AttrHiddenWeaponsLevel, "AttrHiddenWeaponsLevel", "暗器精通",        "HiddenWeaponsLevel", HiddenWeaponsLevel),
		TAB0_ATTR(AttrHiddenWeaponsExp,   "AttrHiddenWeaponsExp",   "暗器经验",        "HiddenWeaponsExp",   HiddenWeaponsExp),
		TAB0_ATTR(AttrOtherWeaponsLevel,  "AttrOtherWeaponsLevel",  "其他武器精通",    "OtherWeaponsLevel",  OtherWeaponsLevel),
		TAB0_ATTR(AttrOtherWeaponsExp,    "AttrOtherWeaponsExp",    "其他武器经验",    "OtherWeaponsExp",    OtherWeaponsExp),
	};

#undef TAB0_ATTR
#undef TAB0_NON_ATTR

	static constexpr size_t kTab0FieldMapSize = sizeof(kTab0FieldMap) / sizeof(kTab0FieldMap[0]);

	const FTab0FieldMeta* FindTab0FieldMeta(ETab0Field Field)
	{
		for (size_t i = 0; i < kTab0FieldMapSize; ++i)
			if (kTab0FieldMap[i].Field == Field)
				return &kTab0FieldMap[i];
		return nullptr;
	}


	struct FTab0Binding final
	{
		ETab0Field Field;
		UEditableTextBox* Edit;
		const wchar_t* Title;
		bool bInteger;
	};

	struct FTab0HeroContext final
	{
		int32 NPCId = -1;
		UTeamInfo* TeamInfo = nullptr;
		UJHAttributeSet* AttrSet = nullptr;
		UItemManager* ItemManager = nullptr;
		UNPCManager* NPCManager = nullptr;
	};

	std::vector<FTab0Binding> GTab0Bindings;
	bool GTab0EnterWasDown = false;
	UEditableTextBox* GTab0LastFocusedEdit = nullptr;

	// 焦点缓存 (方案2: 减少每帧 HasKeyboardFocus 调用)
	constexpr ULONGLONG kTab0FocusCacheDurationMs = 200ULL;  // 缓存有效期 200ms
	UEditableTextBox* GTab0FocusCacheEdit = nullptr;         // 缓存的焦点编辑框
	ULONGLONG GTab0FocusCacheTick = 0;                       // 缓存时间戳

	// 清理降频: 每500ms运行一次清理
	constexpr ULONGLONG kTab0CleanupIntervalMs = 500ULL;
	ULONGLONG GTab0LastCleanupTick = 0;

	// 滑块和下拉框轮询降频: 每100ms运行一次
	constexpr ULONGLONG kTab0UiPollIntervalMs = 100ULL;
	ULONGLONG GTab0LastUiPollTick = 0;

	// 角色选择下拉框
	UBPVE_JHConfigVideoItem2_C* GTab0RoleSelectDD = nullptr;
	std::vector<int32> GTab0RoleSelectNPCIds;      // 对应每个选项的 NPCId
	std::vector<std::wstring> GTab0RoleSelectNames; // 角色名字
	int32 GTab0SelectedRoleIdx = -1;                // 当前选中的角色索引
	int32 GTab0LastSelectedRoleIdx = -1;            // 上一次选中的角色索引

	constexpr bool kTab0VerboseLog = false;
	UBPVE_JHConfigVolumeItem2_C* GTab0MoneyMultiplierItem = nullptr;
	UBPVE_JHConfigVolumeItem2_C* GTab0SkillExpMultiplierItem = nullptr;
	UBPVE_JHConfigVolumeItem2_C* GTab0ManaCostMultiplierItem = nullptr;
	UBPVE_JHConfigVolumeItem2_C* GTab0EscapeSuccrateItem = nullptr;
	UBPVE_JHConfigVideoItem2_C* GTab0ExtraNeiGongLimitDD = nullptr;
	UBPVE_JHConfigVideoItem2_C* GTab0GuildDD = nullptr;
	int32 GTab0ExtraNeiGongLimitLastIdx = -1;
	int32 GTab0GuildLastIdx = -1;
	std::vector<int32> GTab0GuildOptionGuildIds;
	std::vector<std::wstring> GTab0GuildOptionLabels;
	bool GTab0GuildOptionsResolvedFromTable = false;
	ULONGLONG GTab0GuildLastRebuildTick = 0;
	float GTab0MoneyMultiplierLastPercent = -1.0f;
	float GTab0SkillExpMultiplierLastPercent = -1.0f;
	float GTab0ManaCostMultiplierLastPercent = -1.0f;
	float GTab0EscapeSuccrateLastPercent = -1.0f;
	bool GTab0MoneyMinusWasPressed = false;
	bool GTab0MoneyPlusWasPressed = false;
	bool GTab0SkillExpMinusWasPressed = false;
	bool GTab0SkillExpPlusWasPressed = false;
	bool GTab0ManaCostMinusWasPressed = false;
	bool GTab0ManaCostPlusWasPressed = false;
	bool GTab0EscapeSuccrateMinusWasPressed = false;
	bool GTab0EscapeSuccratePlusWasPressed = false;


	const char* Tab0FieldToString(ETab0Field Field)
	{
		const FTab0FieldMeta* Meta = FindTab0FieldMeta(Field);
		return Meta ? Meta->Tag : "Unknown";
	}

	int32 RoundToInt(double Value)
	{
		return static_cast<int32>(std::llround(Value));
	}

	std::wstring FormatNumericText(double Value, bool bInteger)
	{
		wchar_t Buf[64] = {};
		if (bInteger)
		{
			swprintf_s(Buf, 64, L"%d", RoundToInt(Value));
			return Buf;
		}

		swprintf_s(Buf, 64, L"%.3f", Value);
		std::wstring Out = Buf;
		while (!Out.empty() && Out.back() == L'0')
			Out.pop_back();
		if (!Out.empty() && Out.back() == L'.')
			Out.pop_back();
		if (Out.empty())
			Out = L"0";
		return Out;
	}

	std::string SanitizeNumericInputText(const std::string& Raw, bool bInteger)
	{
		std::string Normalized = Raw;
		std::replace(Normalized.begin(), Normalized.end(), ',', '.');

		std::string Out;
		Out.reserve(Normalized.size());
		bool bHasDot = false;
		bool bHasSign = false;
		for (char Ch : Normalized)
		{
			const unsigned char UCh = static_cast<unsigned char>(Ch);
			if (std::isdigit(UCh))
			{
				Out.push_back(Ch);
				continue;
			}
			if (Ch == '-' && !bHasSign && Out.empty())
			{
				Out.push_back(Ch);
				bHasSign = true;
				continue;
			}
			if (!bInteger && Ch == '.' && !bHasDot)
			{
				Out.push_back(Ch);
				bHasDot = true;
				continue;
			}
		}
		return Out;
	}

	std::wstring AsciiToWide(const std::string& Text)
	{
		return std::wstring(Text.begin(), Text.end());
	}

	UEditableTextBox* FindFirstEditableTextBox(UWidget* Root)
	{
		if (!Root)
			return nullptr;

		if (Root->IsA(UEditableTextBox::StaticClass()))
			return static_cast<UEditableTextBox*>(Root);

		// UUserWidget 不是 PanelWidget，先下钻到 WidgetTree.RootWidget 再递归
		if (Root->IsA(UUserWidget::StaticClass()))
		{
			auto* UserWidget = static_cast<UUserWidget*>(Root);
			if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
				return FindFirstEditableTextBox(UserWidget->WidgetTree->RootWidget);
			return nullptr;
		}

		if (!Root->IsA(UPanelWidget::StaticClass()))
			return nullptr;

		auto* Panel = static_cast<UPanelWidget*>(Root);
		const int32 Count = Panel->GetChildrenCount();
		for (int32 i = 0; i < Count; ++i)
		{
			if (UWidget* Child = Panel->GetChildAt(i))
			{
				if (UEditableTextBox* Edit = FindFirstEditableTextBox(Child))
					return Edit;
			}
		}
		return nullptr;
	}

	bool ResolveTab0FieldByTitle(const wchar_t* Title, ETab0Field* OutField, bool* OutInteger)
	{
		if (!Title || !OutField || !OutInteger)
			return false;

		for (size_t i = 0; i < kTab0FieldMapSize; ++i)
		{
			if (kTab0FieldMap[i].Title && wcscmp(Title, kTab0FieldMap[i].Title) == 0)
			{
				*OutField = kTab0FieldMap[i].Field;
				*OutInteger = kTab0FieldMap[i].bInteger;
				return true;
			}
		}
		return false;
	}

	FGameplayAttributeData* ResolveAttrData(UJHAttributeSet* AttrSet, ETab0Field Field)
	{
		if (!AttrSet)
			return nullptr;

		const FTab0FieldMeta* Meta = FindTab0FieldMeta(Field);
		if (!Meta || !Meta->Resolver)
			return nullptr;
		return Meta->Resolver(AttrSet);
	}

	const wchar_t* ResolveAttrFieldName(ETab0Field Field)
	{
		const FTab0FieldMeta* Meta = FindTab0FieldMeta(Field);
		return Meta ? Meta->AttrName : nullptr;
	}

	bool BuildGameplayAttributeByField(ETab0Field Field, FGameplayAttribute* OutAttr)
	{
		if (!OutAttr)
		{
			return false;
		}

		const wchar_t* AttrName = ResolveAttrFieldName(Field);
		if (!AttrName || !AttrName[0])
		{
			return false;
		}

		FGameplayAttribute Attr{};
		Attr.AttributeName = FString(AttrName);
		Attr.AttributeOwner = static_cast<UStruct*>(UJHAttributeSet::StaticClass());
		*OutAttr = Attr;
		return true;
	}

	bool BuildGameplayAttributeByName(const wchar_t* AttrName, FGameplayAttribute* OutAttr)
	{
		if (!AttrName || !AttrName[0] || !OutAttr)
		{
			return false;
		}
		FGameplayAttribute Attr{};
		Attr.AttributeName = FString(AttrName);
		Attr.AttributeOwner = static_cast<UStruct*>(UJHAttributeSet::StaticClass());
		*OutAttr = Attr;
		return true;
	}

	bool TryGetLiveFloatAttribute(const FTab0HeroContext& Ctx, ETab0Field Field, float* OutValue)
	{
		if (!OutValue || Ctx.NPCId < 0)
		{
			return false;
		}

		FGameplayAttribute Attr{};
		if (!BuildGameplayAttributeByField(Field, &Attr))
		{
			return false;
		}

		bool bFound = false;
		const float Value = UNPCFuncLib::GetFloatAttribute(Ctx.NPCId, Attr, &bFound);
		if (!bFound)
		{
			return false;
		}

		*OutValue = Value;
		return true;
	}

	bool TryAddLiveFloatAttributeDelta(const FTab0HeroContext& Ctx, ETab0Field Field, float DeltaValue)
	{
		if (Ctx.NPCId < 0)
		{
			return false;
		}

		if (std::fabs(DeltaValue) < 0.0001f)
		{
			return true;
		}

		FGameplayAttribute Attr{};
		if (!BuildGameplayAttributeByField(Field, &Attr))
		{
			return false;
		}

		UNPCFuncLib::AddFloatAttribute(Ctx.NPCId, Attr, DeltaValue, false);
		return true;
	}

	// 获取当前队伍所有角色的名字和NPCId列表
	void RefreshTab0RoleSelectOptions()
	{
		GTab0RoleSelectNPCIds.clear();
		GTab0RoleSelectNames.clear();

		UTeamManager* TeamManager = UManagerFuncLib::GetTeamManager();
		if (!TeamManager)
			return;

		LOGI_STREAM("Tab0Character") << "[SDK] RefreshTab0RoleSelectOptions: TeamInfos count=" << TeamManager->TeamInfos.Num()
		          << ", FightTeamInfos count=" << TeamManager->FightTeamInfos.Num() << "\n";

		auto ProcessTeamArray = [&](TArray<UTeamInfo*>& Infos, const char* ArrayName)
		{
			const int32 N = Infos.Num();
			LOGI_STREAM("Tab0Character") << "[SDK] Processing " << ArrayName << ", count=" << N << "\n";
			for (int32 i = 0; i < N; ++i)
			{
				UTeamInfo* Info = Infos[i];
				if (!Info || !IsSafeLiveObject(static_cast<UObject*>(Info)))
					continue;
				LOGI_STREAM("Tab0Character") << "[SDK] " << ArrayName << "[" << i << "]: NPCId=" << Info->NPCId << "\n";

				if (Info->NPCId < 0)
					continue;

				// 去重：检查是否已存在相同 NPCId
				bool bDuplicate = false;
				for (int32 ExistingId : GTab0RoleSelectNPCIds)
				{
					if (ExistingId == Info->NPCId)
					{
						bDuplicate = true;
						break;
					}
				}
				if (bDuplicate)
					continue;

				// 获取角色名字 - 使用和门派一样的方式
				FText Name = UNPCFuncLib::GetNPCNameById(Info->NPCId);
				FString NameStr = UKismetTextLibrary::Conv_TextToString(Name);
				const wchar_t* NameWs = NameStr.CStr();
				std::wstring NameWstr = (NameWs && NameWs[0]) ? std::wstring(NameWs) : std::wstring();

				GTab0RoleSelectNPCIds.push_back(Info->NPCId);
				GTab0RoleSelectNames.push_back(NameWstr);
				LOGI_STREAM("Tab0Character") << L"[SDK] Added role: NPCId=" << Info->NPCId << L", name=" << NameWstr.c_str() << L"\n";
			}
		};

		// 先处理队伍列表，再处理战斗队伍列表
		ProcessTeamArray(TeamManager->TeamInfos, "TeamInfos");
		ProcessTeamArray(TeamManager->FightTeamInfos, "FightTeamInfos");
		LOGI_STREAM("Tab0Character") << "[SDK] Total roles found: " << GTab0RoleSelectNPCIds.size() << "\n";
	}

	// 前向声明
	void RefreshTab0BindingsText(APlayerController* PC);

	FTab0HeroContext BuildTab0HeroContext(APlayerController* PC)
	{
		FTab0HeroContext Ctx{};
		Ctx.ItemManager = UManagerFuncLib::GetItemManager();
		Ctx.NPCManager = UManagerFuncLib::GetNPCManager();

		// 如果用户通过下拉框选择了角色，优先使用选中的 NPCId
		if (GTab0SelectedRoleIdx >= 0 && GTab0SelectedRoleIdx < static_cast<int32>(GTab0RoleSelectNPCIds.size()))
		{
			Ctx.NPCId = GTab0RoleSelectNPCIds[GTab0SelectedRoleIdx];
			if (Ctx.NPCId >= 0)
			{
				Ctx.TeamInfo = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);
				if (Ctx.TeamInfo)
				{
					Ctx.AttrSet = Ctx.TeamInfo->GetAttributeSet();
					if (!Ctx.AttrSet)
						Ctx.AttrSet = Ctx.TeamInfo->AttributeSet;
					return Ctx;
				}
			}
		}

		UObject* WorldContext = nullptr;
		if (PC)
			WorldContext = static_cast<UObject*>(PC);
		else if (UWorld* World = UWorld::GetWorld())
			WorldContext = static_cast<UObject*>(World);

		AActor* HeroActor = WorldContext ? UNPCFuncLib::GetSceneHero(WorldContext) : nullptr;
		if (HeroActor && HeroActor->IsA(AJHCharacter::StaticClass()))
		{
			Ctx.NPCId = static_cast<AJHCharacter*>(HeroActor)->NPCId;
		}

		if (Ctx.NPCId >= 0)
		{
			Ctx.TeamInfo = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);
		}

		if (!Ctx.TeamInfo)
		{
			if (UTeamManager* TeamManager = UManagerFuncLib::GetTeamManager())
			{
				Ctx.TeamInfo = TeamManager->GetInfoByIndex(0);
			}
		}

		if (Ctx.TeamInfo)
		{
			if (Ctx.NPCId < 0)
				Ctx.NPCId = Ctx.TeamInfo->NPCId;
			Ctx.AttrSet = Ctx.TeamInfo->GetAttributeSet();
			if (!Ctx.AttrSet)
				Ctx.AttrSet = Ctx.TeamInfo->AttributeSet;
		}

		// 兜底：部分场景 GetSceneHero/Index0 取到的 NPCId 可能是 0，
		// 尝试从 TeamManager 的队伍列表里找第一个有效 NPCId。
		if (Ctx.NPCId < 0)
		{
			if (UTeamManager* TeamManager = UManagerFuncLib::GetTeamManager())
			{
				auto TryPickFromArray = [&](TArray<UTeamInfo*>& Infos) -> bool
				{
					const int32 N = Infos.Num();
					for (int32 i = 0; i < N; ++i)
					{
						UTeamInfo* Info = Infos[i];
						if (!Info || !IsSafeLiveObject(static_cast<UObject*>(Info)))
							continue;
						if (Info->NPCId >= 0)
						{
							Ctx.TeamInfo = Info;
							Ctx.NPCId = Info->NPCId;
							return true;
						}
					}
					return false;
				};

				if (!TryPickFromArray(TeamManager->TeamInfos))
				{
					TryPickFromArray(TeamManager->FightTeamInfos);
				}
			}

			if (Ctx.TeamInfo && (!Ctx.AttrSet || !IsSafeLiveObject(static_cast<UObject*>(Ctx.AttrSet))))
			{
				Ctx.AttrSet = Ctx.TeamInfo->GetAttributeSet();
				if (!Ctx.AttrSet)
					Ctx.AttrSet = Ctx.TeamInfo->AttributeSet;
			}
		}

		return Ctx;
	}

	UTeamInfo* ResolveLiveTeamInfo(const FTab0HeroContext& Ctx)
	{
		UTeamInfo* LiveTeamInfo = nullptr;
		UTeamInfo* ByNpcId = nullptr;
		if (Ctx.NPCId >= 0)
		{
			ByNpcId = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);
			LiveTeamInfo = ByNpcId;
		}
		if (!LiveTeamInfo)
			LiveTeamInfo = Ctx.TeamInfo;
		return LiveTeamInfo;
	}

	bool IsValidTab0Dropdown(UBPVE_JHConfigVideoItem2_C* Item)
	{
		return Item &&
			IsSafeLiveObject(static_cast<UObject*>(Item)) &&
			!(Item->Flags & EObjectFlags::BeginDestroyed) &&
			!(Item->Flags & EObjectFlags::FinishDestroyed) &&
			Item->CB_Main &&
			IsSafeLiveObject(static_cast<UObject*>(Item->CB_Main)) &&
			!(Item->CB_Main->Flags & EObjectFlags::BeginDestroyed) &&
			!(Item->CB_Main->Flags & EObjectFlags::FinishDestroyed);
	}

	int32 GetComboOptionCountFast(UComboBoxString* Combo)
	{
		if (!Combo ||
			!IsSafeLiveObject(static_cast<UObject*>(Combo)) ||
			(Combo->Flags & EObjectFlags::BeginDestroyed) ||
			(Combo->Flags & EObjectFlags::FinishDestroyed))
		{
			return 0;
		}
		return Combo->GetOptionCount();
	}

	int32 GetComboSelectedIndexFast(UComboBoxString* Combo)
	{
		if (!Combo ||
			!IsSafeLiveObject(static_cast<UObject*>(Combo)) ||
			(Combo->Flags & EObjectFlags::BeginDestroyed) ||
			(Combo->Flags & EObjectFlags::FinishDestroyed))
		{
			return -1;
		}

		const int32 Count = Combo->GetOptionCount();
		if (Count <= 0)
			return -1;

		return Combo->GetSelectedIndex();
	}

	int32 ClampComboIndex(UComboBoxString* Combo, int32 Index)
	{
		if (!Combo)
			return 0;
		const int32 Count = GetComboOptionCountFast(Combo);
		if (Count <= 0)
			return 0;
		if (Index < 0)
			return 0;
		if (Index >= Count)
			return Count - 1;
		return Index;
	}

	int32 ClampTab0ExtraNeiGongLimit(int32 Value)
	{
		if (Value < 0) return 0;
		if (Value > 2) return 2;
		return Value;
	}

	void RebuildTab0GuildOptionsFromGame(APlayerController* PC)
	{
		std::vector<std::pair<int32, std::wstring>> Entries;
		bool bResolvedFromTable = false;
		auto IsSyntheticLabel = [](const std::wstring& Label)
		{
			return Label.empty() || Label.rfind(L"Guild_", 0) == 0;
		};
		auto AddEntry = [&Entries, &IsSyntheticLabel](int32 GuildId, const std::wstring& Label)
		{
			std::wstring FinalLabel = Label;
			if (FinalLabel.empty())
			{
				wchar_t Buf[64] = {};
				swprintf_s(Buf, 64, L"Guild_%d", GuildId);
				FinalLabel = Buf;
			}

			for (auto& It : Entries)
			{
				if (It.first == GuildId)
				{
					if (!IsSyntheticLabel(FinalLabel) && IsSyntheticLabel(It.second))
					{
						It.second = FinalLabel;
					}
					return;
				}
			}

			Entries.emplace_back(GuildId, FinalLabel);
		};

		UGuildResManager* GuildResMgr = UManagerFuncLib::GetGuildResManager();
		UDataTable* GuildTable = GuildResMgr ? GuildResMgr->GuildSettingTable : nullptr;
		LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] RebuildGuildOptions begin mgr=" << (void*)GuildResMgr
		          << " table=" << (void*)GuildTable << "\n";
		if (GuildTable && IsSafeLiveObject(static_cast<UObject*>(GuildTable)))
		{
			auto& RowMap = GuildTable->RowMap;
			const int32 AllocatedSlots = RowMap.IsValid() ? RowMap.NumAllocated() : 0;
			const int32 RowCount = RowMap.IsValid() ? RowMap.Num() : 0;
			LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] GuildTable rowMap valid=" << (RowMap.IsValid() ? 1 : 0)
			          << " rows=" << RowCount << " allocated=" << AllocatedSlots
			          << " rowStruct=" << (void*)GuildTable->RowStruct << "\n";

			if (RowMap.IsValid() && AllocatedSlots > 0 && RowCount > 0)
			{
				for (int32 i = 0; i < AllocatedSlots; ++i)
				{
					if (!RowMap.IsValidIndex(i))
						continue;
					uint8* RowData = RowMap[i].Value();
					if (!RowData)
						continue;

					const int32 GuildId = *reinterpret_cast<int32*>(RowData + 0x08);
					std::wstring Label;
					auto* TextData = *reinterpret_cast<FTextImpl::FTextData**>(RowData + 0x10);
					if (TextData)
					{
						const wchar_t* WStr = TextData->TextSource.CStr();
						if (WStr && WStr[0])
							Label.assign(WStr);
					}
					AddEntry(GuildId, Label);
					bResolvedFromTable = true;
				}
			}

			bool bNeedFallbackResolve = Entries.empty();
			if (!bNeedFallbackResolve)
			{
				for (const auto& It : Entries)
				{
					if (IsSyntheticLabel(It.second))
					{
						bNeedFallbackResolve = true;
						break;
					}
				}
			}

			// RowMap 解析到的 FText 可能是占位文本，补走 helper 路径拿本地化后的门派名
			if (bNeedFallbackResolve)
			{
				TArray<FName> RowNames;
				UDataTableFunctionLibrary::GetDataTableRowNames(GuildTable, &RowNames);
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] GuildTable fallback rowNames=" << RowNames.Num() << "\n";
				for (const FName& RowName : RowNames)
				{
					FGuildSetting Row{};
					if (!UDataTableFunctionLibrary::GetDataTableRowFromName(
						GuildTable,
						RowName,
						reinterpret_cast<FTableRowBase*>(&Row)))
					{
						continue;
					}

					FString NameStr = UKismetTextLibrary::Conv_TextToString(Row.Name);
					const wchar_t* NameWs = NameStr.CStr();
					std::wstring Label = (NameWs && NameWs[0]) ? std::wstring(NameWs) : std::wstring();
					AddEntry(Row.ID, Label);
					bResolvedFromTable = true;
				}
			}
		}

		if (Entries.empty())
		{
			FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (LiveTeamInfo && IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
			{
				wchar_t Buf[64] = {};
				swprintf_s(Buf, 64, L"Guild_%d", LiveTeamInfo->GuildId);
				AddEntry(LiveTeamInfo->GuildId, Buf);
			}
		}

		if (Entries.empty())
			AddEntry(0, L"Guild_0");

		std::sort(
			Entries.begin(),
			Entries.end(),
			[](const std::pair<int32, std::wstring>& A, const std::pair<int32, std::wstring>& B)
			{
				return A.first < B.first;
			});

		GTab0GuildOptionGuildIds.clear();
		GTab0GuildOptionLabels.clear();
		GTab0GuildOptionGuildIds.reserve(Entries.size());
		GTab0GuildOptionLabels.reserve(Entries.size());
		for (const auto& It : Entries)
		{
			GTab0GuildOptionGuildIds.push_back(It.first);
			GTab0GuildOptionLabels.push_back(It.second);
		}
		GTab0GuildOptionsResolvedFromTable = bResolvedFromTable;
		LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] RebuildGuildOptions end count=" << GTab0GuildOptionGuildIds.size();
		for (size_t i = 0; i < GTab0GuildOptionGuildIds.size(); ++i)
		{
			LOGI_STREAM("Tab0Character") << L" [" << i << L"]=" << GTab0GuildOptionGuildIds[i]
			           << L":" << GTab0GuildOptionLabels[i].c_str();
		}
		LOGI_STREAM("Tab0Character") << "\n";
	}

	int32 Tab0GuildIdToDropdownIndex(int32 GuildId, UComboBoxString* Combo)
	{
		if (GTab0GuildOptionGuildIds.empty())
			return ClampComboIndex(Combo, 0);

		for (int32 i = 0; i < static_cast<int32>(GTab0GuildOptionGuildIds.size()); ++i)
		{
			if (GTab0GuildOptionGuildIds[i] == GuildId)
				return ClampComboIndex(Combo, i);
		}
		return ClampComboIndex(Combo, 0);
	}

	int32 Tab0DropdownIndexToGuildId(int32 Index)
	{
		if (Index < 0 || Index >= static_cast<int32>(GTab0GuildOptionGuildIds.size()))
			return 0;
		return GTab0GuildOptionGuildIds[Index];
	}

	void RefreshTab0GuildDropdownOptionsIfNeeded(APlayerController* PC, bool bForce)
	{
		if (!IsValidTab0Dropdown(GTab0GuildDD))
			return;

		UComboBoxString* Combo = GTab0GuildDD->CB_Main;
		const int32 CurrentCount = GetComboOptionCountFast(Combo);
		const bool bLooksPlaceholder =
			(CurrentCount <= 1) ||
			(GTab0GuildOptionGuildIds.size() <= 1 && !GTab0GuildOptionsResolvedFromTable);

		const ULONGLONG Now = GetTickCount64();
		if (!bForce)
		{
			if (!bLooksPlaceholder)
				return;
			if (Now - GTab0GuildLastRebuildTick < 1500ULL)
				return;
		}
		GTab0GuildLastRebuildTick = Now;

		int32 PreferredGuildId = 0;
		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
		if (LiveTeamInfo && IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
			PreferredGuildId = LiveTeamInfo->GuildId;

		RebuildTab0GuildOptionsFromGame(PC);
		if (!IsValidTab0Dropdown(GTab0GuildDD))
			return;

		Combo = GTab0GuildDD->CB_Main;
		Combo->ClearOptions();
		for (const auto& Label : GTab0GuildOptionLabels)
			Combo->AddOption(FString(Label.c_str()));

		const int32 TargetIdx = Tab0GuildIdToDropdownIndex(PreferredGuildId, Combo);
		Combo->SetSelectedIndex(TargetIdx);
		GTab0GuildLastIdx = TargetIdx;

		LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] GuildDropdownRefresh force=" << (bForce ? 1 : 0)
		          << " placeholder=" << (bLooksPlaceholder ? 1 : 0)
		          << " count=" << GetComboOptionCountFast(Combo)
		          << " preferredGuildId=" << PreferredGuildId
		          << " selectedIdx=" << TargetIdx << "\n";
	}

	void SyncTab0RoleDropdownsFromLive(APlayerController* PC)
	{
		RefreshTab0GuildDropdownOptionsIfNeeded(PC, false);
		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);

		if (IsValidTab0Dropdown(GTab0ExtraNeiGongLimitDD))
		{
			UComboBoxString* Combo = GTab0ExtraNeiGongLimitDD->CB_Main;
			int32 TargetIdx = 0;
			if (LiveTeamInfo && IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
				TargetIdx = ClampTab0ExtraNeiGongLimit(LiveTeamInfo->AddNeiGongUpLimit);
			TargetIdx = ClampComboIndex(Combo, TargetIdx);
			if (GetComboSelectedIndexFast(Combo) != TargetIdx)
				Combo->SetSelectedIndex(TargetIdx);
			GTab0ExtraNeiGongLimitLastIdx = TargetIdx;
		}

		if (IsValidTab0Dropdown(GTab0GuildDD))
		{
			UComboBoxString* Combo = GTab0GuildDD->CB_Main;
			int32 TargetIdx = 0;
			if (LiveTeamInfo && IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
				TargetIdx = Tab0GuildIdToDropdownIndex(LiveTeamInfo->GuildId, Combo);
			TargetIdx = ClampComboIndex(Combo, TargetIdx);
			if (GetComboSelectedIndexFast(Combo) != TargetIdx)
				Combo->SetSelectedIndex(TargetIdx);
			GTab0GuildLastIdx = TargetIdx;
		}
	}

	bool ApplyTab0ExtraNeiGongLimitSelection(const FTab0HeroContext& Ctx, int32 SelectedIndex)
	{
		UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
		if (!LiveTeamInfo || !IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
			return false;
		const int32 Target = ClampTab0ExtraNeiGongLimit(SelectedIndex);
		LiveTeamInfo->AddNeiGongUpLimit = Target;
		return LiveTeamInfo->AddNeiGongUpLimit == Target;
	}

	bool ApplyTab0GuildSelection(const FTab0HeroContext& Ctx, int32 SelectedIndex)
	{
		const int32 TargetGuildId = Tab0DropdownIndexToGuildId(SelectedIndex);
		if (Ctx.NPCManager && IsSafeLiveObject(static_cast<UObject*>(Ctx.NPCManager)) && Ctx.NPCId >= 0)
			Ctx.NPCManager->ChangeGuildId(Ctx.NPCId, TargetGuildId);

		UTeamInfo* VerifyTeamInfo = ResolveLiveTeamInfo(Ctx);
		if (!VerifyTeamInfo || !IsSafeLiveObject(static_cast<UObject*>(VerifyTeamInfo)))
			return false;
		if (VerifyTeamInfo->GuildId != TargetGuildId)
			VerifyTeamInfo->GuildId = TargetGuildId;
		return VerifyTeamInfo->GuildId == TargetGuildId;
	}

	void PollTab0RoleDropdowns(APlayerController* PC, bool bTab0Active)
	{
		if (!bTab0Active)
			return;

		// 检测角色选择下拉框的变化
		if (GTab0RoleSelectDD && IsValidTab0Dropdown(GTab0RoleSelectDD))
		{
			int32 CurRoleIdx = GetComboSelectedIndexFast(GTab0RoleSelectDD->CB_Main);
			if (CurRoleIdx >= 0 && CurRoleIdx != GTab0LastSelectedRoleIdx)
			{
				GTab0SelectedRoleIdx = CurRoleIdx;
				GTab0LastSelectedRoleIdx = CurRoleIdx;
				// 角色切换时刷新所有数值
				RefreshTab0BindingsText(PC);
				// 重置倍率滑块的值，以便重新读取
				GTab0MoneyMultiplierLastPercent = -1.0f;
				GTab0SkillExpMultiplierLastPercent = -1.0f;
				GTab0ManaCostMultiplierLastPercent = -1.0f;
				// 输出选中的角色信息
				if (CurRoleIdx >= 0 && CurRoleIdx < static_cast<int32>(GTab0RoleSelectNPCIds.size()))
				{
					int32 SelectedNPCId = GTab0RoleSelectNPCIds[CurRoleIdx];
					const std::wstring& SelectedName = GTab0RoleSelectNames[CurRoleIdx];
					LOGI_STREAM("Tab0Character") << L"[SDK] Tab0 Role changed: index=" << CurRoleIdx
					          << L", NPCId=" << SelectedNPCId
					          << L", name=" << SelectedName.c_str() << L"\n";
				}
			}
		}
		else
		{
			// 角色选择下拉框不可用，静默处理
		}

		auto ReadIndex = [](UBPVE_JHConfigVideoItem2_C* Item) -> int32
		{
			if (!IsValidTab0Dropdown(Item))
				return -1;
			return GetComboSelectedIndexFast(Item->CB_Main);
		};

		const int32 CurExtraIdx = ReadIndex(GTab0ExtraNeiGongLimitDD);
		const int32 CurGuildIdx = ReadIndex(GTab0GuildDD);

		if (CurExtraIdx < 0 && CurGuildIdx < 0)
			return;

		RefreshTab0GuildDropdownOptionsIfNeeded(PC, false);

		if (GTab0ExtraNeiGongLimitLastIdx < 0 && CurExtraIdx >= 0)
			GTab0ExtraNeiGongLimitLastIdx = CurExtraIdx;
		if (GTab0GuildLastIdx < 0 && CurGuildIdx >= 0)
			GTab0GuildLastIdx = CurGuildIdx;

		const bool ExtraChanged = (CurExtraIdx >= 0 && CurExtraIdx != GTab0ExtraNeiGongLimitLastIdx);
		const bool GuildChanged = (CurGuildIdx >= 0 && CurGuildIdx != GTab0GuildLastIdx);
		if (!ExtraChanged && !GuildChanged)
			return;

		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		if (ExtraChanged)
		{
			const int32 PrevIdx = GTab0ExtraNeiGongLimitLastIdx;
			const bool Ok = ApplyTab0ExtraNeiGongLimitSelection(Ctx, CurExtraIdx);
			if (Ok)
			{
				GTab0ExtraNeiGongLimitLastIdx = ClampTab0ExtraNeiGongLimit(CurExtraIdx);
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] ExtraNeiGongLimit changed npcId=" << Ctx.NPCId
					<< " prevIdx=" << PrevIdx << " newIdx=" << GTab0ExtraNeiGongLimitLastIdx << "\n";
			}
			else if (IsValidTab0Dropdown(GTab0ExtraNeiGongLimitDD))
			{
				GTab0ExtraNeiGongLimitDD->CB_Main->SetSelectedIndex(
					ClampComboIndex(GTab0ExtraNeiGongLimitDD->CB_Main, GTab0ExtraNeiGongLimitLastIdx));
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] ExtraNeiGongLimit apply failed, rollback idx="
					<< GTab0ExtraNeiGongLimitLastIdx << " npcId=" << Ctx.NPCId << "\n";
			}
		}

		if (GuildChanged)
		{
			const int32 PrevIdx = GTab0GuildLastIdx;
			const bool Ok = ApplyTab0GuildSelection(Ctx, CurGuildIdx);
			if (Ok)
			{
				GTab0GuildLastIdx = CurGuildIdx;
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] Guild changed npcId=" << Ctx.NPCId
					<< " prevIdx=" << PrevIdx << " newIdx=" << GTab0GuildLastIdx << "\n";
			}
			else if (IsValidTab0Dropdown(GTab0GuildDD))
			{
				GTab0GuildDD->CB_Main->SetSelectedIndex(
					ClampComboIndex(GTab0GuildDD->CB_Main, GTab0GuildLastIdx));
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Role] Guild apply failed, rollback idx="
					<< GTab0GuildLastIdx << " npcId=" << Ctx.NPCId << "\n";
			}
		}
	}

	float GetVolumeItemPercent(UBPVE_JHConfigVolumeItem2_C* Item)
	{
		if (!Item || !Item->VolumeSlider || !IsSafeLiveObject(static_cast<UObject*>(Item->VolumeSlider)))
			return -1.0f;

		USlider* Slider = Item->VolumeSlider;
		float CurValue = Slider->GetValue();
		if (CurValue < 0.0f) CurValue = 0.0f;
		if (CurValue > 10.0f) CurValue = 10.0f;
		return CurValue;
	}

	void UpdateVolumeItemPercentText(UBPVE_JHConfigVolumeItem2_C* Item, float Percent)
	{
		if (!Item || !Item->TXT_CurrentValue || !IsSafeLiveObject(static_cast<UObject*>(Item->TXT_CurrentValue)))
			return;

		float Clamped = Percent;
		if (Clamped < 0.0f) Clamped = 0.0f;
		if (Clamped > 10.0f) Clamped = 10.0f;

		wchar_t Buf[32] = {};
		swprintf_s(Buf, 32, L"%.1f", Clamped);  // 显示1位小数
		Item->TXT_CurrentValue->SetText(MakeText(Buf));
	}

	void SetVolumeItemPercent(UBPVE_JHConfigVolumeItem2_C* Item, float Percent)
	{
		if (!Item || !Item->VolumeSlider || !IsSafeLiveObject(static_cast<UObject*>(Item->VolumeSlider)))
			return;
		USlider* Slider = Item->VolumeSlider;
		float ClampedPercent = Percent;
		if (ClampedPercent < 0.0f) ClampedPercent = 0.0f;
		if (ClampedPercent > 10.0f) ClampedPercent = 10.0f;
		Slider->SetValue(ClampedPercent);
		UpdateVolumeItemPercentText(Item, ClampedPercent);
	}

	void RemoveVolumeItemFromGlobalPoll(UBPVE_JHConfigVolumeItem2_C* Item)
	{
		auto It = std::find(GVolumeItems.begin(), GVolumeItems.end(), Item);
		if (It == GVolumeItems.end())
			return;
		const size_t Idx = static_cast<size_t>(std::distance(GVolumeItems.begin(), It));
		GVolumeItems.erase(It);
		if (Idx < GVolumeLastValues.size()) GVolumeLastValues.erase(GVolumeLastValues.begin() + Idx);
		if (Idx < GVolumeMinusWasPressed.size()) GVolumeMinusWasPressed.erase(GVolumeMinusWasPressed.begin() + Idx);
		if (Idx < GVolumePlusWasPressed.size()) GVolumePlusWasPressed.erase(GVolumePlusWasPressed.begin() + Idx);
	}

	void ConfigureTab0RatioSlider(UBPVE_JHConfigVolumeItem2_C* Item)
	{
		if (!Item || !Item->VolumeSlider || !IsSafeLiveObject(static_cast<UObject*>(Item->VolumeSlider)))
			return;
		USlider* Slider = Item->VolumeSlider;
		Slider->MinValue = 0.0f;
		Slider->MaxValue = 10.0f;
		Slider->StepSize = 0.1f;  // 小数步进
		SetVolumeItemPercent(Item, Slider->GetValue());
	}

	bool TryReadMultiplierFallbackFromAttrSet(const FTab0HeroContext& Ctx, const wchar_t* AttrName, float* OutValue)
	{
		if (!Ctx.AttrSet || !AttrName || !OutValue)
		{
			return false;
		}
		if (wcscmp(AttrName, L"MoneyMultiplier") == 0)
		{
			*OutValue = Ctx.AttrSet->MoneyMultiplier.CurrentValue;
			return true;
		}
		if (wcscmp(AttrName, L"SExpMultiplier") == 0)
		{
			*OutValue = Ctx.AttrSet->SExpMultiplier.CurrentValue;
			return true;
		}
		if (wcscmp(AttrName, L"ManaCostMultiplier") == 0)
		{
			*OutValue = Ctx.AttrSet->ManaCostMultiplier.CurrentValue;
			return true;
		}
		if (wcscmp(AttrName, L"EscapeSuccrate") == 0)
		{
			*OutValue = Ctx.AttrSet->EscapeSuccrate.CurrentValue;
			return true;
		}
		return false;
	}

	bool TryWriteMultiplierToAttrSet(const FTab0HeroContext& Ctx, const wchar_t* AttrName, float Target, float* OutAfter)
	{
		if (OutAfter)
			*OutAfter = 0.0f;
		if (!Ctx.AttrSet || !AttrName || !AttrName[0])
			return false;

		FGameplayAttributeData* Data = nullptr;
		if (wcscmp(AttrName, L"MoneyMultiplier") == 0)
			Data = &Ctx.AttrSet->MoneyMultiplier;
		else if (wcscmp(AttrName, L"SExpMultiplier") == 0)
			Data = &Ctx.AttrSet->SExpMultiplier;
		else if (wcscmp(AttrName, L"ManaCostMultiplier") == 0)
			Data = &Ctx.AttrSet->ManaCostMultiplier;
		else if (wcscmp(AttrName, L"EscapeSuccrate") == 0)
			Data = &Ctx.AttrSet->EscapeSuccrate;
		else
			return false;

		Data->BaseValue = Target;
		Data->CurrentValue = Target;
		if (OutAfter)
			*OutAfter = Data->CurrentValue;
		return true;
	}

	bool TryGetTab0MultiplierPercent(const FTab0HeroContext& Ctx, const wchar_t* AttrName, float* OutPercent)
	{
		if (!OutPercent || !AttrName || !AttrName[0])
		{
			return false;
		}

		float RawValue = 0.0f;
		bool bFound = false;

		FGameplayAttribute Attr{};
		if (BuildGameplayAttributeByName(AttrName, &Attr) && Ctx.NPCId >= 0)
		{
			RawValue = UNPCFuncLib::GetFloatAttribute(Ctx.NPCId, Attr, &bFound);
			if (bFound)
			{
				*OutPercent = RawValue;
				return true;
			}
		}

		if (TryReadMultiplierFallbackFromAttrSet(Ctx, AttrName, &RawValue))
		{
			*OutPercent = RawValue;
			return true;
		}
		return false;
	}

	bool ApplyTab0MultiplierAttribute(const FTab0HeroContext& Ctx, const wchar_t* AttrName, float Percent)
	{
		const float Target = Percent;
		float Current = 0.0f;
		const bool HasCurrent = TryReadMultiplierFallbackFromAttrSet(Ctx, AttrName, &Current);
		if (!HasCurrent)
		{
			return false;
		}

		if (std::fabs(Target - Current) <= 0.0001f)
		{
			return true;
		}

		float After = 0.0f;
		if (!TryWriteMultiplierToAttrSet(Ctx, AttrName, Target, &After))
		{
			return false;
		}

		const bool Ok = std::fabs(After - Target) <= 0.0001f;
		return Ok;
	}

	void PollTab0RatioSliders(APlayerController* PC)
	{
		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		if (Ctx.NPCId < 0)
		{
			return;
		}

		auto PollOne = [&](UBPVE_JHConfigVolumeItem2_C* Item, float& LastPercent, const wchar_t* AttrName, bool& MinusWasPressed, bool& PlusWasPressed)
		{
			if (!Item || !IsSafeLiveObject(static_cast<UObject*>(Item)))
			{
				MinusWasPressed = false;
				PlusWasPressed = false;
				return;
			}
			auto* Slider = Item->VolumeSlider;
			if (!Slider || !IsSafeLiveObject(static_cast<UObject*>(Slider)))
			{
				MinusWasPressed = false;
				PlusWasPressed = false;
				return;
			}

			bool MinusPressed = false;
			bool PlusPressed = false;
			if (Item->BTN_Minus && IsSafeLiveObject(static_cast<UObject*>(Item->BTN_Minus)))
				MinusPressed = Item->BTN_Minus->IsPressed();
			if (Item->BTN_Plus && IsSafeLiveObject(static_cast<UObject*>(Item->BTN_Plus)))
				PlusPressed = Item->BTN_Plus->IsPressed();
			const bool MinusClicked = MinusWasPressed && !MinusPressed;
			const bool PlusClicked = PlusWasPressed && !PlusPressed;
			MinusWasPressed = MinusPressed;
			PlusWasPressed = PlusPressed;

			float Percent = GetVolumeItemPercent(Item);
			if (Percent < 0.0f)
			{
				return;
			}

			if (MinusClicked || PlusClicked)
			{
				float Step = Slider->StepSize;
				if (Step <= 0.0001f)
					Step = 1.0f;
				float NewValue = Percent + (PlusClicked ? Step : 0.0f) - (MinusClicked ? Step : 0.0f);
				SetVolumeItemPercent(Item, NewValue);
				Percent = GetVolumeItemPercent(Item);
			}

			UpdateVolumeItemPercentText(Item, Percent);

			if (LastPercent < 0.0f)
			{
				LastPercent = Percent;
				ApplyTab0MultiplierAttribute(Ctx, AttrName, Percent);
				return;
			}

			if (std::fabs(Percent - LastPercent) >= 0.05f)  // 检测小数变化
			{
				ApplyTab0MultiplierAttribute(Ctx, AttrName, Percent);
				LastPercent = Percent;
			}
		};

		PollOne(GTab0MoneyMultiplierItem, GTab0MoneyMultiplierLastPercent, L"MoneyMultiplier", GTab0MoneyMinusWasPressed, GTab0MoneyPlusWasPressed);
		PollOne(GTab0SkillExpMultiplierItem, GTab0SkillExpMultiplierLastPercent, L"SExpMultiplier", GTab0SkillExpMinusWasPressed, GTab0SkillExpPlusWasPressed);
		PollOne(GTab0ManaCostMultiplierItem, GTab0ManaCostMultiplierLastPercent, L"ManaCostMultiplier", GTab0ManaCostMinusWasPressed, GTab0ManaCostPlusWasPressed);
		PollOne(GTab0EscapeSuccrateItem, GTab0EscapeSuccrateLastPercent, L"EscapeSuccrate", GTab0EscapeSuccrateMinusWasPressed, GTab0EscapeSuccratePlusWasPressed);
	}

	bool TryGetTab0FieldValue(const FTab0HeroContext& Ctx, ETab0Field Field, double* OutValue)
	{
		if (!OutValue)
		{
			return false;
		}

		switch (Field)
		{
		case ETab0Field::Money:
			if (!Ctx.ItemManager)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UItemManager::GetMoney fail reason=ItemManagerNull\n";
				return false;
			}
			*OutValue = static_cast<double>(Ctx.ItemManager->GetMoney());
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UItemManager::GetMoney ok value=" << *OutValue << "\n";
			return true;
		case ETab0Field::SkillExp:
		{
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::GetNPCInfoById fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}
			*OutValue = static_cast<double>(LiveTeamInfo->SkillExp);
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UTeamInfo::SkillExp ok value=" << *OutValue
				          << " npcId=" << Ctx.NPCId << "\n";
			return true;
		}
		case ETab0Field::JingMaiPoint:
			if (Ctx.NPCManager && Ctx.NPCId >= 0)
			{
				*OutValue = static_cast<double>(Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId));
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::GetJingMaiPoint ok value=" << *OutValue
					          << " npcId=" << Ctx.NPCId << "\n";
				return true;
			}
			if (UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx))
			{
				*OutValue = static_cast<double>(LiveTeamInfo->JingMaiPoint);
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::JingMaiPoint(fallback) ok value=" << *OutValue
					          << " npcId=" << Ctx.NPCId << "\n";
				return true;
			}
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UNPCManager::GetJingMaiPoint fail reason=NoManagerOrNoTeamInfo npcId=" << Ctx.NPCId << "\n";
			return false;
		case ETab0Field::GuildHonor:
		{
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::GetGuildHonor fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}
			const int32 GuildId = LiveTeamInfo->GuildId;
			if (GuildId <= 0)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::GetGuildHonor fail reason=GuildIdInvalid npcId=" << Ctx.NPCId << "\n";
				return false;
			}
			*OutValue = static_cast<double>(LiveTeamInfo->GetGuildHonor(GuildId));
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UTeamInfo::GetGuildHonor ok value=" << *OutValue
				          << " npcId=" << Ctx.NPCId << " guildId=" << GuildId << "\n";
			return true;
		}
		case ETab0Field::InheritPoint:
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=None fail reason=NotImplemented\n";
			return false;
		case ETab0Field::FishingLevelInt:
		{
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo || !IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::FishingLevel fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}
			*OutValue = static_cast<double>(LiveTeamInfo->FishingLevel);
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UTeamInfo::FishingLevel ok value=" << *OutValue
				          << " npcId=" << Ctx.NPCId << "\n";
			return true;
		}
		default:
			break;
		}

		const wchar_t* AttrName = ResolveAttrFieldName(Field);
		if (AttrName)
		{
			float LiveValue = 0.0f;
			if (Ctx.NPCId >= 0 && TryGetLiveFloatAttribute(Ctx, Field, &LiveValue))
			{
				*OutValue = static_cast<double>(LiveValue);
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::GetFloatAttribute ok value=" << *OutValue
					          << " npcId=" << Ctx.NPCId << "\n";
				return true;
			}

			// 读取兜底：实时属性接口失败时回退 AttributeSet，避免 UI 全部显示 -1。
			if (Ctx.AttrSet)
			{
				if (FGameplayAttributeData* AttrData = ResolveAttrData(Ctx.AttrSet, Field))
				{
					*OutValue = static_cast<double>(AttrData->CurrentValue);
					if (kTab0VerboseLog)
						LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
						          << " sdk=UJHAttributeSet::CurrentValue(fallback) ok value=" << *OutValue
						          << " npcId=" << Ctx.NPCId << "\n";
					return true;
				}
			}
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UNPCFuncLib::GetFloatAttribute fail reason=NoLiveAndNoAttrSetFallback npcId=" << Ctx.NPCId << "\n";
			return false;
		}
		return false;
	}

	bool TrySetTab0FieldValue(const FTab0HeroContext& Ctx, ETab0Field Field, double InValue)
	{
		switch (Field)
		{
		case ETab0Field::Money:
		{
			if (!Ctx.ItemManager)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UItemFuncLib::AddMoney fail reason=ItemManagerNull\n";
				return false;
			}
			const int32 Current = Ctx.ItemManager->GetMoney();
			const int32 Target = RoundToInt(InValue);
			const int32 Delta = Target - Current;
			if (Delta != 0)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UItemFuncLib::AddMoney call delta=" << Delta
					          << " current=" << Current << " target=" << Target << "\n";
				UItemFuncLib::AddMoney(Delta);
			}
			const int32 After = Ctx.ItemManager->GetMoney();
			if (After != Target)
			{
				LOGI_STREAM("Tab0Character") << "[SDK] Tab0CommitMoney: mismatch target=" << Target
				          << " after=" << After << " delta=" << Delta << "\n";
			}
			const bool Ok = (After == Target);
			return Ok;
		}
		case ETab0Field::SkillExp:
		{
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo || !IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=ResolveLiveTeamInfo fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}

			const int32 Target = RoundToInt(InValue);
			const int32 Current = LiveTeamInfo->SkillExp;
			const int32 Delta = Target - Current;
			bool AppliedBySdk = false;
			if (Delta != 0)
			{
				if (Ctx.NPCId >= 0)
				{
					if (kTab0VerboseLog)
						LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
						          << " sdk=USkillFuncLib::AddSkillExp call delta=" << Delta
						          << " current=" << Current << " target=" << Target
						          << " npcId=" << Ctx.NPCId << "\n";
					USkillFuncLib::AddSkillExp(Ctx.NPCId, Delta);
					AppliedBySdk = true;
				}
				else if (kTab0VerboseLog)
				{
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=USkillFuncLib::AddSkillExp skip reason=NpcIdNegative npcId=" << Ctx.NPCId << "\n";
				}
			}

			UTeamInfo* VerifyTeamInfo = nullptr;
			if (Ctx.NPCId >= 0)
				VerifyTeamInfo = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);
			if (!VerifyTeamInfo || !IsSafeLiveObject(static_cast<UObject*>(VerifyTeamInfo)))
				VerifyTeamInfo = ResolveLiveTeamInfo(Ctx);

			if ((!VerifyTeamInfo || VerifyTeamInfo->SkillExp != Target) &&
				LiveTeamInfo && IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
			{
				LiveTeamInfo->SkillExp = Target;
				if (Ctx.NPCId >= 0)
					VerifyTeamInfo = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);
				if (!VerifyTeamInfo || !IsSafeLiveObject(static_cast<UObject*>(VerifyTeamInfo)))
					VerifyTeamInfo = ResolveLiveTeamInfo(Ctx);
			}

			if (!VerifyTeamInfo || VerifyTeamInfo->SkillExp != Target)
			{
				LOGI_STREAM("Tab0Character") << "[SDK] Tab0CommitSkillExp: mismatch npcId=" << Ctx.NPCId
				          << " target=" << Target
				          << " after=" << (VerifyTeamInfo ? VerifyTeamInfo->SkillExp : -1)
				          << " delta=" << Delta
				          << " sdkApplied=" << (AppliedBySdk ? 1 : 0) << "\n";
			}
			const bool Ok = VerifyTeamInfo && (VerifyTeamInfo->SkillExp == Target);
			return Ok;
		}
		case ETab0Field::JingMaiPoint:
		{
			if (Ctx.NPCId < 0 || !Ctx.NPCManager)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::AddJingMaiPoint fail reason=NpcManagerOrNpcIdInvalid npcId=" << Ctx.NPCId << "\n";
				return false;
			}

			const int32 Target = RoundToInt(InValue);
			const int32 Current = Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId);
			const int32 Delta = Target - Current;
			if (Delta != 0)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::AddJingMaiPoint call delta=" << Delta
					          << " current=" << Current << " target=" << Target
					          << " npcId=" << Ctx.NPCId << "\n";
				UNPCFuncLib::AddJingMaiPoint(Ctx.NPCId, Delta, false);
			}

			const int32 AfterDelta = Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId);
			if (AfterDelta != Target)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::SetJingMaiPoint call afterDelta=" << AfterDelta
					          << " target=" << Target << " npcId=" << Ctx.NPCId << "\n";
				Ctx.NPCManager->SetJingMaiPoint(Ctx.NPCId, Target);
			}
			const int32 AfterSet = Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId);
			if (AfterSet != Target)
			{
				LOGI_STREAM("Tab0Character") << "[SDK] Tab0CommitJingMai: mismatch npcId=" << Ctx.NPCId
				          << " target=" << Target << " after=" << AfterSet
				          << " delta=" << Delta << "\n";
			}
			const bool Ok = (AfterSet == Target);
			return Ok;
		}
		case ETab0Field::GuildHonor:
		{
			if (!Ctx.NPCManager)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::ChangeGuildHonor fail reason=NpcManagerNull\n";
				return false;
			}

			const int32 Target = RoundToInt(InValue);
			const int32 EffectiveNPCId = Ctx.NPCId;
			if (EffectiveNPCId < 0)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::ChangeGuildHonor fail reason=NpcIdInvalid\n";
				return false;
			}

			UTeamInfo* LiveTeamInfo = UNPCFuncLib::GetNPCInfoById(EffectiveNPCId);
			if (!LiveTeamInfo)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::GetNPCInfoById fail reason=TeamInfoNull npcId=" << EffectiveNPCId << "\n";
				return false;
			}

			const int32 GuildId = LiveTeamInfo->GuildId;
			if (GuildId <= 0)
			{
				return false;
			}

			const int32 Current = LiveTeamInfo->GetGuildHonor(GuildId);
			const int32 Delta = Target - Current;
			if (Delta != 0)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::ChangeGuildHonor call delta=" << Delta
					          << " current=" << Current << " target=" << Target
					          << " npcId=" << EffectiveNPCId << " guildId=" << GuildId << "\n";
				Ctx.NPCManager->ChangeGuildHonor(EffectiveNPCId, GuildId, Delta);
			}

			UTeamInfo* VerifyTeamInfo = UNPCFuncLib::GetNPCInfoById(EffectiveNPCId);
			if (!VerifyTeamInfo || VerifyTeamInfo->GetGuildHonor(GuildId) != Target)
			{
				const int32 After = VerifyTeamInfo ? VerifyTeamInfo->GetGuildHonor(GuildId) : -1;
				LOGI_STREAM("Tab0Character") << "[SDK] Tab0CommitGuildHonor: mismatch npcId=" << EffectiveNPCId
				          << " guildId=" << GuildId
				          << " target=" << Target << " after=" << After
				          << " delta=" << Delta << "\n";
			}
			const bool Ok = VerifyTeamInfo && (VerifyTeamInfo->GetGuildHonor(GuildId) == Target);
			return Ok;
		}
		case ETab0Field::InheritPoint:
			return false;
		case ETab0Field::FishingLevelInt:
		{
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo || !IsSafeLiveObject(static_cast<UObject*>(LiveTeamInfo)))
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::FishingLevel fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}

			const int32 Target = RoundToInt(InValue);
			LiveTeamInfo->FishingLevel = Target;
			const int32 After = LiveTeamInfo->FishingLevel;
			const bool Ok = (After == Target);
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
				          << " sdk=UTeamInfo::FishingLevel set target=" << Target
				          << " after=" << After << " npcId=" << Ctx.NPCId << "\n";
			return Ok;
		}
		default:
			break;
		}

		const float TargetFloat = static_cast<float>(InValue);
		const wchar_t* AttrName = ResolveAttrFieldName(Field);
		if (AttrName)
		{
			if (Ctx.NPCId < 0)
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::AddFloatAttribute fail reason=NpcIdInvalid\n";
				return false;
			}

			float Current = 0.0f;
			bool HasCurrent = TryGetLiveFloatAttribute(Ctx, Field, &Current);
			bool UsedAttrSetFallback = false;
			FGameplayAttributeData* AttrData = nullptr;
			if (!HasCurrent && Ctx.AttrSet)
			{
				AttrData = ResolveAttrData(Ctx.AttrSet, Field);
				if (AttrData)
				{
					Current = AttrData->CurrentValue;
					HasCurrent = true;
					UsedAttrSetFallback = true;
				}
			}
			if (!HasCurrent)
			{
				LOGI_STREAM("Tab0Character") << "[SDK] Tab0CommitLiveAttr: get current failed, field="
				          << static_cast<int32>(Field) << " npcId=" << Ctx.NPCId << "\n";
				return false;
			}

			const float Delta = TargetFloat - Current;
			bool AppliedLive = false;
			if (std::fabs(Delta) > 0.0001f)
				AppliedLive = TryAddLiveFloatAttributeDelta(Ctx, Field, Delta);
			else
				AppliedLive = true;
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
				          << " sdk=UNPCFuncLib::AddFloatAttribute call delta=" << Delta
				          << " current=" << Current << " target=" << TargetFloat
				          << " npcId=" << Ctx.NPCId << " applied=" << (AppliedLive ? 1 : 0)
				          << " currentSource=" << (UsedAttrSetFallback ? "AttrSet" : "Live") << "\n";

			float LiveAfter = 0.0f;
			bool LiveReadbackOk = TryGetLiveFloatAttribute(Ctx, Field, &LiveAfter);
			if (AppliedLive && LiveReadbackOk && std::fabs(LiveAfter - TargetFloat) <= 0.5f)
			{
				return true;
			}

			if (!AttrData && Ctx.AttrSet)
				AttrData = ResolveAttrData(Ctx.AttrSet, Field);
			if (AttrData)
			{
				AttrData->BaseValue = TargetFloat;
				AttrData->CurrentValue = TargetFloat;
				const float After = AttrData->CurrentValue;
				const bool Ok = std::fabs(After - TargetFloat) <= 0.0001f;
				if (!Ok)
				{
					LOGI_STREAM("Tab0Character") << "[SDK] Tab0CommitLiveAttr: attrset fallback mismatch, field="
					          << static_cast<int32>(Field) << " npcId=" << Ctx.NPCId
					          << " target=" << TargetFloat << " after=" << After << "\n";
				}
				return Ok;
			}

			LOGI_STREAM("Tab0Character") << "[SDK] Tab0CommitLiveAttr: apply/readback failed without attrset fallback, field="
			          << static_cast<int32>(Field) << " npcId=" << Ctx.NPCId
			          << " delta=" << Delta << "\n";
			return false;
		}
		return false;
	}

	void RefreshTab0BindingsText(APlayerController* PC)
	{
		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		for (auto& Binding : GTab0Bindings)
		{
			if (!Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit)))
				continue;

			double Value = 0.0;
			std::wstring Text = L"-1";
			if (TryGetTab0FieldValue(Ctx, Binding.Field, &Value))
				Text = FormatNumericText(Value, Binding.bInteger);
			Binding.Edit->SetText(MakeText(Text.c_str()));
		}
	}

	void RegisterTab0Binding(const wchar_t* Title, UWidget* RowRoot, APlayerController* PC)
	{
		ETab0Field Field{};
		bool bInteger = true;
		if (!ResolveTab0FieldByTitle(Title, &Field, &bInteger))
		{
			return;
		}

		UEditableTextBox* Edit = FindFirstEditableTextBox(RowRoot);
		if (!Edit)
		{
			return;
		}

		auto Existing = std::find_if(
			GTab0Bindings.begin(),
			GTab0Bindings.end(),
			[Edit](const FTab0Binding& B) { return B.Edit == Edit; });
		if (Existing != GTab0Bindings.end())
		{
			return;
		}

		GTab0Bindings.push_back(FTab0Binding{ Field, Edit, Title, bInteger });

		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		double Value = 0.0;
		std::wstring Text = L"-1";
		if (TryGetTab0FieldValue(Ctx, Field, &Value))
			Text = FormatNumericText(Value, bInteger);
		Edit->SetText(MakeText(Text.c_str()));
	}

	void RefreshSingleTab0BindingText(const FTab0Binding& Binding, APlayerController* PC)
	{
		if (!Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit)))
			return;

		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		double Value = 0.0;
		std::wstring Text = L"-1";
		if (TryGetTab0FieldValue(Ctx, Binding.Field, &Value))
			Text = FormatNumericText(Value, Binding.bInteger);
		Binding.Edit->SetText(MakeText(Text.c_str()));
	}

	FTab0Binding* FindTab0BindingByEdit(UEditableTextBox* Edit)
	{
		if (!Edit)
			return nullptr;
		auto It = std::find_if(
			GTab0Bindings.begin(),
			GTab0Bindings.end(),
			[Edit](const FTab0Binding& B) { return B.Edit == Edit; });
		return (It != GTab0Bindings.end()) ? &(*It) : nullptr;
	}

	void DumpTab0InitSnapshot(APlayerController* PC)
	{
		(void)PC;
	}
}



void PopulateTab_Character(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VolumeSlot, "Tab0(VolumeSlot)");
	if (!Container)
	{
		return;
	}

	Container->ClearChildren();
	GTab0Bindings.clear();
	GTab0EnterWasDown = false;
	GTab0LastFocusedEdit = nullptr;
	GTab0FocusCacheEdit = nullptr;       // 重置焦点缓存
	GTab0FocusCacheTick = 0;             // 重置缓存时间戳
	GTab0LastCleanupTick = 0;            // 重置清理时间戳
	GTab0LastUiPollTick = 0;             // 重置UI轮询时间戳
	GTab0RoleSelectDD = nullptr;         // 重置角色选择下拉框
	GTab0RoleSelectNPCIds.clear();        // 清空角色NPCId列表
	GTab0RoleSelectNames.clear();         // 清空角色名字列表
	GTab0SelectedRoleIdx = -1;            // 重置选中的角色索引
	GTab0LastSelectedRoleIdx = -1;       // 重置上一次选中的角色索引
	GTab0MoneyMultiplierItem = nullptr;
	GTab0SkillExpMultiplierItem = nullptr;
	GTab0ManaCostMultiplierItem = nullptr;
	GTab0EscapeSuccrateItem = nullptr;
	GTab0ExtraNeiGongLimitDD = nullptr;
	GTab0GuildDD = nullptr;
	GTab0ExtraNeiGongLimitLastIdx = -1;
	GTab0GuildLastIdx = -1;
	GTab0GuildOptionsResolvedFromTable = false;
	GTab0GuildLastRebuildTick = 0;
	GTab0MoneyMultiplierLastPercent = -1.0f;
	GTab0SkillExpMultiplierLastPercent = -1.0f;
	GTab0ManaCostMultiplierLastPercent = -1.0f;
	GTab0EscapeSuccrateLastPercent = -1.0f;
	GTab0MoneyMinusWasPressed = false;
	GTab0MoneyPlusWasPressed = false;
	GTab0SkillExpMinusWasPressed = false;
	GTab0SkillExpPlusWasPressed = false;
	GTab0ManaCostMinusWasPressed = false;
	GTab0ManaCostPlusWasPressed = false;
	GTab0EscapeSuccrateMinusWasPressed = false;
	GTab0EscapeSuccratePlusWasPressed = false;
	int Count = 0;
	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = Container->AddChild(Panel);
		if (Slot && Slot->IsA(UVerticalBoxSlot::StaticClass()))
		{
			auto* VSlot = static_cast<UVerticalBoxSlot*>(Slot);
			FMargin Pad{};
			Pad.Left = 0.0f;
			Pad.Top = TopGap;
			Pad.Right = 0.0f;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) -> UBPVE_JHConfigVolumeItem2_C* {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
		return Item;
	};

	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) -> UBPVE_JHConfigVolumeItem2_C* {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			RegisterTab0Binding(Title, Item, PC);
			Count++;
		}
		return Item;
	};

	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) -> UBPVE_JHConfigVideoItem2_C* {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
		return Item;
	};

	auto AddDropdownDynamic = [&](UPanelWidget* Box, const wchar_t* Title, const std::vector<std::wstring>& Options) -> UBPVE_JHConfigVideoItem2_C* {
		auto* Item = CreateVideoItem(PC, Title);
		if (!Item)
			return nullptr;
		if (Item->CB_Main)
		{
			Item->CB_Main->ClearOptions();
			for (const auto& Opt : Options)
				Item->CB_Main->AddOption(FString(Opt.c_str()));
			if (GetComboOptionCountFast(Item->CB_Main) > 0)
				Item->CB_Main->SetSelectedIndex(0);
		}
		if (Box) Box->AddChild(Item);
		else Container->AddChild(Item);
		Count++;
		return Item;
	};

	// 角色选择下拉框 - 放在最上方
	RefreshTab0RoleSelectOptions();
	auto* RoleSelectPanel = CreateCollapsiblePanel(PC, L"选择角色");
	auto* RoleSelectBox = RoleSelectPanel ? RoleSelectPanel->CT_Contents : nullptr;
	if (!GTab0RoleSelectNames.empty())
	{
	// 使用 CreateVideoItemWithOptions 的方式创建带选项的下拉框
	GTab0RoleSelectDD = CreateVideoItemWithOptions(PC, L"当前角色", {});
	if (GTab0RoleSelectDD && GTab0RoleSelectDD->CB_Main)
	{
		GTab0RoleSelectDD->CB_Main->ClearOptions();
		for (const auto& Name : GTab0RoleSelectNames)
		{
			// Name 已经是 std::wstring，直接使用
			GTab0RoleSelectDD->CB_Main->AddOption(FString(Name.c_str()));
		}
		if (GetComboOptionCountFast(GTab0RoleSelectDD->CB_Main) > 0)
			GTab0RoleSelectDD->CB_Main->SetSelectedIndex(0);
		GTab0SelectedRoleIdx = 0;
		GTab0LastSelectedRoleIdx = 0;
		LOGI_STREAM("Tab0Character") << "[SDK] Created role dropdown with " << GTab0RoleSelectNames.size() << " options\n";
	}
	if (RoleSelectBox)
		RoleSelectBox->AddChild(GTab0RoleSelectDD);
	else if (GTab0RoleSelectDD)
		Container->AddChild(GTab0RoleSelectDD);
	if (GTab0RoleSelectDD)
		Count++;
	}
	AddPanelWithFixedGap(RoleSelectPanel, 0.0f, 10.0f);

	// 角色选项面板 - 放在基础数值之前
	auto* RolePanel = CreateCollapsiblePanel(PC, L"角色选项");
	auto* RoleBox = RolePanel ? RolePanel->CT_Contents : nullptr;
	GTab0ExtraNeiGongLimitDD = AddDropdown(RoleBox, L"额外心法栏", { L"0", L"1", L"2" });
	RebuildTab0GuildOptionsFromGame(PC);
	GTab0GuildDD = AddDropdownDynamic(RoleBox, L"门派", GTab0GuildOptionLabels);
	RefreshTab0GuildDropdownOptionsIfNeeded(PC, true);
	AddPanelWithFixedGap(RolePanel, 0.0f, 10.0f);

	auto* BasePanel = CreateCollapsiblePanel(PC, L"基础数值");
	auto* BaseBox = BasePanel ? BasePanel->CT_Contents : nullptr;
	AddNumeric(BaseBox, L"金钱", L"99999");
	AddNumeric(BaseBox, L"武学点", L"100");
	AddNumeric(BaseBox, L"经脉点", L"100");
	AddNumeric(BaseBox, L"门派贡献", L"100");
	AddNumeric(BaseBox, L"继承点", L"100");
	AddNumeric(BaseBox, L"等级", L"10");
	AddNumeric(BaseBox, L"钓鱼等级", L"10");
	AddPanelWithFixedGap(BasePanel, 0.0f, 10.0f);

	auto* AttrPanel = CreateCollapsiblePanel(PC, L"基础属性");
	auto* AttrBox = AttrPanel ? AttrPanel->CT_Contents : nullptr;
	AddNumeric(AttrBox, L"气血", L"100");
	AddNumeric(AttrBox, L"气血上限", L"100");
	AddNumeric(AttrBox, L"真气", L"100");
	AddNumeric(AttrBox, L"真气上限", L"100");
	AddNumeric(AttrBox, L"精力", L"100");
	AddNumeric(AttrBox, L"精力上限", L"100");
	AddNumeric(AttrBox, L"力道", L"100");
	AddNumeric(AttrBox, L"根骨", L"100");
	AddNumeric(AttrBox, L"身法", L"100");
	AddNumeric(AttrBox, L"内功", L"100");
	AddNumeric(AttrBox, L"攻击", L"100");
	AddNumeric(AttrBox, L"防御", L"100");
	AddNumeric(AttrBox, L"暴击", L"100");
	AddNumeric(AttrBox, L"暴击抗性", L"100");
	AddNumeric(AttrBox, L"闪避", L"100");
	AddNumeric(AttrBox, L"命中", L"100");
	AddNumeric(AttrBox, L"移动", L"100");
	AddNumeric(AttrBox, L"聚气速率", L"100");
	AddNumeric(AttrBox, L"真气护盾", L"100");
	AddNumeric(AttrBox, L"气血护盾", L"100");
	AddNumeric(AttrBox, L"名声", L"100");
	AddNumeric(AttrBox, L"暴击伤害百分比", L"100");
	AddPanelWithFixedGap(AttrPanel, 0.0f, 10.0f);

	auto* RecoverPanel = CreateCollapsiblePanel(PC, L"恢复速率");
	auto* RecoverBox = RecoverPanel ? RecoverPanel->CT_Contents : nullptr;
	AddNumeric(RecoverBox, L"气血恢复速率1", L"100");
	AddNumeric(RecoverBox, L"气血恢复速率2", L"100");
	AddNumeric(RecoverBox, L"真气恢复速率1", L"100");
	AddNumeric(RecoverBox, L"真气恢复速率2", L"100");
	AddPanelWithFixedGap(RecoverPanel, 0.0f, 10.0f);

	auto* RatioPanel = CreateCollapsiblePanel(PC, L"倍率与消耗");
	auto* RatioBox = RatioPanel ? RatioPanel->CT_Contents : nullptr;
	GTab0MoneyMultiplierItem = AddSlider(RatioBox, L"金钱倍率");
	GTab0SkillExpMultiplierItem = AddSlider(RatioBox, L"武学点倍率");
	GTab0ManaCostMultiplierItem = AddSlider(RatioBox, L"真气消耗倍率");
	GTab0EscapeSuccrateItem = AddSlider(RatioBox, L"逃跑成功率");
	ConfigureTab0RatioSlider(GTab0MoneyMultiplierItem);
	ConfigureTab0RatioSlider(GTab0SkillExpMultiplierItem);
	ConfigureTab0RatioSlider(GTab0ManaCostMultiplierItem);
	ConfigureTab0RatioSlider(GTab0EscapeSuccrateItem);
	RemoveVolumeItemFromGlobalPoll(GTab0MoneyMultiplierItem);
	RemoveVolumeItemFromGlobalPoll(GTab0SkillExpMultiplierItem);
	RemoveVolumeItemFromGlobalPoll(GTab0ManaCostMultiplierItem);
	RemoveVolumeItemFromGlobalPoll(GTab0EscapeSuccrateItem);
	{
		FTab0HeroContext RatioCtx = BuildTab0HeroContext(PC);
			float Pct = 1.0f;

		if (TryGetTab0MultiplierPercent(RatioCtx, L"MoneyMultiplier", &Pct))
			SetVolumeItemPercent(GTab0MoneyMultiplierItem, Pct);
		else
				SetVolumeItemPercent(GTab0MoneyMultiplierItem, 1.0f);
		GTab0MoneyMultiplierLastPercent = GetVolumeItemPercent(GTab0MoneyMultiplierItem);

			Pct = 1.0f;
		if (TryGetTab0MultiplierPercent(RatioCtx, L"SExpMultiplier", &Pct))
			SetVolumeItemPercent(GTab0SkillExpMultiplierItem, Pct);
		else
				SetVolumeItemPercent(GTab0SkillExpMultiplierItem, 1.0f);
		GTab0SkillExpMultiplierLastPercent = GetVolumeItemPercent(GTab0SkillExpMultiplierItem);

			Pct = 1.0f;
		if (TryGetTab0MultiplierPercent(RatioCtx, L"ManaCostMultiplier", &Pct))
			SetVolumeItemPercent(GTab0ManaCostMultiplierItem, Pct);
		else
				SetVolumeItemPercent(GTab0ManaCostMultiplierItem, 1.0f);
		GTab0ManaCostMultiplierLastPercent = GetVolumeItemPercent(GTab0ManaCostMultiplierItem);

			Pct = 1.0f;
		if (TryGetTab0MultiplierPercent(RatioCtx, L"EscapeSuccrate", &Pct))
			SetVolumeItemPercent(GTab0EscapeSuccrateItem, Pct);
		else
				SetVolumeItemPercent(GTab0EscapeSuccrateItem, 1.0f);
		GTab0EscapeSuccrateLastPercent = GetVolumeItemPercent(GTab0EscapeSuccrateItem);
	}
	AddPanelWithFixedGap(RatioPanel, 0.0f, 10.0f);

	auto* WeaponPanel = CreateCollapsiblePanel(PC, L"武学精通与经验");
	auto* WeaponBox = WeaponPanel ? WeaponPanel->CT_Contents : nullptr;
	AddNumeric(WeaponBox, L"拳掌精通", L"100");
	AddNumeric(WeaponBox, L"拳掌经验", L"100");
	AddNumeric(WeaponBox, L"剑法精通", L"100");
	AddNumeric(WeaponBox, L"剑法经验", L"100");
	AddNumeric(WeaponBox, L"刀法精通", L"100");
	AddNumeric(WeaponBox, L"刀法经验", L"100");
	AddNumeric(WeaponBox, L"枪棍精通", L"100");
	AddNumeric(WeaponBox, L"枪棍经验", L"100");
	AddNumeric(WeaponBox, L"暗器精通", L"100");
	AddNumeric(WeaponBox, L"暗器经验", L"100");
	AddNumeric(WeaponBox, L"其他武器精通", L"100");
	AddNumeric(WeaponBox, L"其他武器经验", L"100");
	AddPanelWithFixedGap(WeaponPanel, 0.0f, 8.0f);
	SyncTab0RoleDropdownsFromLive(PC);
	RefreshTab0BindingsText(PC);
	DumpTab0InitSnapshot(PC);
}

void PollTab0CharacterInput(bool bTab0Active)
{

	// 方案1: 清理降频 - 每500ms运行一次清理，而不是每帧都运行
	const ULONGLONG Now = GetTickCount64();
	const bool bDoCleanup = !GTab0LastCleanupTick || (Now - GTab0LastCleanupTick) >= kTab0CleanupIntervalMs;
	if (bDoCleanup)
	{
		GTab0Bindings.erase(
			std::remove_if(
				GTab0Bindings.begin(),
				GTab0Bindings.end(),
				[](const FTab0Binding& Binding)
				{
					return !Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit));
				}),
			GTab0Bindings.end());

		if (GTab0LastFocusedEdit &&
			!IsSafeLiveObject(static_cast<UObject*>(GTab0LastFocusedEdit)))
		{
			GTab0LastFocusedEdit = nullptr;
		}
		// 同时清理焦点缓存中无效的条目
		if (GTab0FocusCacheEdit && !IsSafeLiveObject(static_cast<UObject*>(GTab0FocusCacheEdit)))
		{
			GTab0FocusCacheEdit = nullptr;
		}
		GTab0LastCleanupTick = Now;
	}

	const bool EnterDown = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
	const bool EnterTriggered = EnterDown && !GTab0EnterWasDown;
	if (!bTab0Active)
	{
		PollTab0RoleDropdowns(nullptr, false);
		GTab0EnterWasDown = EnterDown;
		GTab0LastFocusedEdit = nullptr;
		GTab0FocusCacheEdit = nullptr;  // Tab 不活跃时清空焦点缓存
		return;
	}

	APlayerController* PC = nullptr;
	if (UWorld* World = UWorld::GetWorld())
		PC = UGameplayStatics::GetPlayerController(World, 0);

	// 降频轮询: 每100ms运行一次下拉框和滑块轮询
	const bool bDoUiPoll = !GTab0LastUiPollTick || (Now - GTab0LastUiPollTick) >= kTab0UiPollIntervalMs;
	if (bDoUiPoll)
	{
		// 下拉框轮询
		PollTab0RoleDropdowns(PC, true);
		// Tab0 三个倍率滑块：实时同步到游戏属性
		PollTab0RatioSliders(PC);
		GTab0LastUiPollTick = Now;
	}

	// 方案2: 焦点缓存 - 只有在 EnterTriggered 或缓存过期时才扫描焦点
	// 计算缓存是否过期
	const bool bCacheExpired = !GTab0FocusCacheTick || (Now - GTab0FocusCacheTick) >= kTab0FocusCacheDurationMs;
	// 检查缓存的编辑框是否仍然有效
	const bool bCachedEditValid = GTab0FocusCacheEdit && IsSafeLiveObject(static_cast<UObject*>(GTab0FocusCacheEdit));
	// 是否需要进行焦点扫描: EnterTriggered 时必须扫描，或者缓存已过期，或者缓存的编辑框已失效
	const bool bNeedFocusScan = EnterTriggered || bCacheExpired || !bCachedEditValid;

	FTab0Binding* Focused = nullptr;
	if (bNeedFocusScan)
	{
		// 执行真正的焦点扫描
		for (auto& Binding : GTab0Bindings)
		{
			if (!Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit)))
				continue;
			if (Binding.Edit->HasKeyboardFocus())
			{
				Focused = &Binding;
				break;
			}
		}
		// 更新缓存
		GTab0FocusCacheEdit = Focused ? Focused->Edit : nullptr;
		GTab0FocusCacheTick = Now;
	}
	else
	{
		// 使用缓存的结果
		Focused = FindTab0BindingByEdit(GTab0FocusCacheEdit);
	}

	// 某些编辑框按下回车瞬间会丢失键盘焦点，回车提交时回退到“上一次有焦点的编辑框”。
	if (EnterTriggered && !Focused && GTab0LastFocusedEdit)
	{
		Focused = FindTab0BindingByEdit(GTab0LastFocusedEdit);
		if (kTab0VerboseLog && Focused)
			LOGI_STREAM("Tab0Character") << "[SDK][Tab0Input] EnterTriggered fallbackFocus field="
			          << Tab0FieldToString(Focused->Field) << "\n";
	}

	if (Focused && Focused->Edit)
	{
		const std::string RawFocused = Focused->Edit->GetText().ToString();
		const std::string SanitizedFocused = SanitizeNumericInputText(RawFocused, Focused->bInteger);
		if (SanitizedFocused != RawFocused)
		{
			const std::wstring Wide = AsciiToWide(SanitizedFocused);
			Focused->Edit->SetText(MakeText(Wide.c_str()));
		}
	}

	if (!EnterTriggered &&
		GTab0LastFocusedEdit &&
		(!Focused || Focused->Edit != GTab0LastFocusedEdit))
	{
		FTab0Binding* LastFocusedBinding = FindTab0BindingByEdit(GTab0LastFocusedEdit);
		if (LastFocusedBinding)
		{
			RefreshSingleTab0BindingText(*LastFocusedBinding, PC);
		}
	}

	if (EnterTriggered)
	{
		if (kTab0VerboseLog && !Focused)
			LOGI_STREAM("Tab0Character") << "[SDK][Tab0Input] EnterTriggered but no focused editable binding\n";

		if (Focused && Focused->Edit &&
			IsSafeLiveObject(static_cast<UObject*>(Focused->Edit)))
		{
			std::string Raw = Focused->Edit->GetText().ToString();
			Raw = SanitizeNumericInputText(Raw, Focused->bInteger);
			const std::wstring Wide = AsciiToWide(Raw);
			Focused->Edit->SetText(MakeText(Wide.c_str()));

			char* EndPtr = nullptr;
			const double Parsed = std::strtod(Raw.c_str(), &EndPtr);
			const bool bParsedAll = (EndPtr && *EndPtr == '\0');
			if (kTab0VerboseLog)
				LOGI_STREAM("Tab0Character") << "[SDK][Tab0Input] EnterTriggered field=" << Tab0FieldToString(Focused->Field)
				          << " raw=" << Raw << " parsed=" << Parsed
				          << " parsedAll=" << (bParsedAll ? 1 : 0) << "\n";
			if (EndPtr != Raw.c_str() && bParsedAll)
			{
				FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
				if (TrySetTab0FieldValue(Ctx, Focused->Field, Parsed))
				{
					RefreshTab0BindingsText(PC);
					LOGI_STREAM("Tab0Character") << "[SDK] Tab0Commit: " << (Focused->Title ? "ok" : "unnamed")
					          << " raw=" << Raw << "\n";
				}
				else
				{
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Input] Commit failed field=" << Tab0FieldToString(Focused->Field)
					          << " raw=" << Raw << "\n";
				}
			}
			else
			{
				if (kTab0VerboseLog)
					LOGI_STREAM("Tab0Character") << "[SDK][Tab0Input] Invalid numeric input, rollback field="
					          << Tab0FieldToString(Focused->Field) << " raw=" << Raw << "\n";
				RefreshSingleTab0BindingText(*Focused, PC);
			}
		}
	}

	GTab0LastFocusedEdit = Focused ? Focused->Edit : nullptr;
	GTab0EnterWasDown = EnterDown;
}
