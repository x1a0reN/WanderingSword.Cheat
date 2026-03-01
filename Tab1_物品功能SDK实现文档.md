# Tab 1 - 物品 (VideoSlot) SDK 实现文档

> 游戏: 逸剑风云决 (Wandering Sword)
> 引擎: Unreal Engine 4.26.2
> SDK来源: Dumper-7
> 文档版本: 2026-03-02

---

## 一、功能总览

| # | 功能 | 控件类型 | 实现方式 |
|---|------|----------|----------|
| 1 | 物品不减 | 开关 | Hook `UItemManager::ChangeItemNum` / `RemoveBackpackItem` |
| 2 | 物品获得加倍 | 开关 | Hook `UItemManager::AddItem` 的 Num 参数 |
| 3 | 加倍倍数 | 滑块 | 配合功能2，乘以滑块值 |
| 4 | 所有物品可出售 | 开关 | 修改 `FItemInfoSetting.bCantSell` |
| 5 | 包括任务物品 | 开关 | 同上，扩展到 `EItemSubType::QuestItem` |
| 6 | 掉落率100% | 开关 | Hook 掉落池随机逻辑 |
| 7 | 锻造制衣效果加倍 | 开关 | Hook 锻造/制衣产出的 `FActionSetting.Num` |
| 8 | 道具增量效果倍率 | 滑块 | 配合功能7 |
| 9 | 额外效果倍率 | 滑块 | 配合功能7 |
| 10 | 最大额外词条数 | 数值输入 | 修改 `RandRange` / `NormalRandRange` 上限 |
| 11 | 无视物品使用次数 | 开关 | 修改 `FItemInfoSetting.UsedCountLimit` 或跳过检查 |
| 12 | 无视物品使用要求 | 开关 | 清空/跳过 `FItemInfoSetting.Requirements` |
| — | 物品浏览器 | 分类+翻页+网格+数量 | 读取 DataTable + `UItemFuncLib::AddItem` |

---

## 二、核心 SDK 类与结构体

### 2.1 UItemResManager — 物品资源管理器

**头文件**: `SDK/JH_classes.hpp`

```cpp
class UItemResManager final : public UGameInstanceSubsystem
{
public:
    class UDataTable* ItemDataTable;          // +0x30 — 全量物品定义表
    class UDataTable* ItemEffectDataTable;    // +0x38 — 物品效果表
    class UDataTable* DropPoolDataTable;      // +0x40 — 掉落池表
    class UDataTable* RandActionPoolDataTable;// +0x48 — 随机词条池表
};
```

**获取方式**:
```cpp
// 推荐: 通过 ManagerFuncLib 静态函数
UItemResManager* ResMgr = UManagerFuncLib::GetItemResManager();
// RVA: JH-Win64-Shipping.exe+0x13537D0

// 备选: 通过 GObjects 扫描
UClass* Cls = UObject::FindClassFast("ItemResManager");
UItemResManager* ResMgr = static_cast<UItemResManager*>(FindFirstObjectOfClass(Cls));
```

### 2.2 UItemManager — 物品运行时管理器

**头文件**: `SDK/JH_classes.hpp`

```cpp
class UItemManager final : public UGameInstanceSubsystem
{
public:
    // +0x30 ~ +0x87: 内部状态 (padding)
    TArray<UItemInfoSpec*> BackpackItems;  // +0x88 — 背包物品列表
    // +0x98 ~ +0x187: 内部状态 (padding)

    // 核心函数及 RVA:
    UItemInfoSpec* AddItem(int32 ItemDefId, int32 Num, EItemRandPoolType, bool bDrop);
    //   → exe+0x128F160
    bool ChangeItemNum(const FGuid& ID, int32 Num, bool FireEvent);
    //   → exe+0x128F930
    bool RemoveBackpackItem(const FGuid& ID, bool FireEvent);
    //   → exe+0x1290790
    bool SellItem(const FGuid& ID, int32 Num);
    //   → exe+0x1290A0
    bool BuyItem(int32 ItemDefId, int32 Num);
    //   → exe+0x128F7C0
    int32 GetMoney();
    //   → exe+0x12901A0
};
```

**获取方式**:
```cpp
UItemManager* ItemMgr = UManagerFuncLib::GetItemManager();
// RVA: JH-Win64-Shipping.exe+0x13537A0
```

