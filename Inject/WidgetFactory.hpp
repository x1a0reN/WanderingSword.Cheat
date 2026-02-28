#pragma once

#include <initializer_list>

#include "CheatState.hpp"

void HideTabIcon(UJHNeoUIConfigV2TabBtn* TabBtn);
void SetupTab(UJHNeoUIConfigV2TabBtn* TabBtn, int32_t Index, const wchar_t* Title);
void PatchTabBtnRuntimeContext(UJHNeoUIConfigV2TabBtn* TabBtn, UBPMV_ConfigView2_C* CV, const char* SourceTag);
UBP_JHConfigTabBtn_C* CreateTabButton(APlayerController* PC);

UJHCommon_Btn_Free_C* CreateGameStyleButton(
	APlayerController* PC,
	UJHCommon_Btn_Free_C* ReuseSource,
	const wchar_t* LabelText,
	const char* LogTag);
UWidget* CreateShowcaseResetButton(UBPMV_ConfigView2_C* CV, UObject* Outer, APlayerController* PC);
UWidget* CreateShowcaseConfigTabBtn(UBPMV_ConfigView2_C* CV, UObject* Outer);
UWidget* CreateShowcaseWidgetByClassName(APlayerController* PC, const char* ClassName, bool bInvokeConstruct, UClass* FallbackClass = nullptr);

UBPVE_JHConfigVideoItem2_C* CreateVideoItem(APlayerController* PC, const wchar_t* Title);
UBPVE_JHConfigVideoItem2_C* CreateVideoItemWithOptions(APlayerController* PC, const wchar_t* Title, std::initializer_list<const wchar_t*> Options);
UBPVE_JHConfigVideoItem2_C* CreateToggleItem(APlayerController* PC, const wchar_t* Title);
UBPVE_JHConfigVolumeItem2_C* CreateVolumeItem(APlayerController* PC, const wchar_t* Title);
