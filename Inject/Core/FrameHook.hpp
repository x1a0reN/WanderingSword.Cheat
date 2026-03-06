#pragma once
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  FrameHook.hpp  —  游戏渲染帧回调 Hook 入口声明
//
//  本模块通过 VTable 劫持 UGameViewportClient::PostRender，
//  使作弊逻辑搭载在游戏每帧渲染流程末尾执行。
//  具体职责见 FrameHook.cpp 中的 HookedGVCPostRender 实现。
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

/// 被挂钩的 PostRender 回调 —— 作弊模块的心跳入口。
///
/// 每帧由引擎渲染线程调用，依次执行：
///   1. 快捷键检测 —— HOME 切换面板、DELETE 触发卸载
///   2. 面板生命周期 —— 首次创建 / 外部关闭后自动销毁
///   3. Tab1 物品系统轮询 —— 开关读取、DataTable 补丁、物品浏览器交互
///   4. Tab2 战斗系统轮询 —— 招式/遇敌/移速/伤害等 Hook 启停
///   5. 动态 Tab 切换 —— Tab6 队友 / Tab7 任务页面按钮点击侦测
///   6. 滑块控件通用轮询 —— 按钮点击增减 + 文本标签同步
///   7. FPS 叠加层绘制
void __fastcall HookedGVCPostRender(void* This, void* Canvas);