### 2.3 UItemFuncLib — 物品工具库（BlueprintFunctionLibrary）

**头文件**: `SDK/JH_classes.hpp`

这是一个纯静态的蓝图函数库，所有方法都通过 CDO ProcessEvent 调用：

```cpp
class UItemFuncLib final : public UBlueprintFunctionLibrary
{
public:
    // —— 添加物品（最常用的作弊入口） ——
    static void AddItem(int32 ItemDefId, int32 Num);
    //   RVA: exe+0x128F0A0
    //   实现: 通过 ProcessEvent 调用到 UItemManager::AddItem

    static void AddMoney(int32 Money);                    // exe+0x128F460
    static bool HasItem(int32 ItemDefId);                 // exe+0x1290370
    static int32 GetItemNum(int32 ItemDefId);             // exe+0x1290080
    static FText GetItemName(int32 ItemDefId);            // exe+0x128FF90
    static EItemSubType GetItemSubType(int32 ItemDefId);  // exe+0x1290110
    static FLinearColor GetColorByQuality(EItemQuality);  // exe+0x128FD40
    static bool CanRemoveItem(EItemSubType);              // exe+0x128F8B0
    static UItemInfoSpec* MakeItemInfoSpec(int32 DefId, int32 Num, EItemRandPoolType);
    //   RVA: exe+0x1290580
};
```

**调用模式** (SDK ProcessEvent 包装):
```cpp
void UItemFuncLib::AddItem(int32 ItemDefId, int32 Num)
{
    static UFunction* Func = nullptr;
    if (!Func)
        Func = StaticClass()->GetFunction("ItemFuncLib", "AddItem");

    Params::ItemFuncLib_AddItem Parms{};
    Parms.ItemDefId = ItemDefId;
    Parms.Num = Num;

    auto Flgs = Func->FunctionFlags;
    Func->FunctionFlags |= 0x400;  // FUNC_Native
    GetDefaultObj()->ProcessEvent(Func, &Parms);
    Func->FunctionFlags = Flgs;
}
```

### 2.4 FItemInfoSetting — 物品定义结构体 (DataTable 行)

**头文件**: `SDK/JH_structs.hpp`

```cpp
struct FItemInfoSetting final : public FTableRowBase
{
    // +0x08: int32 ID                    — 物品定义ID
    // +0x0C: int32 EquipmentRank         — 装备等阶
    // +0x10: FText Name                  — 物品名称 (0x18字节)
    // +0x28: FText Description           — 物品描述 (0x18字节)
    // +0x40: EItemSubType ItemType       — 物品子类型 (1字节)
    // +0x44: float CooldownTime          — 使用冷却
    // +0x48: float FightCooldownTime     — 战斗冷却
    // +0x50: TSoftObjectPtr<UTexture2D> Icon — 图标引用 (0x28字节)
    // +0x78: float BuyPrice              — 购买价格
    // +0x7C: float SellPrice             — 出售价格
    // +0x80: EItemQuality Quality        — 品质 (1字节)
    // +0x81: bool bCantGift              — 不可赠送
    // +0x82: bool bCantUse               — 不可使用
    // +0x83: bool bCantSell              — 不可出售 ← [所有物品可出售] 修改目标
    // +0x84: bool bCanStack              — 可堆叠
    // +0x85: bool bCantTakeOff           — 不可脱下
    // +0x86: bool bDontCheckHPMP         — 不检查HP/MP
    // +0x87: bool bCantTakeFromCompare   — 不可从对比拿取
    // +0x88: bool bOpenForPlayer         — 对玩家开放
    // +0x90: TArray<FRequirementSetting> Requirements  — 使用条件 ← [无视使用要求] 修改目标
    // +0xA0: TArray<FActionSetting> Actions             — 使用效果
    // +0xB0: int32 UsedCountLimit        — 使用次数上限 ← [无视使用次数] 修改目标
    // +0xB8: TArray<int32> RandActionPools     — 随机词条池ID列表
    // +0xC8: TArray<int32> RandRange           — 随机词条数量范围 ← [最大额外词条] 修改目标
    // +0xD8: TArray<int32> NormalRandActionPools — 普通随机词条池
    // +0xE8: TArray<int32> NormalRandRange       — 普通随机数量范围
};
```

### 2.5 UItemInfoSpec — 物品实例（背包中的具体物品）

