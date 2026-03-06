#pragma once
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  CheatState.hpp  —  逸剑风云决 内部作弊模块 · 全局状态声明
//
//  本头文件集中管理所有运行时跨模块共享的全局变量、结构体定义
//  与 Inline Hook 功能接口。各 Tab 页面、FrameHook 帧轮询、
//  面板管理器等模块均依赖此处的声明。
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// ── 标准库 ──
#include <Windows.h>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

// ── SDK 引用 ──
#include "SDK/Engine_classes.hpp"
#include "SDK/UMG_classes.hpp"
#include "SDK/NeoUI_classes.hpp"
#include "SDK/BPMV_ConfigView2_classes.hpp"
#include "SDK/BP_JHConfigTabBtn_classes.hpp"
#include "SDK/JHGPCBtn_classes.hpp"
#include "SDK/JHGPCBtn_ActiveBG_classes.hpp"
#include "SDK/JHCommon_Btn_Free_classes.hpp"
#include "SDK/JH_classes.hpp"
#include "SDK/BPVE_JHConfigVideoItem2_classes.hpp"
#include "SDK/BPVE_JHConfigVolumeItem2_classes.hpp"
#include "VTHook.hpp"

using namespace SDK;

// ┌─────────────────────────────────────────────────────────────┐
// │                    类型别名 · 全局常量                       │
// └─────────────────────────────────────────────────────────────┘

/// PostRender 渲染回调的原始函数签名 (x64 __fastcall)
using GVCPostRenderFn = void(__fastcall*)(void* /* this */, void* /* Canvas */);

/// 作弊面板相对于游戏原生设置面板的整体缩放比例
inline constexpr float kInternalPanelScale = 1.1f;

/// UBP_JHConfigTabBtn_C 内部指向父级 ConfigView 实例的偏移量
inline constexpr uintptr_t kConfigTabBtnParentCtxOffset = 0x02E0;

/// 子模块面板 (如 VideoPanel2) 内部指向父级 ConfigView 的偏移量
inline constexpr uintptr_t kConfigModuleView2ParentCtxOffset = 0x03B0;

/// 物品浏览器每页网格的列数、行数与单页总槽位数
inline constexpr int32 kItemGridCols   = 8;
inline constexpr int32 kItemGridRows   = 5;
inline constexpr int32 kItemsPerPage   = kItemGridCols * kItemGridRows;  // 40

// ┌─────────────────────────────────────────────────────────────┐
// │                    核心数据结构定义                          │
// └─────────────────────────────────────────────────────────────┘

/// 从游戏 DataTable 解析并缓存的单条物品信息
struct CachedItem
{
    int32    DefId;            // 物品定义 ID
    wchar_t  Name[64];        // 显示名称
    wchar_t  Desc[256];       // 描述文本
    uint8    Quality;         // 品质等级 (白/绿/蓝/紫/金)
    uint8    SubType;         // 子类型 (用于分类筛选)
    uint8    IconData[0x28];  // TSoftObjectPtr<UTexture2D> 原始字节
    bool     HasIcon;         // 是否包含有效图标引用
};

/// 面板关闭后的 UI 状态记忆快照 (不落盘，仅进程内)
struct UIRememberState
{
    // 通用控件状态 —— 以控件标题为键
    std::unordered_map<std::wstring, int32>        ComboIndexByTitle;
    std::unordered_map<std::wstring, float>        SliderValueByTitle;
    std::unordered_map<std::wstring, std::wstring> EditTextByTitle;

    // 物品浏览器专用记忆
    int32        ItemCategoryIndex = 0;
    int32        ItemCurrentPage   = 0;
    int32        ItemAddQuantity   = 1;
    std::wstring ItemSearchText;
};

/// 物品浏览器完整运行时状态
struct ItemBrowserState
{
    // ── 数据层 ──
    std::vector<CachedItem> AllItems;          // 全量物品缓存
    std::vector<int32>      FilteredIndices;   // 当前筛选结果索引
    bool                    CacheBuilt = false;
    int32                   CurrentPage = 0;
    int32                   TotalPages  = 0;

