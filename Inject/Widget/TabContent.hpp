#pragma once

#include "CheatState.hpp"

/// 获取或创建 NamedSlot 的内容容器:
/// 移除游戏原生子模块面板 (其蓝图 Tick/动画会导致崩溃)，
/// 创建新的 UVerticalBox 替代，标记 GC Root。
/// 若 Slot 为空则返回 nullptr。
UPanelWidget* GetOrCreateSlotContainer(UBPMV_ConfigView2_C* CV, UNeoUINamedSlot* Slot, const char* SlotName);

/// Tab 0 - 角色: 金钱/武学点/经脉点/门派贡献/继承点/等级/额外心法栏/
/// 钓鱼等级/门派 + 属性 (气血/真气/精力/力道/根骨/身法/内功/攻防/暴击/闪避/命中)
/// + 武器精通 (拳掌/剑法/刀法/枪棍/暗器)
void PopulateTab_Character(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 1 - 物品: 物品不减/获得加倍/全可售/掉落率100%/锻造制衣加倍/
/// 最大额外词条/无视使用次数和要求 + 物品浏览器 (分类/翻页/图标网格/数量输入)
void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 2 - 战斗: 伤害加倍/招式无视冷却/战斗加速/不遇敌/全队友参战/
/// 战败视为胜利/心法填装最后一格/战斗前自动恢复/移动速度加倍
void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 3 - 生活: 锻造制衣炼丹烹饪无视要求/设置产出数量/
/// 采集一秒冷却/钓鱼只钓稀有物/钓鱼收杆必有收获/家园随时收获
void PopulateTab_Life(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 4 - 社交: 送礼必定喜欢/邀请无视条件/切磋无视好感/
/// 请教无视要求/切磋获得对手背包/NPC装备可脱/NPC无视武器功法限制/强制显示NPC互动
void PopulateTab_Social(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 5 - 系统: 空格跳跃+速度+无限/奔跑骑马加速/坐骑替换/
/// 一周目可选极难和传承/承君传承包括所有/未交互驿站可用/激活GM命令行/
/// 解锁全图鉴/解锁全成就
void PopulateTab_System(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 6 (动态) - 队友: 设置跟随数量/添加队友下拉框/替换指定队友
void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 7 (动态) - 任务: 接到或完成任务开关 + 执行类型下拉框
void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab 8 (动态) - 控件展示: 用于测试各种游戏原生控件的创建和显示
void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// Tab0 数值输入轮询（仅在 Tab0 激活时调用）：
/// - 回车提交当前获得焦点的编辑框
/// - 提交后写入游戏数据（SDK 调用），并回填最新值
void PollTab0CharacterInput(bool bTab0Active);