```cpp
class UItemInfoSpec final : public UObject
{
public:
    FGuid ID;               // +0x28 — 唯一实例ID
    bool bIsNew;            // +0x38
    bool bIsFocus;          // +0x39
    bool bOnlyForLayout;    // +0x3A
    bool bDecompoSelected;  // +0x3B
    int32 ItemDefId;        // +0x3C — 物品定义ID (关联 FItemInfoSetting.ID)
    int32 Num;              // +0x40 — 当前数量 ← [物品不减] 保护目标
    bool bApplied;          // +0x44
    bool bIsLocked;         // +0x45
    bool bIsMakeFromTable;  // +0x46
    TArray<FActionSetting> Actions;     // +0x48 — 固定效果
    TArray<FActionSetting> RandActions; // +0x58 — 随机词条效果
    int32 GainTime;         // +0x68

    FItemInfoSetting GetDef(); // exe+0x1294810 — 获取定义
};
```

---

## 三、枚举类型速查

### EItemSubType (物品子类型)

```
0    = None
1-6  = 武器 (Sword/Sabre/Spear/Fist/HiddenWeapon/OtherWeapons)
10-13= 防具 (Clothing/Hat/Shoes/Ornament)
20-29= 消耗品 (Pill/Drug/Drug_Stamina/Drug_Health/Drug_Mana/CookedFood)
30-38= 秘籍 (SwordBook ~ NoteBook)
40-45= 材料 (Mineral/Plant/Food/Leather/Worm/Wood)
50-56= 珍玩 (Paint/Chess/TeaSet/Book/Lyra/Treasure/Flower)
60   = 任务物品 (QuestItem)
70   = 信件 (Letter)
99   = 配方 (Recipe)
```

### EItemQuality (品质等级)

```
0 = None    1 = White(白)   2 = Green(绿)   3 = Blue(蓝)
4 = Gold(金) 5 = DarkGold(暗金) 6 = Red(红)
```

### EItemRandPoolType (随机词条池类型)

```
0 = None    1 = Normal(普通)   2 = Fusion(融合)
```

---

## 四、各功能详细实现方案

### 4.1 物品不减

**原理**: 当物品使用/消耗时，游戏调用 `UItemManager::ChangeItemNum(FGuid& ID, int32 Num, bool FireEvent)` 来减少数量，或调用 `RemoveBackpackItem` 直接移除。

**实现方案 A — Hook ChangeItemNum (推荐)**:
```cpp
// 原函数签名 (RVA: 0x128F930)
// bool UItemManager::ChangeItemNum(const FGuid& ID, int32 Num, bool FireEvent)
// 当 Num < 0 时为消耗

using ChangeItemNumFn = bool(__fastcall*)(UItemManager*, const FGuid&, int32, bool);
ChangeItemNumFn OrigChangeItemNum = nullptr;

bool __fastcall HookedChangeItemNum(UItemManager* This, const FGuid& ID, int32 Num, bool FireEvent)
{
    if (bItemNoDecrease && Num < 0)
        return true;  // 拦截消耗，返回成功但不执行
    return OrigChangeItemNum(This, ID, Num, FireEvent);
}
```

**实现方案 B — 修改 UItemInfoSpec::Num**:
每帧轮询 `UItemManager::BackpackItems` (+0x88)，将所有物品的 `Num` (+0x40) 强制保持为大值。

### 4.2 物品获得加倍 & 加倍倍数

**原理**: Hook `UItemFuncLib::AddItem` 或 `UItemManager::AddItem`，将 Num 参数乘以倍率。

```cpp
// UItemFuncLib::AddItem 的 Native 实现 RVA: 0x128F0A0
// 实际会转调 UItemManager::AddItem (RVA: 0x128F160)

using AddItemFn = UItemInfoSpec*(__fastcall*)(UItemManager*, int32, int32, EItemRandPoolType, bool);
AddItemFn OrigAddItem = nullptr;

UItemInfoSpec* __fastcall HookedAddItem(UItemManager* This, int32 DefId, int32 Num,
                                         EItemRandPoolType PoolType, bool bDrop)
{
    if (bItemGainMultiplier)
        Num *= iMultiplierValue;  // 从滑块读取的倍率
    return OrigAddItem(This, DefId, Num, PoolType, bDrop);
}
```

