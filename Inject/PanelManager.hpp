#pragma once

#include "CheatState.hpp"

void InitializeConfigView2BySDK(UBPMV_ConfigView2_C* ConfigView);
void ApplyConfigView2TextPatch(UUserWidget* Widget, APlayerController* PC);
void CreateDynamicTabs(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void ClearRuntimeWidgetState();
APlayerController* GetFirstLocalPlayerController();
UUserWidget* CreateInternalWidgetInstance(APlayerController* PlayerController);
void EnsureMouseCursorVisible();
void HideInternalWidget(APlayerController* PlayerController);
void DestroyInternalWidget(APlayerController* PlayerController);
void ShowInternalWidget(APlayerController* PlayerController);
void ToggleInternalWidget();

void ShowDynamicTab(UBPMV_ConfigView2_C* CV, int32 DynIdx);
void ShowOriginalTab(UBPMV_ConfigView2_C* CV);
