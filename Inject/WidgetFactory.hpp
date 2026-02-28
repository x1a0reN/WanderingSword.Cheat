#pragma once

#include <initializer_list>

#include "CheatState.hpp"

/// 隐藏 Tab 按钮上的图标 (原生 Tab 有游戏设置图标，作弊菜单不需要)。
void HideTabIcon(UJHNeoUIConfigV2TabBtn* TabBtn);

/// 设置 Tab 按钮: 修改标题文本、设置 TabIndex、隐藏图标。
void SetupTab(UJHNeoUIConfigV2TabBtn* TabBtn, int32_t Index, const wchar_t* Title);

/// 修补动态创建的 Tab 按钮的运行时上下文:
/// 设置父级 ConfigView 引用 (偏移 0x02E0/0x03B0)，
/// 使 Tab 按钮的蓝图事件能正确找到所属面板。
void PatchTabBtnRuntimeContext(UJHNeoUIConfigV2TabBtn* TabBtn, UBPMV_ConfigView2_C* CV, const char* SourceTag);

/// 通过 UWidgetBlueprintLibrary::Create 创建 UBP_JHConfigTabBtn_C 实例，
/// 调用蓝图 Construct()，标记 GC Root 防回收。
UBP_JHConfigTabBtn_C* CreateTabButton(APlayerController* PC);

/// 创建游戏风格按钮 (UJHCommon_Btn_Free_C)，完成标准初始化流程:
/// 设置可见/可交互/渲染状态 → 设置标题文本 → 初始化 JHGPCBtn 子控件。
/// @param Width/Height  > 0 时创建 USizeBox 包裹按钮以控制尺寸
/// @param OutLayoutWidget  输出应添加到容器的控件 (有 SizeBox 时返回 SizeBox，否则返回按钮自身)
/// @return 按钮指针，可用于交互检测 (IsPressed/SetIsEnabled 等)
UJHCommon_Btn_Free_C* CreateGameStyleButton(
	APlayerController* PC,
	const wchar_t* LabelText,
	const char* LogTag,
	float Width = 0.0f,
	float Height = 0.0f,
	UWidget** OutLayoutWidget = nullptr);

/// 复用原生重置按钮 (Btn_Revert2) 创建展示用按钮。
UWidget* CreateShowcaseResetButton(UBPMV_ConfigView2_C* CV, UObject* Outer, APlayerController* PC);

/// 复用原生 Tab 按钮创建展示用 Tab 按钮。
UWidget* CreateShowcaseConfigTabBtn(UBPMV_ConfigView2_C* CV, UObject* Outer);

/// 按类名动态查找 UClass 并创建 Widget 实例，可选调用 Construct()。
/// 用于控件展示 Tab 的动态创建测试。
UWidget* CreateShowcaseWidgetByClassName(APlayerController* PC, const char* ClassName, bool bInvokeConstruct, UClass* FallbackClass = nullptr);

/// 创建下拉框控件 (UBPVE_JHConfigVideoItem2_C)，设置标题，清除蓝图绑定。
UBPVE_JHConfigVideoItem2_C* CreateVideoItem(APlayerController* PC, const wchar_t* Title);

/// 创建带预设选项的下拉框控件，设置标题后填入指定选项列表并默认选中第一项。
UBPVE_JHConfigVideoItem2_C* CreateVideoItemWithOptions(APlayerController* PC, const wchar_t* Title, std::initializer_list<const wchar_t*> Options);

/// 创建开关控件 (下拉框，选项为"关/开")，默认选中"关"。
UBPVE_JHConfigVideoItem2_C* CreateToggleItem(APlayerController* PC, const wchar_t* Title);

/// 创建滑块控件 (UBPVE_JHConfigVolumeItem2_C)，设置标题，清除蓝图绑定。
UBPVE_JHConfigVolumeItem2_C* CreateVolumeItem(APlayerController* PC, const wchar_t* Title);