**滑块值读取**: 通过 `GVolumeItems` 数组中名为"加倍倍数"的滑块项的 `VolumeSlider->GetValue()` 获取归一化值 [0,1]，映射到实际倍率 (如 1~100)。

### 4.3 所有物品可出售 & 包括任务物品

**原理**: 遍历 `ItemDataTable` 的 RowMap，将每行数据的 `bCantSell` (+0x83) 设为 `false`。

```cpp
void MakeAllItemsSellable(bool includeQuestItems)
{
    UItemResManager* ResMgr = UManagerFuncLib::GetItemResManager();
    if (!ResMgr || !ResMgr->ItemDataTable) return;

    auto& RowMap = ResMgr->ItemDataTable->RowMap;
    int32 Allocated = RowMap.NumAllocated();

    for (int32 i = 0; i < Allocated; i++)
    {
        if (!RowMap.IsValidIndex(i)) continue;
        uint8* RowData = RowMap[i].Value();
        if (!RowData) continue;

        EItemSubType SubType = *reinterpret_cast<EItemSubType*>(RowData + 0x40);

        // 默认跳过任务物品，除非勾选"包括任务物品"
        if (!includeQuestItems && SubType == EItemSubType::QuestItem)
            continue;

        // FItemInfoSetting.bCantSell at +0x83
        *reinterpret_cast<bool*>(RowData + 0x83) = false;

        // 可选: 给无售价物品设置一个基础售价
        float* SellPrice = reinterpret_cast<float*>(RowData + 0x7C);
        if (*SellPrice <= 0.0f)
            *SellPrice = 1.0f;
    }
}
```

### 4.4 掉落率100%

**原理**: 游戏掉落系统使用 `DropPoolDataTable` 中的 `FDropPoolSetting`，每个池包含一组 `FDropWeight` (物品ID + 权重)。掉落时通过 `UManagerFuncLib::RandomByWeight` (RVA: 0x1354880) 做加权随机。

**实现方案**: Hook 随机函数或直接修改掉落池权重，使所有物品都必定掉落。

```cpp
// FDropPoolSetting 结构:
// +0x08: int32 ID
// +0x10: TArray<FDropWeight> DropWeights
//
// FDropWeight 结构:
// +0x00: int32 ItemDefId
// +0x04: int32 Weight

// 方案A: Hook RandomByWeight 使其始终返回期望索引
// 方案B: 修改所有掉落池的 Weight 为相等值，确保均匀分布
// 方案C: Hook 掉落逻辑使其将池中所有物品全部掉落
```

### 4.5 锻造制衣效果加倍 & 道具增量/额外效果倍率

**原理**: 锻造/制衣的产出效果定义在 `FActionSetting` 中:

```cpp
struct FActionSetting
{
    EActionType Type;  // +0x00 — 效果类型
    int32 ID;          // +0x04 — 关联ID
    float Num;         // +0x08 — 效果数值 ← 修改此值实现加倍
};
```

当 `EActionType` 为 `CItem (1)` 时表示产出物品，`Num` 为产出数量。

**实现**: Hook 制作完成时的效果应用逻辑，将 `FActionSetting.Num` 乘以倍率:
```cpp
// 道具增量效果倍率: 乘以 Actions 中 Type=CItem 的 Num
// 额外效果倍率: 乘以 RandActions 中各效果的 Num
```

### 4.6 最大额外词条数

**原理**: `FItemInfoSetting` 的 `RandRange` (+0xC8) 和 `NormalRandRange` (+0xE8) 定义了随机词条的数量范围 `[Min, Max]`。

```cpp
void SetMaxExtraAffixes(int32 MaxAffixes)
{
    UItemResManager* ResMgr = UManagerFuncLib::GetItemResManager();
    if (!ResMgr || !ResMgr->ItemDataTable) return;

    auto& RowMap = ResMgr->ItemDataTable->RowMap;
    for (int32 i = 0; i < RowMap.NumAllocated(); i++)
    {
        if (!RowMap.IsValidIndex(i)) continue;
        uint8* Row = RowMap[i].Value();
        if (!Row) continue;

        // RandRange at +0xC8: TArray<int32>
        auto* RandRange = reinterpret_cast<TArray<int32>*>(Row + 0xC8);
        if (RandRange->Num() >= 2)
        {
            (*RandRange)[0] = MaxAffixes; // Min
            (*RandRange)[1] = MaxAffixes; // Max
        }

        // NormalRandRange at +0xE8: TArray<int32>
        auto* NormalRange = reinterpret_cast<TArray<int32>*>(Row + 0xE8);
        if (NormalRange->Num() >= 2)
        {
            (*NormalRange)[0] = MaxAffixes;
            (*NormalRange)[1] = MaxAffixes;
        }
    }
}
```

