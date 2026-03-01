#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <cctype>
#include "TabContent.hpp"
#include "GCManager.hpp"
#include "ItemBrowser.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"
#include "SDK/BPEntry_Item_classes.hpp"
UPanelWidget* GetOrCreateSlotContainer(UBPMV_ConfigView2_C* CV, UNeoUINamedSlot* Slot, const char* SlotName)
{
	if (!Slot)
	{
		std::cout << "[SDK] " << SlotName << ": slot pointer is null\n";
		return nullptr;
	}

	
	
	
	int childCount = Slot->GetChildrenCount();
	std::cout << "[SDK] " << SlotName << ": ptr=" << (void*)Slot << " children=" << childCount << "\n";
	while (Slot->GetChildrenCount() > 0)
	{
		UWidget* Child = Slot->GetChildAt(0);
		if (Child)
		{
			
			if (Slot == CV->LanSlot && !GOriginalLanPanel)
			{
				GOriginalLanPanel = Child;
				MarkAsGCRoot(GOriginalLanPanel);
				std::cout << "[SDK] Captured original Lan panel: " << (void*)Child << "\n";
			}
			else if (Slot == CV->InputSlot && !GOriginalInputMappingPanel)
			{
				GOriginalInputMappingPanel = Child;
				MarkAsGCRoot(GOriginalInputMappingPanel);
				std::cout << "[SDK] Captured original InputMapping panel: " << (void*)Child << "\n";
			}

			std::cout << "[SDK] " << SlotName << ": removing game panel " << (void*)Child << "\n";
			Child->RemoveFromParent();
		}
		else
			break;
	}

	
	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);



	std::cout << "[SDK] " << SlotName << ": creating UVerticalBox (WidgetTree=" << (void*)WidgetTree << ")\n";

	auto* VBox = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (!VBox)
	{
		std::cout << "[SDK] " << SlotName << ": failed to create UVerticalBox\n";
		return nullptr;
	}

	Slot->AddChild(VBox);
	std::cout << "[SDK] " << SlotName << ": UVerticalBox created and added to slot\n";
	return VBox;
}

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
	constexpr bool kTab0VerboseLog = true;
	UBPVE_JHConfigVolumeItem2_C* GTab0MoneyMultiplierItem = nullptr;
	UBPVE_JHConfigVolumeItem2_C* GTab0SkillExpMultiplierItem = nullptr;
	UBPVE_JHConfigVolumeItem2_C* GTab0ManaCostMultiplierItem = nullptr;
	float GTab0MoneyMultiplierLastPercent = -1.0f;
	float GTab0SkillExpMultiplierLastPercent = -1.0f;
	float GTab0ManaCostMultiplierLastPercent = -1.0f;

	const char* Tab0FieldToString(ETab0Field Field)
	{
		switch (Field)
		{
		case ETab0Field::Money: return "Money";
		case ETab0Field::SkillExp: return "SkillExp";
		case ETab0Field::JingMaiPoint: return "JingMaiPoint";
		case ETab0Field::GuildHonor: return "GuildHonor";
		case ETab0Field::InheritPoint: return "InheritPoint";
		case ETab0Field::FishingLevelInt: return "FishingLevelInt";
		case ETab0Field::AttrLevel: return "AttrLevel";
		case ETab0Field::AttrHealth: return "AttrHealth";
		case ETab0Field::AttrMaxHealth: return "AttrMaxHealth";
		case ETab0Field::AttrMana: return "AttrMana";
		case ETab0Field::AttrMaxMana: return "AttrMaxMana";
		case ETab0Field::AttrJingLi: return "AttrJingLi";
		case ETab0Field::AttrMaxJingLi: return "AttrMaxJingLi";
		case ETab0Field::AttrStrength: return "AttrStrength";
		case ETab0Field::AttrConstitution: return "AttrConstitution";
		case ETab0Field::AttrAgility: return "AttrAgility";
		case ETab0Field::AttrNeiLi: return "AttrNeiLi";
		case ETab0Field::AttrAttackPower: return "AttrAttackPower";
		case ETab0Field::AttrDefense: return "AttrDefense";
		case ETab0Field::AttrCrit: return "AttrCrit";
		case ETab0Field::AttrCritResistance: return "AttrCritResistance";
		case ETab0Field::AttrDodge: return "AttrDodge";
		case ETab0Field::AttrAccuracy: return "AttrAccuracy";
		case ETab0Field::AttrWorldMoveSpeed: return "AttrWorldMoveSpeed";
		case ETab0Field::AttrTurnRate: return "AttrTurnRate";
		case ETab0Field::AttrMagicShield: return "AttrMagicShield";
		case ETab0Field::AttrHealthShield1: return "AttrHealthShield1";
		case ETab0Field::AttrHonor: return "AttrHonor";
		case ETab0Field::AttrCritDamagePercent: return "AttrCritDamagePercent";
		case ETab0Field::AttrHealthRestoreRate: return "AttrHealthRestoreRate";
		case ETab0Field::AttrHealthReGenRate: return "AttrHealthReGenRate";
		case ETab0Field::AttrManaRestoreRate: return "AttrManaRestoreRate";
		case ETab0Field::AttrManaReGeneRate: return "AttrManaReGeneRate";
		case ETab0Field::AttrBoxingLevel: return "AttrBoxingLevel";
		case ETab0Field::AttrBoxingExp: return "AttrBoxingExp";
		case ETab0Field::AttrFencingLevel: return "AttrFencingLevel";
		case ETab0Field::AttrFencingExp: return "AttrFencingExp";
		case ETab0Field::AttrSabreLevel: return "AttrSabreLevel";
		case ETab0Field::AttrSabreExp: return "AttrSabreExp";
		case ETab0Field::AttrSpearLevel: return "AttrSpearLevel";
		case ETab0Field::AttrSpearExp: return "AttrSpearExp";
		case ETab0Field::AttrHiddenWeaponsLevel: return "AttrHiddenWeaponsLevel";
		case ETab0Field::AttrHiddenWeaponsExp: return "AttrHiddenWeaponsExp";
		case ETab0Field::AttrOtherWeaponsLevel: return "AttrOtherWeaponsLevel";
		case ETab0Field::AttrOtherWeaponsExp: return "AttrOtherWeaponsExp";
		default: return "Unknown";
		}
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

		*OutInteger = true;

		if (wcscmp(Title, L"金钱") == 0) { *OutField = ETab0Field::Money; return true; }
		if (wcscmp(Title, L"武学点") == 0) { *OutField = ETab0Field::SkillExp; return true; }
		if (wcscmp(Title, L"经脉点") == 0) { *OutField = ETab0Field::JingMaiPoint; return true; }
		if (wcscmp(Title, L"门派贡献") == 0) { *OutField = ETab0Field::GuildHonor; return true; }
		if (wcscmp(Title, L"继承点") == 0) { *OutField = ETab0Field::InheritPoint; return true; }
		if (wcscmp(Title, L"等级") == 0) { *OutField = ETab0Field::AttrLevel; return true; }
		if (wcscmp(Title, L"钓鱼等级") == 0) { *OutField = ETab0Field::FishingLevelInt; return true; }
		if (wcscmp(Title, L"气血") == 0) { *OutField = ETab0Field::AttrHealth; return true; }
		if (wcscmp(Title, L"气血上限") == 0) { *OutField = ETab0Field::AttrMaxHealth; return true; }
		if (wcscmp(Title, L"真气") == 0) { *OutField = ETab0Field::AttrMana; return true; }
		if (wcscmp(Title, L"真气上限") == 0) { *OutField = ETab0Field::AttrMaxMana; return true; }
		if (wcscmp(Title, L"精力") == 0) { *OutField = ETab0Field::AttrJingLi; return true; }
		if (wcscmp(Title, L"精力上限") == 0) { *OutField = ETab0Field::AttrMaxJingLi; return true; }
		if (wcscmp(Title, L"力道") == 0) { *OutField = ETab0Field::AttrStrength; return true; }
		if (wcscmp(Title, L"根骨") == 0) { *OutField = ETab0Field::AttrConstitution; return true; }
		if (wcscmp(Title, L"身法") == 0) { *OutField = ETab0Field::AttrAgility; return true; }
		if (wcscmp(Title, L"内功") == 0) { *OutField = ETab0Field::AttrNeiLi; return true; }
		if (wcscmp(Title, L"攻击") == 0) { *OutField = ETab0Field::AttrAttackPower; return true; }
		if (wcscmp(Title, L"防御") == 0) { *OutField = ETab0Field::AttrDefense; return true; }
		if (wcscmp(Title, L"暴击") == 0) { *OutField = ETab0Field::AttrCrit; return true; }
		if (wcscmp(Title, L"暴击抗性") == 0) { *OutField = ETab0Field::AttrCritResistance; return true; }
		if (wcscmp(Title, L"闪避") == 0) { *OutField = ETab0Field::AttrDodge; return true; }
		if (wcscmp(Title, L"命中") == 0) { *OutField = ETab0Field::AttrAccuracy; return true; }
		if (wcscmp(Title, L"移动") == 0) { *OutField = ETab0Field::AttrWorldMoveSpeed; return true; }
		if (wcscmp(Title, L"聚气速率") == 0) { *OutField = ETab0Field::AttrTurnRate; return true; }
		if (wcscmp(Title, L"真气护盾") == 0) { *OutField = ETab0Field::AttrMagicShield; return true; }
		if (wcscmp(Title, L"气血护盾") == 0) { *OutField = ETab0Field::AttrHealthShield1; return true; }
		if (wcscmp(Title, L"名声") == 0) { *OutField = ETab0Field::AttrHonor; return true; }
		if (wcscmp(Title, L"暴击伤害百分比") == 0) { *OutField = ETab0Field::AttrCritDamagePercent; return true; }
		if (wcscmp(Title, L"气血恢复速率1") == 0) { *OutField = ETab0Field::AttrHealthRestoreRate; return true; }
		if (wcscmp(Title, L"气血恢复速率2") == 0) { *OutField = ETab0Field::AttrHealthReGenRate; return true; }
		if (wcscmp(Title, L"真气恢复速率1") == 0) { *OutField = ETab0Field::AttrManaRestoreRate; return true; }
		if (wcscmp(Title, L"真气恢复速率2") == 0) { *OutField = ETab0Field::AttrManaReGeneRate; return true; }
		if (wcscmp(Title, L"拳掌精通") == 0) { *OutField = ETab0Field::AttrBoxingLevel; return true; }
		if (wcscmp(Title, L"拳掌经验") == 0) { *OutField = ETab0Field::AttrBoxingExp; return true; }
		if (wcscmp(Title, L"剑法精通") == 0) { *OutField = ETab0Field::AttrFencingLevel; return true; }
		if (wcscmp(Title, L"剑法经验") == 0) { *OutField = ETab0Field::AttrFencingExp; return true; }
		if (wcscmp(Title, L"刀法精通") == 0) { *OutField = ETab0Field::AttrSabreLevel; return true; }
		if (wcscmp(Title, L"刀法经验") == 0) { *OutField = ETab0Field::AttrSabreExp; return true; }
		if (wcscmp(Title, L"枪棍精通") == 0) { *OutField = ETab0Field::AttrSpearLevel; return true; }
		if (wcscmp(Title, L"枪棍经验") == 0) { *OutField = ETab0Field::AttrSpearExp; return true; }
		if (wcscmp(Title, L"暗器精通") == 0) { *OutField = ETab0Field::AttrHiddenWeaponsLevel; return true; }
		if (wcscmp(Title, L"暗器经验") == 0) { *OutField = ETab0Field::AttrHiddenWeaponsExp; return true; }
		if (wcscmp(Title, L"其他武器精通") == 0) { *OutField = ETab0Field::AttrOtherWeaponsLevel; return true; }
		if (wcscmp(Title, L"其他武器经验") == 0) { *OutField = ETab0Field::AttrOtherWeaponsExp; return true; }
		return false;
	}

	FGameplayAttributeData* ResolveAttrData(UJHAttributeSet* AttrSet, ETab0Field Field)
	{
		if (!AttrSet)
			return nullptr;

		switch (Field)
		{
		case ETab0Field::AttrLevel: return &AttrSet->Level;
		case ETab0Field::AttrHealth: return &AttrSet->Health;
		case ETab0Field::AttrMaxHealth: return &AttrSet->MaxHealth;
		case ETab0Field::AttrMana: return &AttrSet->Mana;
		case ETab0Field::AttrMaxMana: return &AttrSet->MaxMana;
		case ETab0Field::AttrJingLi: return &AttrSet->JingLi;
		case ETab0Field::AttrMaxJingLi: return &AttrSet->MaxJingLi;
		case ETab0Field::AttrStrength: return &AttrSet->Strength;
		case ETab0Field::AttrConstitution: return &AttrSet->Constitution;
		case ETab0Field::AttrAgility: return &AttrSet->Agility;
		case ETab0Field::AttrNeiLi: return &AttrSet->NeiLi;
		case ETab0Field::AttrAttackPower: return &AttrSet->AttackPower;
		case ETab0Field::AttrDefense: return &AttrSet->Defense;
		case ETab0Field::AttrCrit: return &AttrSet->Crit;
		case ETab0Field::AttrCritResistance: return &AttrSet->CritResistance;
		case ETab0Field::AttrDodge: return &AttrSet->Dodge;
		case ETab0Field::AttrAccuracy: return &AttrSet->Accuracy;
		case ETab0Field::AttrWorldMoveSpeed: return &AttrSet->WorldMoveSpeed;
		case ETab0Field::AttrTurnRate: return &AttrSet->TurnRate;
		case ETab0Field::AttrMagicShield: return &AttrSet->MagicShield;
		case ETab0Field::AttrHealthShield1: return &AttrSet->HealthShield1;
		case ETab0Field::AttrHonor: return &AttrSet->Honor;
		case ETab0Field::AttrCritDamagePercent: return &AttrSet->CritDamagePercent;
		case ETab0Field::AttrHealthRestoreRate: return &AttrSet->HealthRestoreRate;
		case ETab0Field::AttrHealthReGenRate: return &AttrSet->HealthReGenRate;
		case ETab0Field::AttrManaRestoreRate: return &AttrSet->ManaRestoreRate;
		case ETab0Field::AttrManaReGeneRate: return &AttrSet->ManaReGeneRate;
		case ETab0Field::AttrBoxingLevel: return &AttrSet->BoxingLevel;
		case ETab0Field::AttrBoxingExp: return &AttrSet->BoxingExp;
		case ETab0Field::AttrFencingLevel: return &AttrSet->FencingLevel;
		case ETab0Field::AttrFencingExp: return &AttrSet->FencingExp;
		case ETab0Field::AttrSabreLevel: return &AttrSet->SabreLevel;
		case ETab0Field::AttrSabreExp: return &AttrSet->SabreExp;
		case ETab0Field::AttrSpearLevel: return &AttrSet->SpearLevel;
		case ETab0Field::AttrSpearExp: return &AttrSet->SpearExp;
		case ETab0Field::AttrHiddenWeaponsLevel: return &AttrSet->HiddenWeaponsLevel;
		case ETab0Field::AttrHiddenWeaponsExp: return &AttrSet->HiddenWeaponsExp;
		case ETab0Field::AttrOtherWeaponsLevel: return &AttrSet->OtherWeaponsLevel;
		case ETab0Field::AttrOtherWeaponsExp: return &AttrSet->OtherWeaponsExp;
		default:
			return nullptr;
		}
	}

	const wchar_t* ResolveAttrFieldName(ETab0Field Field)
	{
		switch (Field)
		{
		case ETab0Field::AttrLevel: return L"Level";
		case ETab0Field::AttrHealth: return L"Health";
		case ETab0Field::AttrMaxHealth: return L"MaxHealth";
		case ETab0Field::AttrMana: return L"Mana";
		case ETab0Field::AttrMaxMana: return L"MaxMana";
		case ETab0Field::AttrJingLi: return L"JingLi";
		case ETab0Field::AttrMaxJingLi: return L"MaxJingLi";
		case ETab0Field::AttrStrength: return L"Strength";
		case ETab0Field::AttrConstitution: return L"Constitution";
		case ETab0Field::AttrAgility: return L"Agility";
		case ETab0Field::AttrNeiLi: return L"NeiLi";
		case ETab0Field::AttrAttackPower: return L"AttackPower";
		case ETab0Field::AttrDefense: return L"Defense";
		case ETab0Field::AttrCrit: return L"Crit";
		case ETab0Field::AttrCritResistance: return L"CritResistance";
		case ETab0Field::AttrDodge: return L"Dodge";
		case ETab0Field::AttrAccuracy: return L"Accuracy";
		case ETab0Field::AttrWorldMoveSpeed: return L"WorldMoveSpeed";
		case ETab0Field::AttrTurnRate: return L"TurnRate";
		case ETab0Field::AttrMagicShield: return L"MagicShield";
		case ETab0Field::AttrHealthShield1: return L"HealthShield1";
		case ETab0Field::AttrHonor: return L"Honor";
		case ETab0Field::AttrCritDamagePercent: return L"CritDamagePercent";
		case ETab0Field::AttrHealthRestoreRate: return L"HealthRestoreRate";
		case ETab0Field::AttrHealthReGenRate: return L"HealthReGenRate";
		case ETab0Field::AttrManaRestoreRate: return L"ManaRestoreRate";
		case ETab0Field::AttrManaReGeneRate: return L"ManaReGeneRate";
		case ETab0Field::AttrBoxingLevel: return L"BoxingLevel";
		case ETab0Field::AttrBoxingExp: return L"BoxingExp";
		case ETab0Field::AttrFencingLevel: return L"FencingLevel";
		case ETab0Field::AttrFencingExp: return L"FencingExp";
		case ETab0Field::AttrSabreLevel: return L"SabreLevel";
		case ETab0Field::AttrSabreExp: return L"SabreExp";
		case ETab0Field::AttrSpearLevel: return L"SpearLevel";
		case ETab0Field::AttrSpearExp: return L"SpearExp";
		case ETab0Field::AttrHiddenWeaponsLevel: return L"HiddenWeaponsLevel";
		case ETab0Field::AttrHiddenWeaponsExp: return L"HiddenWeaponsExp";
		case ETab0Field::AttrOtherWeaponsLevel: return L"OtherWeaponsLevel";
		case ETab0Field::AttrOtherWeaponsExp: return L"OtherWeaponsExp";
		case ETab0Field::FishingLevelInt: return L"FishingLevel";
		default:
			return nullptr;
		}
	}

	bool BuildGameplayAttributeByField(ETab0Field Field, FGameplayAttribute* OutAttr)
	{
		if (!OutAttr)
			return false;

		const wchar_t* AttrName = ResolveAttrFieldName(Field);
		if (!AttrName || !AttrName[0])
			return false;

		FGameplayAttribute Attr{};
		Attr.AttributeName = FString(AttrName);
		Attr.AttributeOwner = static_cast<UStruct*>(UJHAttributeSet::StaticClass());
		*OutAttr = Attr;
		return true;
	}

	bool BuildGameplayAttributeByName(const wchar_t* AttrName, FGameplayAttribute* OutAttr)
	{
		if (!AttrName || !AttrName[0] || !OutAttr)
			return false;
		FGameplayAttribute Attr{};
		Attr.AttributeName = FString(AttrName);
		Attr.AttributeOwner = static_cast<UStruct*>(UJHAttributeSet::StaticClass());
		*OutAttr = Attr;
		return true;
	}

	bool TryGetLiveFloatAttribute(const FTab0HeroContext& Ctx, ETab0Field Field, float* OutValue)
	{
		if (!OutValue || Ctx.NPCId <= 0)
			return false;

		FGameplayAttribute Attr{};
		if (!BuildGameplayAttributeByField(Field, &Attr))
			return false;

		bool bFound = false;
		const float Value = UNPCFuncLib::GetFloatAttribute(Ctx.NPCId, Attr, &bFound);
		if (!bFound)
			return false;

		*OutValue = Value;
		return true;
	}

	bool TryAddLiveFloatAttributeDelta(const FTab0HeroContext& Ctx, ETab0Field Field, float DeltaValue)
	{
		if (Ctx.NPCId <= 0)
			return false;

		if (std::fabs(DeltaValue) < 0.0001f)
			return true;

		FGameplayAttribute Attr{};
		if (!BuildGameplayAttributeByField(Field, &Attr))
			return false;

		UNPCFuncLib::AddFloatAttribute(Ctx.NPCId, Attr, DeltaValue, false);
		return true;
	}

	FTab0HeroContext BuildTab0HeroContext(APlayerController* PC)
	{
		FTab0HeroContext Ctx{};
		Ctx.ItemManager = UManagerFuncLib::GetItemManager();
		Ctx.NPCManager = UManagerFuncLib::GetNPCManager();

		UObject* WorldContext = nullptr;
		if (PC)
			WorldContext = static_cast<UObject*>(PC);
		else if (UWorld* World = UWorld::GetWorld())
			WorldContext = static_cast<UObject*>(World);

		AActor* HeroActor = WorldContext ? UNPCFuncLib::GetSceneHero(WorldContext) : nullptr;
		if (HeroActor && HeroActor->IsA(AJHCharacter::StaticClass()))
			Ctx.NPCId = static_cast<AJHCharacter*>(HeroActor)->NPCId;

		if (Ctx.NPCId > 0)
			Ctx.TeamInfo = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);

		if (!Ctx.TeamInfo)
		{
			if (UTeamManager* TeamManager = UManagerFuncLib::GetTeamManager())
				Ctx.TeamInfo = TeamManager->GetInfoByIndex(0);
		}

		if (Ctx.TeamInfo)
		{
			if (Ctx.NPCId <= 0)
				Ctx.NPCId = Ctx.TeamInfo->NPCId;
			Ctx.AttrSet = Ctx.TeamInfo->GetAttributeSet();
			if (!Ctx.AttrSet)
				Ctx.AttrSet = Ctx.TeamInfo->AttributeSet;
		}

		// 兜底：部分场景 GetSceneHero/Index0 取到的 NPCId 可能是 0，
		// 尝试从 TeamManager 的队伍列表里找第一个有效 NPCId。
		if (Ctx.NPCId <= 0)
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
						if (Info->NPCId > 0)
						{
							Ctx.TeamInfo = Info;
							Ctx.NPCId = Info->NPCId;
							return true;
						}
					}
					return false;
				};

				if (!TryPickFromArray(TeamManager->TeamInfos))
					TryPickFromArray(TeamManager->FightTeamInfos);
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
		if (Ctx.NPCId > 0)
			LiveTeamInfo = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);
		if (!LiveTeamInfo)
			LiveTeamInfo = Ctx.TeamInfo;
		return LiveTeamInfo;
	}

	float GetVolumeItemPercent(UBPVE_JHConfigVolumeItem2_C* Item)
	{
		if (!Item || !Item->VolumeSlider || !IsSafeLiveObject(static_cast<UObject*>(Item->VolumeSlider)))
			return -1.0f;

		USlider* Slider = Item->VolumeSlider;
		float MinValue = Slider->MinValue;
		float MaxValue = Slider->MaxValue;
		float CurValue = Slider->GetValue();
		float Norm = CurValue;
		if (MaxValue > MinValue)
			Norm = (CurValue - MinValue) / (MaxValue - MinValue);
		if (Norm < 0.0f) Norm = 0.0f;
		if (Norm > 1.0f) Norm = 1.0f;
		return Norm * 100.0f;
	}

	void UpdateVolumeItemPercentText(UBPVE_JHConfigVolumeItem2_C* Item, float Percent)
	{
		if (!Item || !Item->TXT_CurrentValue || !IsSafeLiveObject(static_cast<UObject*>(Item->TXT_CurrentValue)))
			return;
		int32 DisplayValue = static_cast<int32>(std::round(Percent));
		if (DisplayValue < 0) DisplayValue = 0;
		if (DisplayValue > 100) DisplayValue = 100;
		wchar_t Buf[16] = {};
		swprintf_s(Buf, 16, L"%d", DisplayValue);
		Item->TXT_CurrentValue->SetText(MakeText(Buf));
	}

	void SetVolumeItemPercent(UBPVE_JHConfigVolumeItem2_C* Item, float Percent)
	{
		if (!Item || !Item->VolumeSlider || !IsSafeLiveObject(static_cast<UObject*>(Item->VolumeSlider)))
			return;
		USlider* Slider = Item->VolumeSlider;
		float MinValue = Slider->MinValue;
		float MaxValue = Slider->MaxValue;
		float ClampedPercent = Percent;
		if (ClampedPercent < 0.0f) ClampedPercent = 0.0f;
		if (ClampedPercent > 100.0f) ClampedPercent = 100.0f;
		float Norm = ClampedPercent / 100.0f;
		float TargetValue = MinValue + (MaxValue - MinValue) * Norm;
		Slider->SetValue(TargetValue);
		UpdateVolumeItemPercentText(Item, ClampedPercent);
	}

	bool TryReadMultiplierFallbackFromAttrSet(const FTab0HeroContext& Ctx, const wchar_t* AttrName, float* OutValue)
	{
		if (!Ctx.AttrSet || !AttrName || !OutValue)
			return false;
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
		return false;
	}

	bool TryGetTab0MultiplierPercent(const FTab0HeroContext& Ctx, const wchar_t* AttrName, float* OutPercent)
	{
		if (!OutPercent || !AttrName || !AttrName[0])
			return false;

		float RawValue = 0.0f;
		bool bFound = false;

		FGameplayAttribute Attr{};
		if (BuildGameplayAttributeByName(AttrName, &Attr) && Ctx.NPCId > 0)
		{
			RawValue = UNPCFuncLib::GetFloatAttribute(Ctx.NPCId, Attr, &bFound);
			if (bFound)
			{
				*OutPercent = RawValue * 100.0f;
				return true;
			}
		}

		if (TryReadMultiplierFallbackFromAttrSet(Ctx, AttrName, &RawValue))
		{
			*OutPercent = RawValue * 100.0f;
			return true;
		}
		return false;
	}

	bool ApplyTab0MultiplierAttribute(const FTab0HeroContext& Ctx, const wchar_t* AttrName, float Percent, const char* LogName)
	{
		if (Ctx.NPCId <= 0)
		{
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Write] field=" << LogName
				          << " sdk=UNPCFuncLib::AddFloatAttribute fail reason=NpcIdInvalid\n";
			return false;
		}

		FGameplayAttribute Attr{};
		if (!BuildGameplayAttributeByName(AttrName, &Attr))
			return false;

		const float Target = Percent / 100.0f;
		bool bFound = false;
		float Current = UNPCFuncLib::GetFloatAttribute(Ctx.NPCId, Attr, &bFound);
		if (!bFound)
		{
			bFound = TryReadMultiplierFallbackFromAttrSet(Ctx, AttrName, &Current);
			if (kTab0VerboseLog && bFound)
				std::cout << "[SDK][Tab0Write] field=" << LogName
				          << " sdk=UJHAttributeSet::CurrentValue(fallback) current=" << Current
				          << " npcId=" << Ctx.NPCId << "\n";
		}
		if (!bFound)
		{
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Write] field=" << LogName
				          << " sdk=UNPCFuncLib::GetFloatAttribute fail reason=AttrNotFound npcId=" << Ctx.NPCId << "\n";
			return false;
		}

		const float Delta = Target - Current;
		if (std::fabs(Delta) <= 0.0001f)
			return true;

		if (kTab0VerboseLog)
			std::cout << "[SDK][Tab0Write] field=" << LogName
			          << " sdk=UNPCFuncLib::AddFloatAttribute call delta=" << Delta
			          << " current=" << Current << " target=" << Target
			          << " npcId=" << Ctx.NPCId << "\n";
		UNPCFuncLib::AddFloatAttribute(Ctx.NPCId, Attr, Delta, false);

		bool bFoundAfter = false;
		float After = UNPCFuncLib::GetFloatAttribute(Ctx.NPCId, Attr, &bFoundAfter);
		if (!bFoundAfter)
			bFoundAfter = TryReadMultiplierFallbackFromAttrSet(Ctx, AttrName, &After);
		if (!bFoundAfter)
			return false;

		const bool Ok = std::fabs(After - Target) <= 0.05f;
		if (kTab0VerboseLog)
			std::cout << "[SDK][Tab0Write] field=" << LogName
			          << " sdk=UNPCFuncLib::GetFloatAttribute(readback) after=" << After
			          << " target=" << Target << " ok=" << (Ok ? 1 : 0) << "\n";
		return Ok;
	}

	void PollTab0RatioSliders(APlayerController* PC)
	{
		FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
		if (Ctx.NPCId <= 0)
			return;

		auto PollOne = [&](UBPVE_JHConfigVolumeItem2_C* Item, float& LastPercent, const wchar_t* AttrName, const char* LogName)
		{
			if (!Item || !IsSafeLiveObject(static_cast<UObject*>(Item)))
				return;
			float Percent = GetVolumeItemPercent(Item);
			if (Percent < 0.0f)
				return;
			UpdateVolumeItemPercentText(Item, Percent);

			if (LastPercent < 0.0f)
			{
				LastPercent = Percent;
				ApplyTab0MultiplierAttribute(Ctx, AttrName, Percent, LogName);
				return;
			}

			if (std::fabs(Percent - LastPercent) >= 0.5f)
			{
				ApplyTab0MultiplierAttribute(Ctx, AttrName, Percent, LogName);
				LastPercent = Percent;
			}
		};

		PollOne(GTab0MoneyMultiplierItem, GTab0MoneyMultiplierLastPercent, L"MoneyMultiplier", "MoneyMultiplier");
		PollOne(GTab0SkillExpMultiplierItem, GTab0SkillExpMultiplierLastPercent, L"SExpMultiplier", "SExpMultiplier");
		PollOne(GTab0ManaCostMultiplierItem, GTab0ManaCostMultiplierLastPercent, L"ManaCostMultiplier", "ManaCostMultiplier");
	}

	bool TryGetTab0FieldValue(const FTab0HeroContext& Ctx, ETab0Field Field, double* OutValue)
	{
		if (!OutValue)
			return false;

		switch (Field)
		{
		case ETab0Field::Money:
			if (!Ctx.ItemManager)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UItemManager::GetMoney fail reason=ItemManagerNull\n";
				return false;
			}
			*OutValue = static_cast<double>(Ctx.ItemManager->GetMoney());
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UItemManager::GetMoney ok value=" << *OutValue << "\n";
			return true;
		case ETab0Field::SkillExp:
		{
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::GetNPCInfoById fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}
			*OutValue = static_cast<double>(LiveTeamInfo->SkillExp);
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UTeamInfo::SkillExp ok value=" << *OutValue
				          << " npcId=" << Ctx.NPCId << "\n";
			return true;
		}
		case ETab0Field::JingMaiPoint:
			if (Ctx.NPCManager && Ctx.NPCId > 0)
			{
				*OutValue = static_cast<double>(Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId));
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::GetJingMaiPoint ok value=" << *OutValue
					          << " npcId=" << Ctx.NPCId << "\n";
				return true;
			}
			if (UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx))
			{
				*OutValue = static_cast<double>(LiveTeamInfo->JingMaiPoint);
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::JingMaiPoint(fallback) ok value=" << *OutValue
					          << " npcId=" << Ctx.NPCId << "\n";
				return true;
			}
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UNPCManager::GetJingMaiPoint fail reason=NoManagerOrNoTeamInfo npcId=" << Ctx.NPCId << "\n";
			return false;
		case ETab0Field::GuildHonor:
		{
			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::GetGuildHonor fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}
			const int32 GuildId = LiveTeamInfo->GuildId;
			if (GuildId <= 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
					          << " sdk=UTeamInfo::GetGuildHonor fail reason=GuildIdInvalid npcId=" << Ctx.NPCId << "\n";
				return false;
			}
			*OutValue = static_cast<double>(LiveTeamInfo->GetGuildHonor(GuildId));
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=UTeamInfo::GetGuildHonor ok value=" << *OutValue
				          << " npcId=" << Ctx.NPCId << " guildId=" << GuildId << "\n";
			return true;
		}
		case ETab0Field::InheritPoint:
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
				          << " sdk=None fail reason=NotImplemented\n";
			return false;
		case ETab0Field::FishingLevelInt:
			break;
		default:
			break;
		}

		const wchar_t* AttrName = ResolveAttrFieldName(Field);
		if (AttrName)
		{
			float LiveValue = 0.0f;
			if (Ctx.NPCId > 0 && TryGetLiveFloatAttribute(Ctx, Field, &LiveValue))
			{
				*OutValue = static_cast<double>(LiveValue);
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
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
						std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
						          << " sdk=UJHAttributeSet::CurrentValue(fallback) ok value=" << *OutValue
						          << " npcId=" << Ctx.NPCId << "\n";
					return true;
				}
			}
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Read] field=" << Tab0FieldToString(Field)
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
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UItemFuncLib::AddMoney fail reason=ItemManagerNull\n";
				return false;
			}
			const int32 Current = Ctx.ItemManager->GetMoney();
			const int32 Target = RoundToInt(InValue);
			const int32 Delta = Target - Current;
			if (Delta != 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UItemFuncLib::AddMoney call delta=" << Delta
					          << " current=" << Current << " target=" << Target << "\n";
				UItemFuncLib::AddMoney(Delta);
			}
			const int32 After = Ctx.ItemManager->GetMoney();
			if (After != Target)
			{
				std::cout << "[SDK] Tab0CommitMoney: mismatch target=" << Target
				          << " after=" << After << " delta=" << Delta << "\n";
			}
			return (After == Target);
		}
		case ETab0Field::SkillExp:
		{
			if (Ctx.NPCId <= 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=USkillFuncLib::AddSkillExp fail reason=NpcIdInvalid\n";
				return false;
			}

			UTeamInfo* LiveTeamInfo = ResolveLiveTeamInfo(Ctx);
			if (!LiveTeamInfo)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::GetNPCInfoById fail reason=TeamInfoNull npcId=" << Ctx.NPCId << "\n";
				return false;
			}

			const int32 Target = RoundToInt(InValue);
			const int32 Current = LiveTeamInfo->SkillExp;
			const int32 Delta = Target - Current;
			if (Delta != 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=USkillFuncLib::AddSkillExp call delta=" << Delta
					          << " current=" << Current << " target=" << Target
					          << " npcId=" << Ctx.NPCId << "\n";
				USkillFuncLib::AddSkillExp(Ctx.NPCId, Delta);
			}

			UTeamInfo* VerifyTeamInfo = UNPCFuncLib::GetNPCInfoById(Ctx.NPCId);
			if (!VerifyTeamInfo)
				VerifyTeamInfo = LiveTeamInfo;
			if (!VerifyTeamInfo || VerifyTeamInfo->SkillExp != Target)
			{
				std::cout << "[SDK] Tab0CommitSkillExp: mismatch npcId=" << Ctx.NPCId
				          << " target=" << Target
				          << " after=" << (VerifyTeamInfo ? VerifyTeamInfo->SkillExp : -1)
				          << " delta=" << Delta << "\n";
			}
			return VerifyTeamInfo && (VerifyTeamInfo->SkillExp == Target);
		}
		case ETab0Field::JingMaiPoint:
		{
			if (Ctx.NPCId <= 0 || !Ctx.NPCManager)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::AddJingMaiPoint fail reason=NpcManagerOrNpcIdInvalid npcId=" << Ctx.NPCId << "\n";
				return false;
			}

			const int32 Target = RoundToInt(InValue);
			const int32 Current = Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId);
			const int32 Delta = Target - Current;
			if (Delta != 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::AddJingMaiPoint call delta=" << Delta
					          << " current=" << Current << " target=" << Target
					          << " npcId=" << Ctx.NPCId << "\n";
				UNPCFuncLib::AddJingMaiPoint(Ctx.NPCId, Delta, false);
			}

			const int32 AfterDelta = Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId);
			if (AfterDelta != Target)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::SetJingMaiPoint call afterDelta=" << AfterDelta
					          << " target=" << Target << " npcId=" << Ctx.NPCId << "\n";
				Ctx.NPCManager->SetJingMaiPoint(Ctx.NPCId, Target);
			}
			const int32 AfterSet = Ctx.NPCManager->GetJingMaiPoint(Ctx.NPCId);
			if (AfterSet != Target)
			{
				std::cout << "[SDK] Tab0CommitJingMai: mismatch npcId=" << Ctx.NPCId
				          << " target=" << Target << " after=" << AfterSet
				          << " delta=" << Delta << "\n";
			}
			return (AfterSet == Target);
		}
		case ETab0Field::GuildHonor:
		{
			if (!Ctx.NPCManager)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::ChangeGuildHonor fail reason=NpcManagerNull\n";
				return false;
			}

			const int32 Target = RoundToInt(InValue);
			const int32 EffectiveNPCId = Ctx.NPCId;
			if (EffectiveNPCId <= 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::ChangeGuildHonor fail reason=NpcIdInvalid\n";
				return false;
			}

			UTeamInfo* LiveTeamInfo = UNPCFuncLib::GetNPCInfoById(EffectiveNPCId);
			if (!LiveTeamInfo)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::GetNPCInfoById fail reason=TeamInfoNull npcId=" << EffectiveNPCId << "\n";
				return false;
			}

			const int32 GuildId = LiveTeamInfo->GuildId;
			if (GuildId <= 0)
				return false;

			const int32 Current = LiveTeamInfo->GetGuildHonor(GuildId);
			const int32 Delta = Target - Current;
			if (Delta != 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCManager::ChangeGuildHonor call delta=" << Delta
					          << " current=" << Current << " target=" << Target
					          << " npcId=" << EffectiveNPCId << " guildId=" << GuildId << "\n";
				Ctx.NPCManager->ChangeGuildHonor(EffectiveNPCId, GuildId, Delta);
			}

			UTeamInfo* VerifyTeamInfo = UNPCFuncLib::GetNPCInfoById(EffectiveNPCId);
			if (!VerifyTeamInfo || VerifyTeamInfo->GetGuildHonor(GuildId) != Target)
			{
				const int32 After = VerifyTeamInfo ? VerifyTeamInfo->GetGuildHonor(GuildId) : -1;
				std::cout << "[SDK] Tab0CommitGuildHonor: mismatch npcId=" << EffectiveNPCId
				          << " guildId=" << GuildId
				          << " target=" << Target << " after=" << After
				          << " delta=" << Delta << "\n";
			}
			return VerifyTeamInfo && (VerifyTeamInfo->GetGuildHonor(GuildId) == Target);
		}
		case ETab0Field::InheritPoint:
			return false;
		case ETab0Field::FishingLevelInt:
			break;
		default:
			break;
		}

		const float TargetFloat = static_cast<float>(InValue);
		const wchar_t* AttrName = ResolveAttrFieldName(Field);
		if (AttrName)
		{
			if (Ctx.NPCId <= 0)
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
					          << " sdk=UNPCFuncLib::AddFloatAttribute fail reason=NpcIdInvalid\n";
				return false;
			}

			float LiveCurrent = 0.0f;
			if (!TryGetLiveFloatAttribute(Ctx, Field, &LiveCurrent))
			{
				std::cout << "[SDK] Tab0CommitLiveAttr: get current failed, field="
				          << static_cast<int32>(Field) << " npcId=" << Ctx.NPCId << "\n";
				return false;
			}

			const float Delta = TargetFloat - LiveCurrent;
			const bool AppliedLive = TryAddLiveFloatAttributeDelta(Ctx, Field, Delta);
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Write] field=" << Tab0FieldToString(Field)
				          << " sdk=UNPCFuncLib::AddFloatAttribute call delta=" << Delta
				          << " current=" << LiveCurrent << " target=" << TargetFloat
				          << " npcId=" << Ctx.NPCId << " applied=" << (AppliedLive ? 1 : 0) << "\n";

			float LiveAfter = 0.0f;
			if (!AppliedLive || !TryGetLiveFloatAttribute(Ctx, Field, &LiveAfter))
			{
				std::cout << "[SDK] Tab0CommitLiveAttr: apply/readback failed, field="
				          << static_cast<int32>(Field) << " npcId=" << Ctx.NPCId
				          << " delta=" << Delta << "\n";
				return false;
			}

			if (std::fabs(LiveAfter - TargetFloat) > 0.5f)
			{
				std::cout << "[SDK] Tab0CommitLiveAttr: mismatch after apply, field="
				          << static_cast<int32>(Field) << " npcId=" << Ctx.NPCId
				          << " target=" << TargetFloat << " after=" << LiveAfter << "\n";
				return false;
			}
			return true;
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
			return;

		UEditableTextBox* Edit = FindFirstEditableTextBox(RowRoot);
		if (!Edit)
			return;

		auto Existing = std::find_if(
			GTab0Bindings.begin(),
			GTab0Bindings.end(),
			[Edit](const FTab0Binding& B) { return B.Edit == Edit; });
		if (Existing != GTab0Bindings.end())
			return;

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
}