    // ── 控件引用 ──
    UBPVE_JHConfigVideoItem2_C* CategoryDD   = nullptr;   // 分类下拉框
    int32                       LastCatIdx   = -1;
    UHorizontalBox*             PagerRow     = nullptr;    // 翻页栏容器
    UJHCommon_Btn_Free_C*       PrevPageBtn  = nullptr;
    UJHCommon_Btn_Free_C*       NextPageBtn  = nullptr;
    UTextBlock*                 PageLabel    = nullptr;
    bool                        PrevWasPressed = false;
    bool                        NextWasPressed = false;
    UHorizontalBox*             QuantityRow  = nullptr;    // 数量输入栏
    UEditableTextBox*           QuantityEdit = nullptr;
    int32                       AddQuantity  = 1;
    UWidget*                    GridPanel    = nullptr;    // 网格面板根
    UListView*                  ListView     = nullptr;

    // ── 槽位数组 (固定 kItemsPerPage 大小) ──
    UButton*      SlotButtons       [kItemsPerPage] = {};
    UImage*       SlotImages        [kItemsPerPage] = {};
    UImage*       SlotQualityBorders[kItemsPerPage] = {};
    UUserWidget*  SlotEntryWidgets  [kItemsPerPage] = {};
    int32         SlotItemIndices   [kItemsPerPage] = {};
    bool          SlotWasPressed    [kItemsPerPage] = {};

    // ── 悬浮提示 ──
    int32                HoveredSlot      = -1;
    UJHNeoUITipsVEBase*  HoverTipsWidget  = nullptr;
    UItemInfoSpec*       HoverTempSpec    = nullptr;
};

// ┌─────────────────────────────────────────────────────────────┐
// │               Tab 页面 UI 控件引用结构体                    │
// └─────────────────────────────────────────────────────────────┘

/// Tab1「物品」页功能开关与滑块
struct Tab1Controls
{
    UBPVE_JHConfigVideoItem2_C*  ItemNoDecreaseToggle         = nullptr;  // 物品不减
    UBPVE_JHConfigVideoItem2_C*  ItemGainMultiplierToggle     = nullptr;  // 获取倍数开关
    UBPVE_JHConfigVolumeItem2_C* ItemGainMultiplierSlider     = nullptr;  // 获取倍数滑块
    UBPVE_JHConfigVideoItem2_C*  AllItemsSellableToggle       = nullptr;  // 全部可售
    UBPVE_JHConfigVideoItem2_C*  DropRate100Toggle            = nullptr;  // 掉率 100%
    UBPVE_JHConfigVideoItem2_C*  CraftEffectMultiplierToggle  = nullptr;  // 制造倍率开关
    UBPVE_JHConfigVolumeItem2_C* CraftItemIncrementSlider     = nullptr;  // 制造数量滑块
    UBPVE_JHConfigVolumeItem2_C* CraftExtraEffectSlider       = nullptr;  // 额外效果滑块
    UBPVE_JHConfigVideoItem2_C*  MaxExtraAffixesToggle        = nullptr;  // 最大额外词条
    UBPVE_JHConfigVideoItem2_C*  IgnoreItemUseCountToggle     = nullptr;  // 无视使用次数
    UBPVE_JHConfigVideoItem2_C*  IgnoreItemRequirementsToggle = nullptr;  // 无视使用条件
};