### 4.7 无视物品使用次数

**原理**: `FItemInfoSetting.UsedCountLimit` (+0xB0) 定义了物品可使用的最大次数。设为 0 或极大值即可。

```cpp
// 方案A: 修改 DataTable
// 遍历 ItemDataTable, 将每行 +0xB0 的 UsedCountLimit 设为 0 (无限)
*reinterpret_cast<int32*>(RowData + 0xB0) = 0;

// 方案B: Hook 使用次数检查函数, 直接跳过检查
```

### 4.8 无视物品使用要求

**原理**: `FItemInfoSetting.Requirements` (+0x90) 是 `TArray<FRequirementSetting>`，每个条件包含:

```cpp
struct FRequirementSetting
{
    ERequirementType Type;  // +0x00 — 条件类型
    int32 ID;               // +0x04 — 条件关联ID
    float Num;              // +0x08 — 条件数值
};
```

**实现**: 清空 Requirements 数组或 Hook 条件检查函数使其始终返回 true:
```cpp
// 方案A: 修改 DataTable, 清空 Requirements
auto* Reqs = reinterpret_cast<TArray<FRequirementSetting>*>(RowData + 0x90);
Reqs->Reset();  // 清空数组

// 方案B: Hook 需求检查函数, 始终返回满足
```

---

## 五、物品浏览器实现详解

### 5.1 数据源: ItemDataTable 解析

物品浏览器从 `UItemResManager::ItemDataTable` 中读取全量物品数据。

**完整的解析流程**:

```cpp
void BuildItemCache()
{
    // 1. 获取 UItemResManager 实例
    UItemResManager* ResMgr = UManagerFuncLib::GetItemResManager();
    // 备选: 通过 GObjects 扫描
    if (!ResMgr) {
        UClass* Cls = UObject::FindClassFast("ItemResManager");
        ResMgr = static_cast<UItemResManager*>(FindFirstObjectOfClass(Cls));
    }

    // 2. 获取 ItemDataTable (位于 UItemResManager+0x30)
    UDataTable* DataTable = ResMgr->ItemDataTable;

    // 3. 遍历 RowMap
    auto& RowMap = DataTable->RowMap;
    int32 AllocatedSlots = RowMap.NumAllocated();

    for (int32 i = 0; i < AllocatedSlots; i++)
    {
        if (!RowMap.IsValidIndex(i)) continue;
        uint8* RowData = RowMap[i].Value();
        if (!RowData) continue;

        // 4. 读取 FItemInfoSetting 字段
        int32 DefId    = *reinterpret_cast<int32*>(RowData + 0x08);
        uint8 SubType  = *reinterpret_cast<uint8*>(RowData + 0x40);
        uint8 Quality  = *reinterpret_cast<uint8*>(RowData + 0x80);

        // 读取名称 (FText at +0x10, 前8字节是 FTextData*)
        auto* TextData = *reinterpret_cast<FTextImpl::FTextData**>(RowData + 0x10);
        const wchar_t* Name = TextData ? TextData->TextSource.CStr() : L"";

        // 读取描述 (FText at +0x28)
        auto* DescData = *reinterpret_cast<FTextImpl::FTextData**>(RowData + 0x28);
        const wchar_t* Desc = DescData ? DescData->TextSource.CStr() : L"";

        // 读取图标 (TSoftObjectPtr<UTexture2D> at +0x50, 0x28字节)
        uint8 IconData[0x28];
        memcpy(IconData, RowData + 0x50, 0x28);
        // 判断图标是否有效: FSoftObjectPath.AssetPathName 位于 TSoftObjectPtr+0x10
        FName* IconName = reinterpret_cast<FName*>(IconData + 0x10);
        bool HasIcon = !IconName->IsNone();

        // 5. 存入缓存
        // ...
    }

    // 6. 按 DefId 排序
    std::sort(items.begin(), items.end(),
        [](auto& a, auto& b) { return a.DefId < b.DefId; });
}
```