void PopulateTab_Character(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VolumeSlot, "Tab0(VolumeSlot)");
	if (!Container)
	{
		std::cout << "[SDK] Tab0: no container available, skipping\n";
		return;
	}

	Container->ClearChildren();
	GTab0Bindings.clear();
	GTab0EnterWasDown = false;
	GTab0LastFocusedEdit = nullptr;
	GTab0MoneyMultiplierItem = nullptr;
	GTab0SkillExpMultiplierItem = nullptr;
	GTab0ManaCostMultiplierItem = nullptr;
	GTab0MoneyMultiplierLastPercent = -1.0f;
	GTab0SkillExpMultiplierLastPercent = -1.0f;
	GTab0ManaCostMultiplierLastPercent = -1.0f;
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

	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};

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

	auto* RolePanel = CreateCollapsiblePanel(PC, L"角色选项");
	auto* RoleBox = RolePanel ? RolePanel->CT_Contents : nullptr;
	AddDropdown(RoleBox, L"额外心法栏", { L"0", L"1", L"2" });
	AddDropdown(RoleBox, L"门派",
		{ L"无门派", L"少林", L"武当", L"峨眉",
		  L"明教", L"丐帮", L"唐门", L"天山" });
	AddPanelWithFixedGap(RolePanel, 0.0f, 10.0f);

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
	{
		FTab0HeroContext RatioCtx = BuildTab0HeroContext(PC);
		float Pct = 100.0f;

		if (TryGetTab0MultiplierPercent(RatioCtx, L"MoneyMultiplier", &Pct))
			SetVolumeItemPercent(GTab0MoneyMultiplierItem, Pct);
		else
			SetVolumeItemPercent(GTab0MoneyMultiplierItem, 100.0f);
		GTab0MoneyMultiplierLastPercent = GetVolumeItemPercent(GTab0MoneyMultiplierItem);

		Pct = 100.0f;
		if (TryGetTab0MultiplierPercent(RatioCtx, L"SExpMultiplier", &Pct))
			SetVolumeItemPercent(GTab0SkillExpMultiplierItem, Pct);
		else
			SetVolumeItemPercent(GTab0SkillExpMultiplierItem, 100.0f);
		GTab0SkillExpMultiplierLastPercent = GetVolumeItemPercent(GTab0SkillExpMultiplierItem);

		Pct = 100.0f;
		if (TryGetTab0MultiplierPercent(RatioCtx, L"ManaCostMultiplier", &Pct))
			SetVolumeItemPercent(GTab0ManaCostMultiplierItem, Pct);
		else
			SetVolumeItemPercent(GTab0ManaCostMultiplierItem, 100.0f);
		GTab0ManaCostMultiplierLastPercent = GetVolumeItemPercent(GTab0ManaCostMultiplierItem);
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
	RefreshTab0BindingsText(PC);

	std::cout << "[SDK] Tab0 (Character): " << Count
	          << " widgets added, bindings=" << GTab0Bindings.size() << "\n";
}

