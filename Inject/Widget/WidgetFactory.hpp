#pragma once

#include <initializer_list>

#include "CheatState.hpp"
#include "SDK/VE_JHVideoPanel2_classes.hpp"

// Tab button helpers
void HideTabIcon(UJHNeoUIConfigV2TabBtn* TabBtn);
void SetupTab(UJHNeoUIConfigV2TabBtn* TabBtn, int32_t Index, const wchar_t* Title);
void PatchTabBtnRuntimeContext(UJHNeoUIConfigV2TabBtn* TabBtn, UBPMV_ConfigView2_C* CV, const char* SourceTag);
UBP_JHConfigTabBtn_C* CreateTabButton(APlayerController* PC);

// Game style button helpers
UJHCommon_Btn_Free_C* CreateGameStyleButton(
	APlayerController* PC,
	const wchar_t* LabelText,
	const char* LogTag,
	float Width = 0.0f,
	float Height = 0.0f,
	UWidget** OutLayoutWidget = nullptr);

UWidget* CreateShowcaseResetButton(UBPMV_ConfigView2_C* CV, UObject* Outer, APlayerController* PC);
UWidget* CreateShowcaseConfigTabBtn(UBPMV_ConfigView2_C* CV, UObject* Outer);
UWidget* CreateShowcaseWidgetByClassName(
	APlayerController* PC,
	const char* ClassName,
	bool bInvokeConstruct,
	UClass* FallbackClass = nullptr);

// Config rows
UBPVE_JHConfigVideoItem2_C* CreateVideoItem(APlayerController* PC, const wchar_t* Title);
UBPVE_JHConfigVideoItem2_C* CreateVideoItemWithOptions(
	APlayerController* PC,
	const wchar_t* Title,
	std::initializer_list<const wchar_t*> Options);
UBPVE_JHConfigVideoItem2_C* CreateToggleItem(APlayerController* PC, const wchar_t* Title);
UBPVE_JHConfigVolumeItem2_C* CreateVolumeItem(APlayerController* PC, const wchar_t* Title);

// EditBox row based on volume item style
UBPVE_JHConfigVolumeItem2_C* CreateVolumeEditBoxItem(
	APlayerController* PC,
	UObject* Outer,
	UPanelWidget* FallbackContainer,
	const wchar_t* Title,
	const wchar_t* Hint,
	const wchar_t* DefaultValue);

// Numeric-only editbox row based on volume item style
UBPVE_JHConfigVolumeItem2_C* CreateVolumeNumericEditBoxItem(
	APlayerController* PC,
	UObject* Outer,
	UPanelWidget* FallbackContainer,
	const wchar_t* Title,
	const wchar_t* Hint,
	const wchar_t* DefaultValue);

// Collapsible panel
UVE_JHVideoPanel2_C* CreateCollapsiblePanel(
	APlayerController* PC,
	const wchar_t* Title,
	bool bStartCollapsed = false);

// 每帧轮询折叠面板标题点击，手动切换展开/收起
void PollCollapsiblePanelsInput();

/// 读取当前已创建控件的实时状态，写入 GUIRememberState。
void RememberUIControlStatesFromLiveWidgets();

/// 针对单个滑块控件即时记忆（用于拖拽/滚轮/按钮等实时变化路径）。
void RememberSingleSliderState(UBPVE_JHConfigVolumeItem2_C* Item);

/// 在当前已创建的 live 控件上，按 GUIRememberState 回灌滑块值。
void RestoreRememberedSliderStatesToLiveWidgets();

/// 在指定毫秒内禁止滑块实时记忆覆写（用于面板刚显示时防止默认值回写）。
void SuppressSliderRealtimeRememberForMs(DWORD DurationMs);

/// 重建/销毁时清空“当前运行时控件绑定”，保留 GUIRememberState 中的记忆值。
void ResetRuntimeControlStateBindings();
