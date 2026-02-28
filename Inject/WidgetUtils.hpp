#pragma once

#include "CheatState.hpp"

// ── 文本与诊断工具 ──

/// 将 wchar_t* 字符串转换为 UE4 FText (通过 FTextStringHelper::CreateFromBuffer)。
FText MakeText(const wchar_t* W);

/// 将 ESlateVisibility 枚举值转为可读字符串 (用于日志输出)。
const char* ToVisName(ESlateVisibility V);

/// 从全局物品数量输入框 (GItemQuantityEdit) 读取用户输入的整数值，
/// 转换失败或为空时返回默认值 1。
int32 GetItemAddQuantityFromEdit();

// ── 对象查找 ──

/// 遍历 GObjects 查找指定 UClass 的第一个有效实例。
UObject* FindFirstObjectOfClass(UClass* TargetClass);

/// 从 UItemResManager 获取 ItemDataTable 指针 (偏移 +0x30)。
UDataTable* GetItemDataTableFromManager(UItemResManager* ResMgr);

// ── 委托/绑定清除 ──
// 防止 Widget 触发游戏蓝图逻辑 (导致空指针崩溃)，
// 通过直接将 TMulticastInlineDelegate (0x10字节) 清零来断开绑定。

/// 清零一个 TMulticastInlineDelegate (16字节)。
void ClearDelegate(void* DelegatePtr);

/// 清除 USlider 的 OnValueChanged/OnMouseCaptureBegin/End 委托。
void ClearSliderBindings(USlider* Slider);

/// 清除 USlider 游戏特有的绑定 (OnValueChanged + OnMouseCaptureEnd)。
void ClearSliderGameBinding(USlider* Slider);

/// 清除 UButton 的 OnClicked/OnPressed/OnReleased/OnHovered/OnUnhovered 委托。
void ClearButtonBindings(UWidget* Btn);

/// 清除 UComboBoxString 的 OnSelectionChanged/OnOpening 委托。
void ClearComboBoxBindings(UComboBoxString* CB);

/// 清除 UComboBoxString 游戏特有的 OnSelectionChanged 绑定。
void ClearComboBoxGameBinding(UComboBoxString* CB);

/// 清除 UEditableTextBox 的 OnTextChanged/OnTextCommitted 委托。
void ClearEditableTextBindings(UEditableTextBox* Edit);

/// 清除 UUserWidget 的 Construct 和 Destruct 蓝图绑定。
void ClearUserWidgetBindings(UUserWidget* UserWidget);

/// 清除 NeoUI 通用按钮 (UNeoUIButtonBase 系) 的 OnClicked/OnHovered/OnUnhovered 绑定。
void ClearNeoUICommonButtonBindings(UWidget* Btn);

/// 清除 JH 手柄确认按钮 (UJHNeoUIGamepadConfirmButton) 的特有绑定。
void ClearJHGamepadConfirmButtonBindings(UWidget* Btn);

/// 清除 UJHCommon_Btn_Free_C 内部的 JHGPCBtn 和 JHGPCBtn_ActiveBG 绑定。
void ClearJHCommonBtnFreeBindings(UWidget* Widget);

/// 清除 UBPVE_RegionTitle_C 按钮的区域展开/收起绑定。
void ClearRegionTitleBindings(UWidget* Widget);

/// 清理单个 Widget: 清除其所有绑定，可选禁用交互。
void SanitizeSingleWidget(UWidget* Widget, bool bDisableInteraction);

/// 递归清理 Widget 树: 对根控件及其所有子控件执行 SanitizeSingleWidget。
void SanitizeWidgetTree(UWidget* Root, bool bDisableInteraction);

/// 复用已从父容器移除的 Widget: 重新设为可见并标记 GC Root。
UWidget* ReuseDetachedWidget(UWidget* Widget);

// ── 底层 Widget 创建 ──

/// 通过 StaticConstructObject_Internal (RVA 0x17C6140) 直接创建非 UUserWidget 控件
/// (如 UVerticalBox / USizeBox / UButton 等)，自动标记 GC Root。
/// UUserWidget 子类应使用 UWidgetBlueprintLibrary::Create 而非此函数。
UWidget* CreateRawWidget(UClass* WidgetClass, UObject* Outer);

/// 基于模板对象克隆创建 Widget (bCopyTransients=1, bAssumeTemplate=1)，
/// 自动标记 GC Root。
UWidget* CreateRawWidgetFromTemplate(UClass* WidgetClass, UObject* Outer, UObject* TemplateObj, const char* Tag);

/// 创建 UTextBlock 文本标签并设置内容和自动换行。
UTextBlock* CreateRawTextLabel(UObject* Outer, const wchar_t* Text);
