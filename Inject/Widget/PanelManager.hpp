#pragma once

#include "CheatState.hpp"

/// 按蓝图正确顺序调用 ConfigView2 的初始化事件链:
/// EVT_VisualConstructOnce → EVT_SetupSubModuleSlots → EVT_SyncTabIndex(0)
/// → EVT_SyncWithGlobalInputMode → EVT_VisualShow
/// 调用顺序严格，否则 Tab 切换/NamedSlot 绑定会失效。
void InitializeConfigView2BySDK(UBPMV_ConfigView2_C* ConfigView);

/// 面板初始化完成后的 UI 定制入口:
/// 重命名 6 个原生 Tab 标签 → 隐藏标题图片 → 移除重置按钮/提示文字
/// → 填充所有 Tab 内容 (PopulateTab_*) → 创建动态 Tab 6/7/8
/// → 设置默认 Tab 激活状态。
void ApplyConfigView2TextPatch(UUserWidget* Widget, APlayerController* PC);

/// 动态新增 Tab 6(队友)、7(任务)、8(控件展示):
/// 创建 Tab 按钮并添加到 CT_TabBtns，创建 UVerticalBox 内容容器
/// 挂载到 Switcher 父容器 (避免 SetActiveWidgetIndex 蓝图崩溃)，
/// 然后调用 PopulateTab_Teammates/Quests/Controls 填充内容。
void CreateDynamicTabs(UBPMV_ConfigView2_C* CV, APlayerController* PC);

/// 将所有运行时 Widget 全局指针清零 (动态 Tab、队友/任务控件、物品浏览器等)。
/// 在面板销毁或 DLL 卸载时调用。
void ClearRuntimeWidgetState();

/// 获取第一个本地玩家的 PlayerController，有三层 fallback:
/// GetPlayerController(0) → GameInstance.LocalPlayers[0] → PersistentLevel.OwningWorld
APlayerController* GetFirstLocalPlayerController();

/// 创建作弊面板的 Widget 实例。优先使用缓存的成功类，
/// 否则依次尝试 BPMV_ConfigView2_C 等 8 个候选类。
/// 仅创建实例，不做初始化。
UUserWidget* CreateInternalWidgetInstance(APlayerController* PlayerController);

/// 确保鼠标光标可见 (bShowMouseCursor = true)。
void EnsureMouseCursorVisible();

/// 隐藏作弊面板 (RemoveFromParent，不销毁，可复用)，恢复游戏。
void HideInternalWidget(APlayerController* PlayerController);

/// 销毁作弊面板: RemoveFromParent + 清除所有 GC Root + 清零全部状态，恢复游戏。
void DestroyInternalWidget(APlayerController* PlayerController);

/// 显示作弊面板: 不存在则创建 → AddToViewport(ZOrder=10000)
/// → 首次创建时执行初始化+文本修补+1.1x缩放 → 暂停游戏。
void ShowInternalWidget(APlayerController* PlayerController);

/// HOME 键触发，根据 IsInViewport() 判断当前状态，
/// 调用 ShowInternalWidget 或 HideInternalWidget。
void ToggleInternalWidget();

/// 切换到动态 Tab: 隐藏原生 CT_Contents，显示 DynIdx 对应的动态内容容器。
/// DynIdx = 6(队友) / 7(任务) / 8(控件)
void ShowDynamicTab(UBPMV_ConfigView2_C* CV, int32 DynIdx);

/// 切换回原生 Tab: 显示 CT_Contents，隐藏所有动态内容容器。
void ShowOriginalTab(UBPMV_ConfigView2_C* CV);