void PollTab0CharacterInput(bool bTab0Active)
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

	const bool EnterDown = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
	const bool EnterTriggered = EnterDown && !GTab0EnterWasDown;
	if (!bTab0Active)
	{
		GTab0EnterWasDown = EnterDown;
		GTab0LastFocusedEdit = nullptr;
		return;
	}

	APlayerController* PC = nullptr;
	if (UWorld* World = UWorld::GetWorld())
		PC = UGameplayStatics::GetPlayerController(World, 0);

	// Tab0 三个倍率滑块：实时同步到游戏属性
	PollTab0RatioSliders(PC);

	FTab0Binding* Focused = nullptr;
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

	// 某些编辑框按下回车瞬间会丢失键盘焦点，回车提交时回退到“上一次有焦点的编辑框”。
	if (EnterTriggered && !Focused && GTab0LastFocusedEdit)
	{
		Focused = FindTab0BindingByEdit(GTab0LastFocusedEdit);
		if (kTab0VerboseLog && Focused)
			std::cout << "[SDK][Tab0Input] EnterTriggered fallbackFocus field="
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
			RefreshSingleTab0BindingText(*LastFocusedBinding, PC);
	}

	if (EnterTriggered)
	{
		if (kTab0VerboseLog && !Focused)
			std::cout << "[SDK][Tab0Input] EnterTriggered but no focused editable binding\n";

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
				std::cout << "[SDK][Tab0Input] EnterTriggered field=" << Tab0FieldToString(Focused->Field)
				          << " raw=" << Raw << " parsed=" << Parsed
				          << " parsedAll=" << (bParsedAll ? 1 : 0) << "\n";
			if (EndPtr != Raw.c_str() && bParsedAll)
			{
				FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
				if (TrySetTab0FieldValue(Ctx, Focused->Field, Parsed))
				{
					RefreshTab0BindingsText(PC);
					std::cout << "[SDK] Tab0Commit: " << (Focused->Title ? "ok" : "unnamed")
					          << " raw=" << Raw << "\n";
				}
				else if (kTab0VerboseLog)
				{
					std::cout << "[SDK][Tab0Input] Commit failed field=" << Tab0FieldToString(Focused->Field)
					          << " raw=" << Raw << "\n";
				}
			}
			else
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Input] Invalid numeric input, rollback field="
					          << Tab0FieldToString(Focused->Field) << " raw=" << Raw << "\n";
				RefreshSingleTab0BindingText(*Focused, PC);
			}
		}
	}

	GTab0LastFocusedEdit = Focused ? Focused->Edit : nullptr;
	GTab0EnterWasDown = EnterDown;
}


