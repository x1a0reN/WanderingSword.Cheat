#pragma once

#include "CheatState.hpp"

FText MakeText(const wchar_t* W);
const char* ToVisName(ESlateVisibility V);
int32 GetItemAddQuantityFromEdit();

UObject* FindFirstObjectOfClass(UClass* TargetClass);
UDataTable* GetItemDataTableFromManager(UItemResManager* ResMgr);

void ClearDelegate(void* DelegatePtr);
void ClearSliderBindings(USlider* Slider);
void ClearButtonBindings(UWidget* Btn);
void ClearComboBoxBindings(UComboBoxString* CB);
void ClearEditableTextBindings(UEditableTextBox* Edit);
void ClearUserWidgetBindings(UUserWidget* UserWidget);
void ClearNeoUICommonButtonBindings(UWidget* Btn);
void ClearJHGamepadConfirmButtonBindings(UWidget* Btn);
void ClearJHCommonBtnFreeBindings(UWidget* Widget);
void ClearRegionTitleBindings(UWidget* Widget);
void SanitizeSingleWidget(UWidget* Widget, bool bDisableInteraction);
void SanitizeWidgetTree(UWidget* Root, bool bDisableInteraction);
UWidget* ReuseDetachedWidget(UWidget* Widget);

UWidget* CreateRawWidget(UClass* WidgetClass, UObject* Outer);
UWidget* CreateRawWidgetFromTemplate(UClass* WidgetClass, UObject* Outer, UObject* TemplateObj, const char* Tag);
UTextBlock* CreateRawTextLabel(UObject* Outer, const wchar_t* Text);