### 5.2 图标加载

图标使用 `TSoftObjectPtr<UTexture2D>` 延迟加载机制:

```cpp
UTexture2D* ResolveTextureFromSoftData(const uint8* SoftTextureData28)
{
    // 1. 直接尝试 TSoftObjectPtr::Get()
    TSoftObjectPtr<UTexture2D> SoftTexture{};
    memcpy(&SoftTexture, SoftTextureData28, sizeof(SoftTexture));
    UTexture2D* Texture = SoftTexture.Get();

    // 2. 若未加载, 调用同步加载
    if (!Texture)
    {
        TSoftObjectPtr<UObject> SoftObject{};
        memcpy(&SoftObject, SoftTextureData28, sizeof(SoftObject));
        UObject* Loaded = UKismetSystemLibrary::LoadAsset_Blocking(SoftObject);
        if (Loaded && Loaded->IsA(UTexture2D::StaticClass()))
            Texture = static_cast<UTexture2D*>(Loaded);
    }

    return Texture;
}

// 设置到 UImage 控件:
ImageWidget->SetBrushFromTexture(Texture, true);
```

### 5.3 分类过滤

按 `EItemSubType` 进行分类:

```cpp
void FilterItems(int32 category)
{
    switch (category)
    {
    case 0: /* 全部 */         match = true; break;
    case 1: /* 武器 */         match = (SubType >= 1  && SubType <= 6);  break;
    case 2: /* 防具 */         match = (SubType >= 10 && SubType <= 13); break;
    case 3: /* 消耗品 */       match = (SubType >= 14 && SubType <= 17); break;
    default: /* 其他 */        match = (SubType == 0 || SubType > 17);   break;
    }
}
```

### 5.4 添加物品到背包

点击物品格子时，通过 `UItemFuncLib::AddItem` 添加:

```cpp
// UItemFuncLib::AddItem 是静态蓝图函数，通过 CDO ProcessEvent 调用
UItemFuncLib::AddItem(DefId, Quantity);
// 内部流程: CDO->ProcessEvent(UFunction"AddItem", {DefId, Num})
//         → Native 实现 (RVA: 0x128F0A0)
//         → UItemManager::AddItem (RVA: 0x128F160)
```

### 5.5 品质边框显示

使用游戏原生的纹理加载器获取品质边框:

```cpp
// UJHNeoUITextureLoader 或 Console 版本
UTexture2D* QBorderTex = UJHNeoUITextureLoader::JHIcon_Item_QualityBorder(QualityLevel);
if (!QBorderTex)
    QBorderTex = UJHNeoUITextureLoader_Console::JHIcon_C_Item_QualityBorder(QualityLevel);

// 主边框颜色 (使用 ItemFuncLib)
FLinearColor BorderColor = UItemFuncLib::GetColorByQuality(Quality);
MainBorderImage->SetColorAndOpacity(BorderColor);
```

---

## 六、关键 RVA 地址汇总

| 函数 | RVA (exe+) | 说明 |
|------|-----------|------|
| `UItemFuncLib::AddItem` | `0x128F0A0` | 添加物品 (BP包装) |
| `UItemManager::AddItem` | `0x128F160` | 添加物品 (Native) |
| `UItemManager::ChangeItemNum` | `0x128F930` | 修改物品数量 |
| `UItemManager::RemoveBackpackItem` | `0x1290790` | 移除背包物品 |
| `UItemManager::SellItem` | `0x1290A80` | 出售物品 |
| `UItemManager::BuyItem` | `0x128F7C0` | 购买物品 |
| `UItemManager::GetMoney` | `0x12901A0` | 获取金钱 |
| `UItemFuncLib::AddMoney` | `0x128F460` | 添加金钱 |
| `UItemFuncLib::GetItemNum` | `0x1290080` | 获取物品数量 |
| `UItemFuncLib::HasItem` | `0x1290370` | 检查是否拥有物品 |
| `UItemFuncLib::GetItemName` | `0x128FF90` | 获取物品名称 |
| `UItemFuncLib::GetColorByQuality` | `0x128FD40` | 获取品质颜色 |
| `UItemFuncLib::MakeItemInfoSpec` | `0x1290580` | 创建物品实例 |
| `UItemInfoSpec::GetDef` | `0x1294810` | 获取物品定义 |
| `UManagerFuncLib::GetItemManager` | `0x13537A0` | 获取运行时管理器 |
| `UManagerFuncLib::GetItemResManager` | `0x13537D0` | 获取资源管理器 |
| `UManagerFuncLib::RandomByWeight` | `0x1354880` | 加权随机 |

