#pragma once

/// UGameViewportClient::PostRender 的 VTable Hook 实现。
/// 每帧执行，处理以下逻辑:
/// - HOME 键边沿触发: 切换作弊面板显示/隐藏
/// - BTN_Exit 点击检测: 边沿触发关闭面板
/// - 物品浏览器轮询: 分类切换/翻页按钮/物品槽点击添加
/// - 动态 Tab 6/7/8 点击切换: 检测按钮 IsPressed 并切换内容可见性
/// - 外部关闭检测: 蓝图逻辑关闭面板时自动恢复游戏状态
void __fastcall HookedGVCPostRender(void* This, void* Canvas);

/// UItemManager::ProcessEvent 的 VTable Hook 实现。
/// 用于拦截 ChangeItemNum 等物品操作函数，实现物品不减功能
void __stdcall HookedProcessEvent(void* This, void* Function, void* Parms);