/// Tab2「战斗」页功能开关与滑块
struct Tab2Controls
{
    UBPVE_JHConfigVideoItem2_C*  DamageBoostToggle          = nullptr;  // 伤害加倍
    UBPVE_JHConfigVideoItem2_C*  SkillNoCooldownToggle      = nullptr;  // 招式无 CD
    UBPVE_JHConfigVideoItem2_C*  NoEncounterToggle           = nullptr;  // 不遇敌
    UBPVE_JHConfigVideoItem2_C*  AllTeammatesInFightToggle   = nullptr;  // 全员参战
    UBPVE_JHConfigVideoItem2_C*  DefeatAsVictoryToggle       = nullptr;  // 败犹为胜
    UBPVE_JHConfigVideoItem2_C*  NeiGongFillLastSlotToggle   = nullptr;  // 心法破限
    UBPVE_JHConfigVideoItem2_C*  AutoRecoverHpMpToggle       = nullptr;  // 自动回复
    UBPVE_JHConfigVideoItem2_C*  TotalMoveSpeedToggle        = nullptr;  // 移速加倍
    UBPVE_JHConfigVideoItem2_C*  DamageFriendlyOnlyToggle    = nullptr;  // 仅友方加速
    UBPVE_JHConfigVolumeItem2_C* DamageMultiplierSlider      = nullptr;  // 伤害倍率
    UBPVE_JHConfigVolumeItem2_C* MoveSpeedMultiplierSlider   = nullptr;  // 移速倍率
};

/// Tab3「生活」页功能开关与滑块
struct Tab3Controls
{
    UBPVE_JHConfigVideoItem2_C*  CraftIgnoreRequirementsToggle = nullptr;  // 锻造/制衣/炼丹/烹饪无视要求

    UBPVE_JHConfigVolumeItem2_C* CraftOutputQuantityEdit       = nullptr;  // 产出数量 (Numeric EditBox)
    UBPVE_JHConfigVideoItem2_C*  GatherCooldownToggle          = nullptr;  // 采集一秒冷却
    UBPVE_JHConfigVideoItem2_C*  FishRareOnlyToggle            = nullptr;  // 钓鱼只钓稀有物
    UBPVE_JHConfigVideoItem2_C*  FishAlwaysCatchToggle         = nullptr;  // 钓鱼收杆必有收获
    UBPVE_JHConfigVideoItem2_C*  HomelandHarvestToggle         = nullptr;  // 家园随时收获
};

/// Tab4「社交」页功能开关
struct Tab4Controls
{
    UBPVE_JHConfigVideoItem2_C*  GiftAlwaysLikedToggle         = nullptr;  // 送礼必定喜欢
    UBPVE_JHConfigVideoItem2_C*  InviteIgnoreToggle            = nullptr;  // 邀请无视条件
    UBPVE_JHConfigVideoItem2_C*  SparIgnoreFavorToggle         = nullptr;  // 切磋无视好感
    UBPVE_JHConfigVideoItem2_C*  ConsultIgnoreToggle           = nullptr;  // 请教无视要求
    UBPVE_JHConfigVideoItem2_C*  SparGetLootToggle             = nullptr;  // 切磋获得对手背包
    UBPVE_JHConfigVideoItem2_C*  NpcEquipRemovableToggle       = nullptr;  // NPC装备可脱
    UBPVE_JHConfigVideoItem2_C*  NpcIgnoreWeaponLimitToggle    = nullptr;  // NPC无视武器功法限制
    UBPVE_JHConfigVideoItem2_C*  ForceNpcInteractionToggle     = nullptr;  // 强制显示NPC互动
    UBPVE_JHConfigVideoItem2_C*  GiftQualityDD                 = nullptr;  // 物品质量(送礼)
};