> 以上 RVA 基于游戏版本 V1.24.32，由 Dumper-7 从运行时获取，可直接用于 Hook 定位。

---

## 七、掉落系统相关结构体

```cpp
// 掉落池定义 (DataTable 行)
struct FDropPoolSetting : public FTableRowBase
{
    int32 ID;                              // +0x08
    TArray<FDropWeight> DropWeights;       // +0x10
};

// 单条掉落权重
struct FDropWeight
{
    int32 ItemDefId;   // +0x00 — 掉落物品ID
    int32 Weight;      // +0x04 — 权重值
};

// NPC 掉落条件
struct FNPCDropItemWithRequirement
{
    TArray<FRequirementSetting> Requirements; // +0x00 — 掉落条件
    int32 ItemDefId;                          // +0x10 — 掉落物品ID
};

// 随机词条池 (DataTable 行)
struct FRandActionPoolSetting : public FTableRowBase
{
    int32 ID;                                 // +0x08
    TArray<FActionWeight> ActionWeights;      // +0x10
};

// 带权重的效果
struct FActionWeight
{
    int32 Weight;           // +0x00
    FActionSetting Action;  // +0x04 (0x0C字节)
};
```

---

## 八、UI 布局结构

物品 Tab 的 UI 层级:

```
VideoSlot (Container)
├── OptionsPanelRoot (VBox)
│   ├── CollapsiblePanel "基础开关"
│   │   ├── Toggle "物品不减"
│   │   ├── Toggle "物品获得加倍"
│   │   ├── Toggle "所有物品可出售"
│   │   ├── Toggle "包括任务物品"
│   │   ├── Toggle "掉落率100%"
│   │   └── Toggle "锻造制衣效果加倍"
│   ├── CollapsiblePanel "倍率设置"
│   │   ├── Slider "加倍倍数"
│   │   ├── Slider "道具增量效果倍率"
│   │   └── Slider "额外效果倍率"
│   └── CollapsiblePanel "限制与词条"
│       ├── NumericEdit "最大额外词条数" (默认3)
│       ├── Toggle "无视物品使用次数"
│       └── Toggle "无视物品使用要求"
└── CollapsiblePanel "物品浏览器"
    ├── VideoItem (搜索框 + 分类下拉)
    ├── UniformGridPanel (6×4 物品格子)
    │   └── BPEntry_Item_C × 24 (游戏原生背包格子控件)
    └── HBox (上一页 | 页码 | 下一页)
```

---

## 九、GC 安全与对象生命周期

在 UE4 中操作 UObject 时必须注意 GC:

1. **MarkAsGCRoot**: 为外部持有的 UObject 添加 GC Root 引用，防止被回收
2. **ClearGCRoot**: 不再使用时移除引用
3. **IsSafeLiveObject**: 每帧检查对象是否仍然有效
4. **IsA / IsDefaultObject**: 避免误操作 CDO (Class Default Object)

```cpp
// 创建物品 Tip Widget 时标记为 GC Root
MarkAsGCRoot(static_cast<UObject*>(Created));

// 销毁时清除
ClearGCRoot(static_cast<UObject*>(Widget));

// 每帧使用前检查
if (!IsSafeLiveObject(static_cast<UObject*>(Widget)))
    return;
```

---

## 十、注意事项

1. **版本兼容**: 所有偏移量和 RVA 基于 V1.24.32，游戏更新后需重新 dump SDK 并验证。
2. **DataTable 修改时机**: 修改 DataTable 行数据最好在游戏初始化完成后、物品使用前进行，避免竞态。
3. **ProcessEvent 线程安全**: 所有 `ProcessEvent` 调用必须在游戏主线程（通常在 PostRender Hook 回调中）执行。
4. **图标缓存清理**: 地图切换后旧的 UTexture2D 指针可能失效，需要在 World 变化时清空缓存。
5. **物品数量溢出**: AddItem 的数量参数为 int32，设置过大可能导致 UI 显示异常。
