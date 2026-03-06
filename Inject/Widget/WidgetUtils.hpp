#pragma once

#include "CheatState.hpp"

bool IsPointerInLiveObjectArray(UObject* Obj);
bool IsSafeLiveObject(UObject* Obj);
bool IsSafeLiveObjectOfClass(UObject* Obj, UClass* ExpectedClass);


FWeakObjectPtr ReadWeakPtrAt(UObject* Obj, uintptr_t Offset);
void WriteWeakPtrAt(UObject* Obj, uintptr_t Offset, const FWeakObjectPtr& Value);
bool IsWeakPtrFilled(const FWeakObjectPtr& Weak);
bool IsSameWeak(const FWeakObjectPtr& A, const FWeakObjectPtr& B);
UObject* ResolveWeakPtrLoose(const FWeakObjectPtr& Weak);


FText MakeText(const wchar_t* W);

const char* ToVisName(ESlateVisibility V);

int32 GetItemAddQuantityFromEdit();


UObject* FindFirstObjectOfClass(UClass* TargetClass);

UDataTable* GetItemDataTableFromManager(UItemResManager* ResMgr);


void ClearDelegate(void* DelegatePtr);

void ClearSliderBindings(USlider* Slider);

void ClearSliderGameBinding(USlider* Slider);

void ClearButtonBindings(UWidget* Btn);

void ClearComboBoxBindings(UComboBoxString* CB);

void ClearComboBoxGameBinding(UComboBoxString* CB);

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