/// Tab5「系统」页功能开关与滑块
struct Tab5Controls
{
    UBPVE_JHConfigVideoItem2_C*  SpaceJumpToggle               = nullptr;  // 空格跳跃
    UBPVE_JHConfigVolumeItem2_C* JumpSpeedSlider               = nullptr;  // 跳跃速度
    UBPVE_JHConfigVideoItem2_C*  InfiniteJumpToggle            = nullptr;  // 无限跳跃
    UBPVE_JHConfigVideoItem2_C*  RunMountSpeedToggle           = nullptr;  // 奔跑/骑马加速
    UBPVE_JHConfigVolumeItem2_C* RunMountSpeedSlider           = nullptr;  // 加速倍率
    UBPVE_JHConfigVolumeItem2_C* WorldMoveSpeedSlider          = nullptr;  // 世界移动速度
    UBPVE_JHConfigVolumeItem2_C* SceneMoveSpeedSlider          = nullptr;  // 场景移动速度
    UBPVE_JHConfigVideoItem2_C*  MountReplaceToggle            = nullptr;  // 坐骑替换
    UBPVE_JHConfigVideoItem2_C*  MountSelectDD                 = nullptr;  // 指定坐骑
    UBPVE_JHConfigVideoItem2_C*  FirstPlayHardToggle           = nullptr;  // 一周目可选极难
    UBPVE_JHConfigVideoItem2_C*  FirstPlayInheritToggle        = nullptr;  // 一周目可选传承
    UBPVE_JHConfigVideoItem2_C*  AllInheritToggle              = nullptr;  // 承君传承包括所有
    UBPVE_JHConfigVideoItem2_C*  PostStationToggle             = nullptr;  // 未交互驿站可用
    UBPVE_JHConfigVideoItem2_C*  GmCommandToggle               = nullptr;  // 激活GM命令行
    UBPVE_JHConfigVideoItem2_C*  UnlockCodexToggle             = nullptr;  // 解锁全图鉴
    UBPVE_JHConfigVideoItem2_C*  UnlockAchievementToggle       = nullptr;  // 解锁全成就
    UBPVE_JHConfigVideoItem2_C*  ScreenModeDD                  = nullptr;  // 首选屏幕模式
    UBPVE_JHConfigVideoItem2_C*  VSyncDD                       = nullptr;  // 使用垂直同步
    UBPVE_JHConfigVideoItem2_C*  DynResDD                      = nullptr;  // 使用动态分辨率
    UBPVE_JHConfigVolumeItem2_C* ResolutionXEdit               = nullptr;  // 分辨率X
    UBPVE_JHConfigVolumeItem2_C* ResolutionYEdit               = nullptr;  // 分辨率Y
};

/// 动态扩展 Tab (6/7/8) 的按钮与内容容器
struct DynTabState
{
    UBP_JHConfigTabBtn_C* Btn6     = nullptr;   // 队友页按钮
    UBP_JHConfigTabBtn_C* Btn7     = nullptr;   // 任务页按钮
    UBP_JHConfigTabBtn_C* Btn8     = nullptr;   // 预留页按钮
    UVerticalBox*         Content6 = nullptr;   // 队友内容区
    UVerticalBox*         Content7 = nullptr;   // 任务内容区
    UVerticalBox*         Content8 = nullptr;   // 预留内容区
};

/// Tab6「队友」页控件
struct TeammateTabControls
{
    UBPVE_JHConfigVideoItem2_C*  FollowToggle  = nullptr;  // 强制跟随开关
    UBPVE_JHConfigVolumeItem2_C* FollowCount   = nullptr;  // 跟随人数滑块
    UBPVE_JHConfigVideoItem2_C*  AddDD         = nullptr;  // 添加队友下拉
    UBPVE_JHConfigVideoItem2_C*  ReplaceToggle = nullptr;  // 替换队友开关
    UBPVE_JHConfigVideoItem2_C*  ReplaceDD     = nullptr;  // 替换目标下拉
};

/// Tab7「任务」页控件
struct QuestTabControls
{
    UBPVE_JHConfigVideoItem2_C* Toggle = nullptr;  // 任务操作开关
    UBPVE_JHConfigVideoItem2_C* TypeDD = nullptr;  // 操作类型 (接取/完成)
};

// ┌─────────────────────────────────────────────────────────────┐
// │                   全局变量 extern 声明                      │
// └─────────────────────────────────────────────────────────────┘