void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->VideoSlot, "Tab1(VideoSlot)");
	if (!Container) return;

	Container->ClearChildren();
	ClearItemBrowserState();
	int Count = 0;
	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	auto* OptionsPanelRoot = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	UPanelWidget* OptionsBox = OptionsPanelRoot;

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else if (OptionsBox) OptionsBox->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else if (OptionsBox) OptionsBox->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : (OptionsBox ? OptionsBox : Container), Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item);
			else if (OptionsBox) OptionsBox->AddChild(Item);
			else Container->AddChild(Item);
			Count++;
		}
	};

	if (OptionsBox)
	{
		auto AddSubPanel = [&](const wchar_t* Title) -> UPanelWidget*
		{
			auto* Sub = CreateCollapsiblePanel(PC, Title);
			if (!Sub)
				return nullptr;
			OptionsBox->AddChild(Sub);
			Count++;
			return Sub->CT_Contents;
		};

		auto* CoreBox = AddSubPanel(L"基础开关");
		AddToggle(CoreBox, L"物品不减");
		AddToggle(CoreBox, L"物品获得加倍");
		AddToggle(CoreBox, L"所有物品可出售");
		AddToggle(CoreBox, L"包括任务物品");
		AddToggle(CoreBox, L"掉落率100%");
		AddToggle(CoreBox, L"锻造制衣效果加倍");

		auto* RatioBox = AddSubPanel(L"倍率设置");
		AddSlider(RatioBox, L"加倍倍数");
		AddSlider(RatioBox, L"道具增量效果倍率");
		AddSlider(RatioBox, L"额外效果倍率");

		auto* LimitBox = AddSubPanel(L"限制与词条");
		AddNumeric(LimitBox, L"最大额外词条数", L"3");
		AddToggle(LimitBox, L"无视物品使用次数");
		AddToggle(LimitBox, L"无视物品使用要求");
	}
	else
	{
		AddToggle(nullptr, L"物品不减");
		AddToggle(nullptr, L"物品获得加倍");
		AddSlider(nullptr, L"加倍倍数");
		AddToggle(nullptr, L"所有物品可出售");
		AddToggle(nullptr, L"包括任务物品");
		AddToggle(nullptr, L"掉落率100%");
		AddToggle(nullptr, L"锻造制衣效果加倍");
		AddSlider(nullptr, L"道具增量效果倍率");
		AddSlider(nullptr, L"额外效果倍率");
		AddNumeric(nullptr, L"最大额外词条数", L"3");
		AddToggle(nullptr, L"无视物品使用次数");
		AddToggle(nullptr, L"无视物品使用要求");
	}

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

	if (OptionsPanelRoot)
	{
		UPanelSlot* Slot = Container->AddChild(OptionsPanelRoot);
		if (Slot && Slot->IsA(UVerticalBoxSlot::StaticClass()))
		{
			auto* VSlot = static_cast<UVerticalBoxSlot*>(Slot);
			FMargin Pad{};
			Pad.Left = 0.0f;
			Pad.Top = 0.0f;
			Pad.Right = 0.0f;
			Pad.Bottom = 14.0f;
			VSlot->SetPadding(Pad);
		}
		Count++;
	}

	
	auto* BrowserPanel = CreateCollapsiblePanel(PC, L"物品浏览器");
	UPanelWidget* BrowserBox = BrowserPanel ? BrowserPanel->CT_Contents : nullptr;

	BuildItemCache();
	GItemQuantityEdit = nullptr;
	GItemCategoryDD = CreateVideoItemWithOptions(PC,
		L"\u2501\u2501\u7269\u54C1\u7BA1\u7406\u2501\u2501",
		{ L"\u5168\u90E8", L"\u6B66\u5668", L"\u9632\u5177", L"\u6D88\u8017\u54C1", L"\u5176\u4ED6" });
	GItemLastCatIdx = 0;

	GItemQuantityRow = nullptr;
	if (GItemCategoryDD)
	{
		auto* SearchEdit = static_cast<UEditableTextBox*>(
			CreateRawWidget(UEditableTextBox::StaticClass(), Outer));
		if (SearchEdit)
		{
			SearchEdit->SetHintText(MakeText(L"\u8F93\u5165\u4EE5\u641C\u7D22..."));
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

			auto MakeSlateColor = [](float R, float G, float B, float A) -> FSlateColor
			{
				FSlateColor C{};
				C.SpecifiedColor = FLinearColor{ R, G, B, A };
				C.ColorUseRule = ESlateColorStylingMode::UseColor_Specified;
				return C;
			};

			SearchEdit->ForegroundColor = FLinearColor{ 0.95f, 0.95f, 0.95f, 1.0f };
			SearchEdit->BackgroundColor = FLinearColor{ 0.0f, 0.0f, 0.0f, 0.0f };
			SearchEdit->WidgetStyle.ForegroundColor = MakeSlateColor(0.95f, 0.95f, 0.95f, 1.0f);
			SearchEdit->WidgetStyle.BackgroundColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.ReadOnlyForegroundColor = MakeSlateColor(0.75f, 0.75f, 0.75f, 1.0f);
			SearchEdit->WidgetStyle.BackgroundImageNormal.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.BackgroundImageHovered.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.BackgroundImageFocused.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
			SearchEdit->WidgetStyle.BackgroundImageReadOnly.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);

			UWidget* SearchWidget = SearchEdit;
			auto* SearchSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
			if (SearchSize)
			{
				SearchSize->SetWidthOverride(310.0f);
				SearchSize->SetHeightOverride(64.0f);
				SearchSize->SetContent(SearchWidget);
				SearchWidget = SearchSize;
			}
			SearchWidget->SetRenderTranslation(FVector2D{ 0.0f, -0.75f });

			if (GItemCategoryDD->TXT_Title)
			{
				GItemCategoryDD->TXT_Title->SetText(MakeText(L""));
				GItemCategoryDD->TXT_Title->SetVisibility(ESlateVisibility::Collapsed);
				UPanelWidget* TitleParent = GItemCategoryDD->TXT_Title->GetParent();
				if (TitleParent)
				{
					UPanelSlot* SearchPanelSlot = nullptr;
					if (TitleParent->IsA(UHorizontalBox::StaticClass()))
					{
						auto* HParent = static_cast<UHorizontalBox*>(TitleParent);
						HParent->RemoveChild(GItemCategoryDD->TXT_Title);

						bool bReaddCombo = false;
						if (GItemCategoryDD->CB_Main && GItemCategoryDD->CB_Main->GetParent() == HParent)
						{
							HParent->RemoveChild(GItemCategoryDD->CB_Main);
							bReaddCombo = true;
						}

						SearchPanelSlot = HParent->AddChildToHorizontalBox(SearchWidget);
						if (bReaddCombo)
						{
							UWidget* ComboWidget = GItemCategoryDD->CB_Main;
							auto* ComboSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
							if (ComboSize && GItemCategoryDD->CB_Main)
							{
								ComboSize->SetWidthOverride(260.0f);
								ComboSize->SetHeightOverride(40.0f);
								ComboSize->SetContent(GItemCategoryDD->CB_Main);
								ComboWidget = ComboSize;
							}

							auto* ComboSlot = HParent->AddChildToHorizontalBox(ComboWidget);
							if (ComboSlot)
							{
								FSlateChildSize AutoSize{};
								AutoSize.SizeRule = ESlateSizeRule::Automatic;
								AutoSize.Value = 0.0f;
								ComboSlot->SetSize(AutoSize);
								ComboSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
								ComboSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
								FMargin ComboPad{};
								ComboPad.Right = 4.0f;
								ComboSlot->SetPadding(ComboPad);
							}
							ComboWidget->SetRenderTranslation(FVector2D{ 2.0f, 0.0f });
						}
					}
					else
					{
						// 非 Horizontal 容器（常见是 Canvas）时，重建一层横向容器承载搜索框和下拉框，
						// 避免原蓝图左侧标题区域固定宽度导致搜索框宽度不生效。
						auto* ReplaceRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
						if (ReplaceRow)
						{
							TitleParent->RemoveChild(GItemCategoryDD->TXT_Title);
							if (GItemCategoryDD->CB_Main && GItemCategoryDD->CB_Main->GetParent() == TitleParent)
								TitleParent->RemoveChild(GItemCategoryDD->CB_Main);

							auto* NewSearchSlot = ReplaceRow->AddChildToHorizontalBox(SearchWidget);
							if (NewSearchSlot)
							{
								FSlateChildSize Fill{};
								Fill.SizeRule = ESlateSizeRule::Fill;
								Fill.Value = 1.0f;
								NewSearchSlot->SetSize(Fill);
								NewSearchSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
								NewSearchSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
								FMargin SearchPad{};
								SearchPad.Right = 10.0f;
								NewSearchSlot->SetPadding(SearchPad);
							}

							if (GItemCategoryDD->CB_Main)
							{
								UWidget* ComboWidget = GItemCategoryDD->CB_Main;
								auto* ComboSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
								if (ComboSize)
								{
									ComboSize->SetWidthOverride(300.0f);
									ComboSize->SetHeightOverride(40.0f);
									ComboSize->SetContent(GItemCategoryDD->CB_Main);
									ComboWidget = ComboSize;
								}

								auto* NewComboSlot = ReplaceRow->AddChildToHorizontalBox(ComboWidget);
								if (NewComboSlot)
								{
									FSlateChildSize AutoSize{};
									AutoSize.SizeRule = ESlateSizeRule::Automatic;
									NewComboSlot->SetSize(AutoSize);
									NewComboSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
									NewComboSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
									FMargin ComboPad{};
									ComboPad.Right = 4.0f;
									NewComboSlot->SetPadding(ComboPad);
								}
								ComboWidget->SetRenderTranslation(FVector2D{ 2.0f, 0.0f });
							}

							SearchPanelSlot = TitleParent->AddChild(ReplaceRow);
						}
						else
						{
							SearchPanelSlot = TitleParent->AddChild(SearchWidget);
						}
					}

					if (SearchPanelSlot && SearchPanelSlot->IsA(UHorizontalBoxSlot::StaticClass()))
					{
						auto* SearchHSlot = static_cast<UHorizontalBoxSlot*>(SearchPanelSlot);
						FSlateChildSize Fill{};
						Fill.SizeRule = ESlateSizeRule::Fill;
						Fill.Value = 1.0f;
						SearchHSlot->SetSize(Fill);
						SearchHSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
						SearchHSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
						FMargin SearchPad{};
						SearchPad.Left = 0.0f;
						SearchPad.Top = 0.0f;
						SearchPad.Right = 10.0f;
						SearchPad.Bottom = 0.0f;
						SearchHSlot->SetPadding(SearchPad);
					}
					else if (SearchPanelSlot && SearchPanelSlot->IsA(UCanvasPanelSlot::StaticClass()))
					{
						auto* SearchCSlot = static_cast<UCanvasPanelSlot*>(SearchPanelSlot);
						SearchCSlot->SetAutoSize(false);
						FAnchors Anchors{};
						Anchors.Minimum = FVector2D{ 0.0f, 0.0f };
						Anchors.Maximum = FVector2D{ 1.0f, 1.0f };
						SearchCSlot->SetAnchors(Anchors);
						FMargin Offsets{};
						Offsets.Left = 0.0f;
						Offsets.Top = 0.0f;
						Offsets.Right = 0.0f;
						Offsets.Bottom = 0.0f;
						SearchCSlot->SetOffsets(Offsets);
						SearchCSlot->SetAlignment(FVector2D{ 0.0f, 0.0f });
					}
				}
			}
		}

		if (BrowserBox) BrowserBox->AddChild(GItemCategoryDD);
		else Container->AddChild(GItemCategoryDD);
		Count++;
	}

	GItemPagerRow = static_cast<UHorizontalBox*>(CreateRawWidget(UHorizontalBox::StaticClass(), Outer));
	if (GItemPagerRow)
	{
		UWidget* PrevLayout = nullptr;
		GItemPrevPageBtn = CreateGameStyleButton(PC, L"上一页", "ItemPrevPage",
			136.0f, 48.0f, &PrevLayout);
		if (PrevLayout)
		{
			auto* PrevSlot = GItemPagerRow->AddChildToHorizontalBox(PrevLayout);
			if (PrevSlot)
			{
				PrevSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
				PrevSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			}
		}

		GItemPageLabel = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
		if (GItemPageLabel)
		{
			GItemPageLabel->SetText(MakeText(L"1/1"));
			GItemPageLabel->SetJustification(ETextJustify::Center);
			GItemPageLabel->SetMinDesiredWidth(92.0f);
			GItemPageLabel->Font.Size = 18;
			auto* LabelSlot = GItemPagerRow->AddChildToHorizontalBox(GItemPageLabel);
			if (LabelSlot)
			{
				LabelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
				LabelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			}
		}

		UWidget* NextLayout = nullptr;
		GItemNextPageBtn = CreateGameStyleButton(PC, L"下一页", "ItemNextPage",
			136.0f, 48.0f, &NextLayout);
		if (NextLayout)
		{
			auto* NextSlot = GItemPagerRow->AddChildToHorizontalBox(NextLayout);
			if (NextSlot)
			{
				NextSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
				NextSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			}
		}
	}

	GItemGridPanel = static_cast<UUniformGridPanel*>(CreateRawWidget(UUniformGridPanel::StaticClass(), Outer));
	if (GItemGridPanel)
	{
		for (int32 i = 0; i < ITEMS_PER_PAGE; ++i)
		{
			GItemSlotButtons[i] = nullptr;
			GItemSlotImages[i] = nullptr;
			GItemSlotQualityBorders[i] = nullptr;
			GItemSlotEntryWidgets[i] = nullptr;
			GItemSlotItemIndices[i] = -1;
			GItemSlotWasPressed[i] = false;
		}

		GItemGridPanel->SetMinDesiredSlotWidth(68.0f);
		GItemGridPanel->SetMinDesiredSlotHeight(84.0f);
		GItemGridPanel->SetSlotPadding(FMargin{ 3.0f, 6.0f, 3.0f, 6.0f });
		if (BrowserBox) BrowserBox->AddChild(GItemGridPanel);
		else Container->AddChild(GItemGridPanel);
		Count++;

		for (int32 i = 0; i < ITEMS_PER_PAGE; i++)
		{
			UButton* Btn = nullptr;
			UImage* Img = nullptr;
			UImage* QualityBorder = nullptr;
			UUserWidget* EntryWidget = nullptr;

			auto* Entry = static_cast<UBPEntry_Item_C*>(
				UWidgetBlueprintLibrary::Create(PC, UBPEntry_Item_C::StaticClass(), PC));
			if (Entry)
			{
				MarkAsGCRoot(Entry);
				EntryWidget = Entry;

				if (Entry->ItemDisplay && Entry->ItemDisplay->CMP)
				{
					auto* Display = Entry->ItemDisplay->CMP;
					Img = static_cast<UImage*>(Display->IMG_Item);
					QualityBorder = static_cast<UImage*>(Display->IMG_QualityBorder);

					if (Display->IMG_SolidBG)
					{
						Display->IMG_SolidBG->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
						Display->IMG_SolidBG->SetColorAndOpacity(FLinearColor{ 0.0f, 0.0f, 0.0f, 0.85f });
					}
					if (Display->TXT_Count)
						Display->TXT_Count->SetVisibility(ESlateVisibility::Collapsed);
				}

				if (Entry->BTN_JHItem && Entry->BTN_JHItem->BtnMain)
					Btn = static_cast<UButton*>(Entry->BTN_JHItem->BtnMain);
			}

			// Fallback to a plain slot only if backpack-style widget creation fails.
			if (!EntryWidget || !Btn || !Img)
			{
				auto* FallbackBtn = static_cast<UButton*>(CreateRawWidget(UButton::StaticClass(), Outer));
				auto* FallbackImg = static_cast<UImage*>(CreateRawWidget(UImage::StaticClass(), Outer));
				if (!FallbackBtn || !FallbackImg)
				{
					GItemSlotButtons[i] = nullptr;
					GItemSlotImages[i] = nullptr;
					GItemSlotQualityBorders[i] = nullptr;
					GItemSlotEntryWidgets[i] = nullptr;
					GItemSlotItemIndices[i] = -1;
					GItemSlotWasPressed[i] = false;
					continue;
				}
				FallbackImg->SetVisibility(ESlateVisibility::Collapsed);
				FallbackBtn->SetContent(FallbackImg);
				Btn = FallbackBtn;
				Img = FallbackImg;
				EntryWidget = nullptr;
			}

			int32 Row = i / ITEM_GRID_COLS;
			int32 Col = i % ITEM_GRID_COLS;
			GItemGridPanel->AddChildToUniformGrid(
				EntryWidget ? static_cast<UWidget*>(EntryWidget) : static_cast<UWidget*>(Btn),
				Row, Col);

			GItemSlotButtons[i] = Btn;
			GItemSlotImages[i] = Img;
			GItemSlotQualityBorders[i] = QualityBorder;
			GItemSlotEntryWidgets[i] = EntryWidget;
			GItemSlotItemIndices[i] = -1;
			GItemSlotWasPressed[i] = false;
		}
	}

	if (GItemPagerRow)
	{
		UPanelSlot* PagerSlot = nullptr;
		if (BrowserBox)
			PagerSlot = BrowserBox->AddChild(GItemPagerRow);
		else
			PagerSlot = Container->AddChild(GItemPagerRow);

		if (PagerSlot && PagerSlot->IsA(UVerticalBoxSlot::StaticClass()))
		{
			auto* VSlot = static_cast<UVerticalBoxSlot*>(PagerSlot);
			VSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
			VSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			FMargin Pad{};
			Pad.Left = 0.0f;
			Pad.Top = 14.0f;
			Pad.Right = 12.0f;
			Pad.Bottom = 0.0f;
			VSlot->SetPadding(Pad);
		}
		Count++;
	}

	AddPanelWithFixedGap(BrowserPanel, 0.0f, 8.0f);

	GItemCurrentPage = 0;
	FilterItems(0);
	RefreshItemPage();

	std::cout << "[SDK] Tab1 (Items): " << Count << " widgets, "
	          << GAllItems.size() << " items cached\n";
}
void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->InputSlot, "Tab2(InputSlot)");
	if (!Container) return;
	Container->ClearChildren();
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
			Pad.Top = TopGap;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"战斗开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	AddToggle(SwitchBox, L"伤害加倍");
	AddToggle(SwitchBox, L"招式无视冷却");
	AddToggle(SwitchBox, L"战斗加速");
	AddToggle(SwitchBox, L"不遇敌");
	AddToggle(SwitchBox, L"全队友参战");
	AddToggle(SwitchBox, L"战败视为胜利");
	AddToggle(SwitchBox, L"心法填装最后一格");
	AddToggle(SwitchBox, L"战斗前自动恢复");
	AddToggle(SwitchBox, L"移动速度加倍");
	AddToggle(SwitchBox, L"只对本方生效");
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);

	auto* RatioPanel = CreateCollapsiblePanel(PC, L"倍率与速度");
	auto* RatioBox = RatioPanel ? RatioPanel->CT_Contents : nullptr;
	AddSlider(RatioBox, L"伤害倍率");
	AddSlider(RatioBox, L"战斗加速倍数");
	AddSlider(RatioBox, L"移动倍率");
	AddSlider(RatioBox, L"逃跑成功率");
	AddPanelWithFixedGap(RatioPanel, 0.0f, 10.0f);

	auto* ExtraPanel = CreateCollapsiblePanel(PC, L"额外参数");
	auto* ExtraBox = ExtraPanel ? ExtraPanel->CT_Contents : nullptr;
	AddNumeric(ExtraBox, L"战斗时间流速", L"1");
	AddPanelWithFixedGap(ExtraPanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab2 (Battle): " << Count << " widgets added\n";
}
void PopulateTab_Life(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->LanSlot, "Tab3(LanSlot)");
	if (!Container) return;
	Container->ClearChildren();
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
			Pad.Top = TopGap;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto* SwitchPanel = CreateCollapsiblePanel(PC, L"生活开关");
	auto* SwitchBox = SwitchPanel ? SwitchPanel->CT_Contents : nullptr;
	AddToggle(SwitchBox, L"锻造/制衣/炼丹/烹饪无视要求");
	AddToggle(SwitchBox, L"设置产出数量");
	AddToggle(SwitchBox, L"采集一秒冷却");
	AddToggle(SwitchBox, L"钓鱼只钓稀有物");
	AddToggle(SwitchBox, L"钓鱼收杆必有收获");
	AddToggle(SwitchBox, L"家园随时收获");
	AddPanelWithFixedGap(SwitchPanel, 0.0f, 10.0f);

	auto* OutputPanel = CreateCollapsiblePanel(PC, L"产出与掉落");
	auto* OutputBox = OutputPanel ? OutputPanel->CT_Contents : nullptr;
	AddNumeric(OutputBox, L"产出数量", L"1");
	AddPanelWithFixedGap(OutputPanel, 0.0f, 10.0f);

	auto* MasteryPanel = CreateCollapsiblePanel(PC, L"生活精通");
	auto* MasteryBox = MasteryPanel ? MasteryPanel->CT_Contents : nullptr;
	AddNumeric(MasteryBox, L"锻造精通", L"100");
	AddNumeric(MasteryBox, L"医术精通", L"100");
	AddNumeric(MasteryBox, L"制衣精通", L"100");
	AddNumeric(MasteryBox, L"炼丹精通", L"100");
	AddNumeric(MasteryBox, L"烹饪精通", L"100");
	AddNumeric(MasteryBox, L"采集精通", L"100");
	AddNumeric(MasteryBox, L"钓鱼精通", L"100");
	AddNumeric(MasteryBox, L"饮酒精通", L"100");
	AddNumeric(MasteryBox, L"茶道精通", L"100");
	AddNumeric(MasteryBox, L"口才精通", L"100");
	AddNumeric(MasteryBox, L"书法精通", L"100");
	AddPanelWithFixedGap(MasteryPanel, 0.0f, 10.0f);

	auto* ExpPanel = CreateCollapsiblePanel(PC, L"生活经验");
	auto* ExpBox = ExpPanel ? ExpPanel->CT_Contents : nullptr;
	AddNumeric(ExpBox, L"锻造经验", L"100");
	AddNumeric(ExpBox, L"医术经验", L"100");
	AddNumeric(ExpBox, L"制衣经验", L"100");
	AddNumeric(ExpBox, L"炼丹经验", L"100");
	AddNumeric(ExpBox, L"烹饪经验", L"100");
	AddNumeric(ExpBox, L"采集经验", L"100");
	AddNumeric(ExpBox, L"钓鱼经验", L"100");
	AddNumeric(ExpBox, L"饮酒经验", L"100");
	AddNumeric(ExpBox, L"茶道经验", L"100");
	AddNumeric(ExpBox, L"口才经验", L"100");
	AddNumeric(ExpBox, L"书法经验", L"100");
	AddPanelWithFixedGap(ExpPanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab3 (Life): " << Count << " widgets added\n";
}
void PopulateTab_Social(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->OthersSlot, "Tab4(OthersSlot)");
	if (!Container) return;
	Container->ClearChildren();
	int Count = 0;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = Container->AddChild(Panel);
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

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto* MainPanel = CreateCollapsiblePanel(PC, L"社交开关");
	auto* MainBox = MainPanel ? MainPanel->CT_Contents : nullptr;
	AddToggle(MainBox, L"送礼必定喜欢");
	AddToggle(MainBox, L"邀请无视条件");
	AddToggle(MainBox, L"切磋无视好感");
	AddToggle(MainBox, L"请教无视要求");
	AddToggle(MainBox, L"切磋获得对手背包");
	AddToggle(MainBox, L"NPC装备可脱");
	AddToggle(MainBox, L"NPC无视武器功法限制");
	AddToggle(MainBox, L"强制显示NPC互动");
	AddPanelWithFixedGap(MainPanel, 0.0f, 10.0f);

	auto* GiftPanel = CreateCollapsiblePanel(PC, L"送礼设置");
	auto* GiftBox = GiftPanel ? GiftPanel->CT_Contents : nullptr;
	AddDropdown(GiftBox, L"物品质量(送礼)", { L"全部", L"白", L"绿", L"蓝", L"紫", L"橙", L"红" });
	AddPanelWithFixedGap(GiftPanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab4 (Social): " << Count << " widgets added\n";
}
void PopulateTab_System(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	UPanelWidget* Container = GetOrCreateSlotContainer(CV, CV->GamepadSlot, "Tab5(GamepadSlot)");
	if (!Container) return;
	Container->ClearChildren();
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
			Pad.Top = TopGap;
			Pad.Bottom = BottomGap;
			VSlot->SetPadding(Pad);
		}
		Count++;
	};

	auto AddToggle = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateToggleItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddSlider = [&](UPanelWidget* Box, const wchar_t* Title) {
		auto* Item = CreateVolumeItem(PC, Title);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddDropdown = [&](UPanelWidget* Box, const wchar_t* Title, std::initializer_list<const wchar_t*> Options) {
		auto* Item = CreateVideoItemWithOptions(PC, Title, Options);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};
	auto AddNumeric = [&](UPanelWidget* Box, const wchar_t* Title, const wchar_t* DefaultValue) {
		auto* Item = CreateVolumeNumericEditBoxItem(PC, Outer, Box ? Box : Container, Title, L"输入数字", DefaultValue);
		if (Item)
		{
			if (Box) Box->AddChild(Item); else Container->AddChild(Item);
			Count++;
		}
	};

	auto* MovePanel = CreateCollapsiblePanel(PC, L"移动与跳跃");
	auto* MoveBox = MovePanel ? MovePanel->CT_Contents : nullptr;
	AddToggle(MoveBox, L"空格跳跃");
	AddSlider(MoveBox, L"跳跃速度");
	AddToggle(MoveBox, L"无限跳跃");
	AddToggle(MoveBox, L"奔跑/骑马加速");
	AddSlider(MoveBox, L"加速倍率");
	AddSlider(MoveBox, L"世界移动速度");
	AddSlider(MoveBox, L"场景移动速度");
	AddPanelWithFixedGap(MovePanel, 0.0f, 10.0f);

	auto* MountPanel = CreateCollapsiblePanel(PC, L"坐骑设置");
	auto* MountBox = MountPanel ? MountPanel->CT_Contents : nullptr;
	AddToggle(MountBox, L"坐骑替换");
	AddDropdown(MountBox, L"指定坐骑", { L"黑马", L"白马", L"棕马", L"小毛驴" });
	AddPanelWithFixedGap(MountPanel, 0.0f, 10.0f);

	auto* StoryPanel = CreateCollapsiblePanel(PC, L"开档与解锁");
	auto* StoryBox = StoryPanel ? StoryPanel->CT_Contents : nullptr;
	AddToggle(StoryBox, L"一周目可选极难");
	AddToggle(StoryBox, L"一周目可选传承");
	AddToggle(StoryBox, L"承君传承包括所有");
	AddToggle(StoryBox, L"未交互驿站可用");
	AddToggle(StoryBox, L"激活GM命令行");
	AddToggle(StoryBox, L"解锁全图鉴");
	AddToggle(StoryBox, L"解锁全成就");
	AddPanelWithFixedGap(StoryPanel, 0.0f, 10.0f);

	auto* ScreenPanel = CreateCollapsiblePanel(PC, L"屏幕设置");
	auto* ScreenBox = ScreenPanel ? ScreenPanel->CT_Contents : nullptr;
	AddDropdown(ScreenBox, L"分辨率", { L"1920x1080", L"2560x1440", L"3840x2160" });
	AddDropdown(ScreenBox, L"窗口模式", { L"全屏", L"无边框", L"窗口" });
	AddToggle(ScreenBox, L"垂直同步");
	AddPanelWithFixedGap(ScreenPanel, 0.0f, 10.0f);

	auto* DiffPanel = CreateCollapsiblePanel(PC, L"开档难度系数");
	auto* DiffBox = DiffPanel ? DiffPanel->CT_Contents : nullptr;
	AddNumeric(DiffBox, L"简单系数", L"100");
	AddNumeric(DiffBox, L"普通系数", L"100");
	AddNumeric(DiffBox, L"困难系数", L"100");
	AddNumeric(DiffBox, L"极难系数", L"100");
	AddNumeric(DiffBox, L"敌人伤害系数", L"100");
	AddNumeric(DiffBox, L"敌人气血系数", L"100");
	AddNumeric(DiffBox, L"资源产出系数", L"100");
	AddNumeric(DiffBox, L"经验获取系数", L"100");
	AddPanelWithFixedGap(DiffPanel, 0.0f, 10.0f);

	auto* TitlePanel = CreateCollapsiblePanel(PC, L"称号战力门槛");
	auto* TitleBox = TitlePanel ? TitlePanel->CT_Contents : nullptr;
	AddNumeric(TitleBox, L"称号门槛1", L"100");
	AddNumeric(TitleBox, L"称号门槛2", L"200");
	AddNumeric(TitleBox, L"称号门槛3", L"300");
	AddNumeric(TitleBox, L"称号门槛4", L"400");
	AddNumeric(TitleBox, L"称号门槛5", L"500");
	AddNumeric(TitleBox, L"称号门槛6", L"600");
	AddNumeric(TitleBox, L"称号门槛7", L"700");
	AddNumeric(TitleBox, L"称号门槛8", L"800");
	AddNumeric(TitleBox, L"称号门槛9", L"900");
	AddNumeric(TitleBox, L"称号门槛10", L"1000");
	AddNumeric(TitleBox, L"称号门槛11", L"1100");
	AddPanelWithFixedGap(TitlePanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab5 (System): " << Count << " widgets added\n";
}


void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent6) return;
	GDynTabContent6->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GTeammateFollowToggle = nullptr;
	GTeammateFollowCount = nullptr;
	GTeammateAddDD = nullptr;
	GTeammateReplaceToggle = nullptr;
	GTeammateReplaceDD = nullptr;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTabContent6->AddChild(Panel);
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
	GTeammateFollowToggle = CreateToggleItem(PC, L"设置队友跟随数量");
	if (GTeammateFollowToggle)
	{
		if (TeamBox) TeamBox->AddChild(GTeammateFollowToggle);
		else GDynTabContent6->AddChild(GTeammateFollowToggle);
		Count++;
	}
	GTeammateFollowCount = CreateVolumeNumericEditBoxItem(PC, Outer, TeamBox ? TeamBox : GDynTabContent6, L"跟随数量", L"输入数字", L"3");
	if (GTeammateFollowCount)
	{
		if (TeamBox) TeamBox->AddChild(GTeammateFollowCount);
		else GDynTabContent6->AddChild(GTeammateFollowCount);
		Count++;
	}
	AddPanelWithFixedGap(TeamPanel, 0.0f, 10.0f);

	auto* OperatePanel = CreateCollapsiblePanel(PC, L"队友操作");
	auto* OperateBox = OperatePanel ? OperatePanel->CT_Contents : nullptr;
	GTeammateAddDD = CreateVideoItemWithOptions(PC, L"添加队友",
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢尧", L"唐婉婉", L"徐小小", L"向天笑" });
	if (GTeammateAddDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammateAddDD);
		else GDynTabContent6->AddChild(GTeammateAddDD);
		Count++;
	}
	GTeammateReplaceToggle = CreateToggleItem(PC, L"替换指定队友");
	if (GTeammateReplaceToggle)
	{
		if (OperateBox) OperateBox->AddChild(GTeammateReplaceToggle);
		else GDynTabContent6->AddChild(GTeammateReplaceToggle);
		Count++;
	}
	GTeammateReplaceDD = CreateVideoItemWithOptions(PC, L"指定队友",
		{ L"请选择", L"百里东风", L"尚云溪", L"叶千秋", L"谢尧", L"唐婉婉", L"徐小小", L"向天笑" });
	if (GTeammateReplaceDD)
	{
		if (OperateBox) OperateBox->AddChild(GTeammateReplaceDD);
		else GDynTabContent6->AddChild(GTeammateReplaceDD);
		Count++;
	}
	AddPanelWithFixedGap(OperatePanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab6 (Teammates): " << Count << " widgets added\n";
}


