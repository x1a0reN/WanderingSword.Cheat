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

// Collapsible panel
UVE_JHVideoPanel2_C* CreateCollapsiblePanel(
	APlayerController* PC,
	const wchar_t* Title,
	bool bStartCollapsed = false);

// 每帧轮询折叠面板标题点击，手动切换展开/收起
void PollCollapsiblePanelsInput();