// ── Hook 与核心运行状态 ──
extern GVCPostRenderFn          GOriginalPostRender;    // 原始 PostRender 函数指针
extern VTableHook               GVCPostRenderHook;      // VTable 虚表 Hook 实例
extern UUserWidget*             GInternalWidget;        // 作弊面板 Widget 实例
extern bool                     GInternalWidgetVisible; // 面板可见性标志
extern std::atomic<bool>        GIsUnloading;           // DLL 正在卸载
extern std::atomic<bool>        GUnloadCleanupDone;     // 游戏线程清理完毕
extern std::atomic<bool>        GHooksRemoved;          // 所有 Hook 已还原
extern std::atomic<bool>        GConsoleClosed;         // 调试控制台已关闭
extern std::atomic<int32>       GPostRenderInFlight;    // 当前渲染回调嵌套计数
extern UIRememberState          GUIRememberState;       // UI 状态记忆快照

// ── 劫持的原生面板引用 ──
extern UNeoUIButtonBase*        GCachedBtnExit;         // 面板"关闭"按钮
extern std::vector<UObject*>    GRootedObjects;         // GC Root 托管列表
extern UWidget*                 GOriginalLanPanel;      // 原始语言设置面板
extern UWidget*                 GOriginalInputMappingPanel; // 原始按键映射面板
extern UJHCommon_Btn_Free_C*    GOriginalResetButton;   // 原始"重置"按钮

// ── 物品浏览器 ──
extern ItemBrowserState         GItemBrowser;

// ── 滑块控件缓存 (跨 Tab 通用) ──
extern std::vector<UBPVE_JHConfigVolumeItem2_C*> GVolumeItems;
extern std::vector<float>       GVolumeLastValues;
extern std::vector<bool>        GVolumeMinusWasPressed;
extern std::vector<bool>        GVolumePlusWasPressed;

// ── Tab 页面控件实例 ──
extern Tab1Controls             GTab1;
extern Tab2Controls             GTab2;
extern Tab3Controls             GTab3;
extern Tab4Controls             GTab4;
extern Tab5Controls             GTab5;
extern std::vector<int32>       GTab5MountOptionIds;
extern std::vector<std::wstring> GTab5MountOptionLabels;
extern DynTabState              GDynTab;
extern TeammateTabControls      GTeammate;
extern QuestTabControls         GQuest;

// ── 物品不减 Inline Hook 底层变量 ──
extern std::atomic<bool>        GItemNoDecreaseEnabled; // 功能总开关
extern uintptr_t                GChangeItemNumAddr;     // 被 Hook 函数地址
extern unsigned char            GOriginalChangeItemNumBytes[5]; // 原始指令备份
extern void*                    GHookTrampoline;        // 跳板缓冲区
extern bool                     GInlineHookInstalled;   // Hook 安装状态

// ┌─────────────────────────────────────────────────────────────┐
// │              Inline Hook 功能接口 (Enable/Disable)          │
// └─────────────────────────────────────────────────────────────┘

// ── Tab1: 物品系统 ──
void EnableItemNoDecreaseHook();               // 物品数量不减
void DisableItemNoDecreaseHook();

void SetItemGainMultiplierHookValue(int32 Value); // 物品获取倍率
void EnableItemGainMultiplierHook();
void DisableItemGainMultiplierHook();

void SetCraftItemIncrementHookValue(float Value); // 制造产出提升
void SetCraftExtraEffectHookValue(float Value);
void EnableCraftEffectMultiplierHook();
void DisableCraftEffectMultiplierHook();

void EnableMaxExtraAffixesHooks();              // 最大额外词条
void DisableMaxExtraAffixesHooks();

void EnableAllItemsSellable();                  // 全物品可售
void DisableAllItemsSellable();

void EnableDropRate100Patch();                  // 掉率 100%
void DisableDropRate100Patch();

void EnableIgnoreItemUseCountFeature();         // 无视使用次数
void DisableIgnoreItemUseCountFeature();

void EnableIgnoreItemRequirementsPatch();       // 无视使用条件
void DisableIgnoreItemRequirementsPatch();

// ── Tab2: 战斗系统 ──
void EnableSkillNoCooldownHooks();              // 招式无 CD
void DisableSkillNoCooldownHooks();

void EnableNoEncounterPatch();                  // 不遇敌
void DisableNoEncounterPatch();

void EnableDefeatAsVictoryHook();               // 败犹为胜
void DisableDefeatAsVictoryHook();