void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent7) return;
	GDynTabContent7->ClearChildren();
	int Count = 0;

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);

	GQuestToggle = nullptr;
	GQuestTypeDD = nullptr;

	auto AddPanelWithFixedGap = [&](UVE_JHVideoPanel2_C* Panel, float TopGap, float BottomGap)
	{
		if (!Panel)
			return;
		UPanelSlot* Slot = GDynTabContent7->AddChild(Panel);
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

	auto* MainPanel = CreateCollapsiblePanel(PC, L"任务执行");
	auto* MainBox = MainPanel ? MainPanel->CT_Contents : nullptr;
	GQuestToggle = CreateToggleItem(PC, L"接到/完成任务");
	if (GQuestToggle)
	{
		if (MainBox) MainBox->AddChild(GQuestToggle);
		else GDynTabContent7->AddChild(GQuestToggle);
		Count++;
	}
	GQuestTypeDD = CreateVideoItemWithOptions(PC, L"执行类型", { L"接到", L"完成" });
	if (GQuestTypeDD)
	{
		if (MainBox) MainBox->AddChild(GQuestTypeDD);
		else GDynTabContent7->AddChild(GQuestTypeDD);
		Count++;
	}
	AddPanelWithFixedGap(MainPanel, 0.0f, 10.0f);

	auto* ArgPanel = CreateCollapsiblePanel(PC, L"任务参数");
	auto* ArgBox = ArgPanel ? ArgPanel->CT_Contents : nullptr;
	auto* QuestIdItem = CreateVolumeNumericEditBoxItem(PC, Outer, ArgBox ? ArgBox : GDynTabContent7, L"任务ID", L"输入数字", L"1");
	if (QuestIdItem)
	{
		if (ArgBox) ArgBox->AddChild(QuestIdItem);
		else GDynTabContent7->AddChild(QuestIdItem);
		Count++;
	}
	AddPanelWithFixedGap(ArgPanel, 0.0f, 8.0f);

	std::cout << "[SDK] Tab7 (Quests): " << Count << " widgets added\n";
}


void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent8 || !CV || !PC) return;
	GDynTabContent8->ClearChildren();

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
		: static_cast<UObject*>(CV);
	UWidget* BtnLayout = nullptr;
	auto* ResetBtn = CreateGameStyleButton(PC, L"下一页", "Tab8Showcase",
		0.0f, 0.0f, &BtnLayout);
	if (BtnLayout)
	{
		GDynTabContent8->AddChild(BtnLayout);
		std::cout << "[SDK] Tab8 (Controls): showcase button added\n";
	}
	else
	{
		std::cout << "[SDK] Tab8 (Controls): failed to create showcase button\n";
	}
}