void EnableAllTeammatesInFightHooks();          // 全员参战
void DisableAllTeammatesInFightHooks();

void EnableNeiGongFillLastSlotFeature();        // 心法破限
void DisableNeiGongFillLastSlotFeature();

void EnableBattleSpeedHooks();                  // 战斗加速
void DisableBattleSpeedHooks();
void SetBattleSpeedHookMultiplier(float Value);

void EnableAutoRecoverHpMpHook();               // 自动恢复气血/真气
void DisableAutoRecoverHpMpHook();

void EnableTotalMoveSpeedHook();                // 总移速倍率
void DisableTotalMoveSpeedHook();
void SetTotalMoveSpeedMultiplier(float Value);
void SetTotalMoveSpeedFriendlyOnly(bool Enabled);

// ── Tab3: 生活系统 (生活/Life) ──
void EnableCraftIgnoreRequirements();           // 无视锻造/制衣/炼丹/烹饪要求
void DisableCraftIgnoreRequirements();

void SetCraftOutputQuantity(int32 Value);       // 设置产出数量
void EnableCraftOutputQuantityHook();
void DisableCraftOutputQuantityHook();

void EnableGatherCooldownPatch();               // 采集一秒冷却
void DisableGatherCooldownPatch();

void EnableFishRareOnlyHook();                  // 钓鱼只钓稀有物
void DisableFishRareOnlyHook();

void EnableFishAlwaysCatchHook();               // 钓鱼收杆必有收获
void DisableFishAlwaysCatchHook();

void EnableHomelandHarvestPatch();              // 家园随时收获
void DisableHomelandHarvestPatch();

// ── Tab4: 社交系统 ──
void EnableGiftAlwaysLiked();                   // 送礼必定喜欢
void DisableGiftAlwaysLiked();

void EnableInviteIgnoreConditions();            // 邀请无视条件
void DisableInviteIgnoreConditions();

void EnableSparIgnoreFavor();                   // 切磋无视好感
void DisableSparIgnoreFavor();

void EnableConsultIgnoreRequirements();         // 请教无视要求
void DisableConsultIgnoreRequirements();

void EnableSparGetAllLoot();                    // 切磋获得对手背包
void DisableSparGetAllLoot();

void EnableNpcEquipRemovable();                 // NPC装备可脱
void DisableNpcEquipRemovable();

void EnableNpcIgnoreWeaponLimit();              // NPC无视武器功法限制
void DisableNpcIgnoreWeaponLimit();

void EnableForceNpcInteraction();               // 强制显示NPC互动
void DisableForceNpcInteraction();

// ── Tab5: 系统 ──
void EnableInfiniteJumpPatch();                 // 无限跳跃
void DisableInfiniteJumpPatch();

void EnableRunMountSpeedHook();                 // 奔跑/骑马加速
void DisableRunMountSpeedHook();
void SetRunMountSpeedMultiplier(float Value);

void EnableMountReplacePatch();                 // 坐骑替换
void DisableMountReplacePatch();
void SetMountReplaceId(int32 Value);

void EnableFirstPlayHardPatch();                // 一周目可选极难
void DisableFirstPlayHardPatch();

void EnableFirstPlayInheritPatch();             // 一周目可选传承
void DisableFirstPlayInheritPatch();

void EnablePostStationPatch();                  // 未交互驿站可用
void DisablePostStationPatch();

void EnableUnlockAllCodex();                     // 解锁全图鉴
void DisableUnlockAllCodex();

void EnableUnlockAllAchievements();              // 解锁全成就
void DisableUnlockAllAchievements();

void ApplyScreenSettings();                      // 屏幕设置

// ── Tab6: 队友系统 ──
void EnableFollowerCountHook();                 // 设置队友跟随数量
void DisableFollowerCountHook();
void SetFollowerCountValue(int32 Value);

void EnableReplaceTeammateHook();               // 替换指定队友
void DisableReplaceTeammateHook();
void SetReplaceTeammateId(int32 Value);