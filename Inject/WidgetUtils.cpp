#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <cstring>

#include "WidgetUtils.hpp"
#include "GCManager.hpp"

bool IsPointerInLiveObjectArray(UObject* Obj)
{
	if (!Obj)
		return false;

	const uintptr_t Ptr = reinterpret_cast<uintptr_t>(Obj);
	if (Ptr < 0x10000 || (Ptr & (sizeof(void*) - 1)) != 0)
		return false;

	auto* ObjArray = UObject::GObjects.GetTypedPtr();
	if (!ObjArray)
		return false;

	const int32 Num = ObjArray->Num();
	for (int32 i = 0; i < Num; ++i)
	{
		if (ObjArray->GetByIndex(i) == Obj)
			return true;
	}
	return false;
}

bool IsSafeLiveObject(UObject* Obj)
{
	if (!IsPointerInLiveObjectArray(Obj))
		return false;
	return UKismetSystemLibrary::IsValid(Obj);
}

bool IsSafeLiveObjectOfClass(UObject* Obj, UClass* ExpectedClass)
{
	if (!ExpectedClass || !IsSafeLiveObject(Obj))
		return false;
	return Obj->IsA(ExpectedClass);
}

FText MakeText(const wchar_t* W)
{
	return UKismetTextLibrary::Conv_StringToText(FString(W));
}
const char* ToVisName(ESlateVisibility V)
{
	switch (V)
	{
	case ESlateVisibility::Visible: return "Visible";
	case ESlateVisibility::Collapsed: return "Collapsed";
	case ESlateVisibility::Hidden: return "Hidden";
	case ESlateVisibility::HitTestInvisible: return "HitTestInvisible";
	case ESlateVisibility::SelfHitTestInvisible: return "SelfHitTestInvisible";
	default: return "Unknown";
	}
}
int32 GetItemAddQuantityFromEdit()
{
	if (!GItemQuantityEdit)
		return GItemAddQuantity;

	FText T = GItemQuantityEdit->GetText();
	FString S = UKismetTextLibrary::Conv_TextToString(T);
	const wchar_t* WS = S.CStr();
	if (!WS || !WS[0])
		return 1;

	int32 V = _wtoi(WS);
	if (V < 1) V = 1;
	if (V > 9999) V = 9999;
	return V;
}

// ── Tab helpers ──
void ClearDelegate(void* DelegatePtr)
{
	if (DelegatePtr)
		memset(DelegatePtr, 0, 0x10);
}

// Clear all blueprint event bindings from a USlider
// USlider::OnValueChanged at 0x04D8
void ClearSliderBindings(USlider* Slider)
{
	if (!Slider) return;
	auto base = reinterpret_cast<uintptr_t>(Slider);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0498)); // OnMouseCaptureBegin
	ClearDelegate(reinterpret_cast<void*>(base + 0x04A8)); // OnMouseCaptureEnd
	ClearDelegate(reinterpret_cast<void*>(base + 0x04B8)); // OnControllerCaptureBegin
	ClearDelegate(reinterpret_cast<void*>(base + 0x04C8)); // OnControllerCaptureEnd
	ClearDelegate(reinterpret_cast<void*>(base + 0x04D8)); // OnValueChanged
}

// Only clear the game-effect delegate, keep UI interaction delegates intact.
void ClearSliderGameBinding(USlider* Slider)
{
	if (!Slider) return;
	auto base = reinterpret_cast<uintptr_t>(Slider);
	ClearDelegate(reinterpret_cast<void*>(base + 0x04A8)); // OnMouseCaptureEnd
	ClearDelegate(reinterpret_cast<void*>(base + 0x04C8)); // OnControllerCaptureEnd
	ClearDelegate(reinterpret_cast<void*>(base + 0x04D8)); // OnValueChanged
}

// Clear all blueprint event bindings from a UButton (or UNeoUIButtonBase)
// UButton::OnClicked at 0x03C8, OnPressed at 0x03D8, OnReleased at 0x03E8
void ClearButtonBindings(UWidget* Btn)
{
	if (!Btn) return;
	auto base = reinterpret_cast<uintptr_t>(Btn);
	ClearDelegate(reinterpret_cast<void*>(base + 0x03C8)); // OnClicked
	ClearDelegate(reinterpret_cast<void*>(base + 0x03D8)); // OnPressed
	ClearDelegate(reinterpret_cast<void*>(base + 0x03E8)); // OnReleased
	ClearDelegate(reinterpret_cast<void*>(base + 0x03F8)); // OnHovered
	ClearDelegate(reinterpret_cast<void*>(base + 0x0408)); // OnUnhovered
}

// Clear all blueprint event bindings from a UComboBoxString
// UComboBoxString::OnSelectionChanged at 0x0D90
void ClearComboBoxBindings(UComboBoxString* CB)
{
	if (!CB) return;
	auto base = reinterpret_cast<uintptr_t>(CB);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0D90)); // OnSelectionChanged
	ClearDelegate(reinterpret_cast<void*>(base + 0x0DA0)); // OnOpening
}

// Only clear the game-effect delegate, keep dropdown UI working.
void ClearComboBoxGameBinding(UComboBoxString* CB)
{
	if (!CB) return;
	auto base = reinterpret_cast<uintptr_t>(CB);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0D90)); // OnSelectionChanged only
}

// Clear all blueprint event bindings from a UEditableTextBox
// UEditableTextBox::OnTextChanged at 0x0A08, OnTextCommitted at 0x0A18
void ClearEditableTextBindings(UEditableTextBox* Edit)
{
	if (!Edit) return;
	auto base = reinterpret_cast<uintptr_t>(Edit);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0A08)); // OnTextChanged
	ClearDelegate(reinterpret_cast<void*>(base + 0x0A18)); // OnTextCommitted
}

// Clear all blueprint event bindings from a UUserWidget
// UUserWidget::OnVisibilityChanged at 0x0168
void ClearUserWidgetBindings(UUserWidget* UserWidget)
{
	if (!UserWidget) return;
	auto base = reinterpret_cast<uintptr_t>(UserWidget);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0168)); // OnVisibilityChanged
}

// Clear delegates declared on NeoUICommonButton (not covered by UButton offsets).
void ClearNeoUICommonButtonBindings(UWidget* Btn)
{
	if (!Btn || !Btn->IsA(UNeoUICommonButton::StaticClass()))
		return;
	auto base = reinterpret_cast<uintptr_t>(Btn);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0270)); // SingleClick
	ClearDelegate(reinterpret_cast<void*>(base + 0x0280)); // DoubleClick
	ClearDelegate(reinterpret_cast<void*>(base + 0x0290)); // MouseEneter
	ClearDelegate(reinterpret_cast<void*>(base + 0x02A0)); // MouseLeave
}

// Clear delegates declared on JH's wrapper button class.
void ClearJHGamepadConfirmButtonBindings(UWidget* Btn)
{
	if (!Btn || !Btn->IsA(UJHNeoUIGamepadConfirmButton::StaticClass()))
		return;

	auto* JHBtn = static_cast<UJHNeoUIGamepadConfirmButton*>(Btn);
	auto base = reinterpret_cast<uintptr_t>(JHBtn);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0540)); // OnBtnMouseEnter
	ClearDelegate(reinterpret_cast<void*>(base + 0x0550)); // OnBtnMouseLeave
	ClearDelegate(reinterpret_cast<void*>(base + 0x0560)); // OnBtnClicked
	ClearDelegate(reinterpret_cast<void*>(base + 0x0570)); // OnBtnDoubleClicked

	// The wrapped native button can also have base UButton delegates bound.
	if (JHBtn->BtnMain)
		ClearButtonBindings(static_cast<UWidget*>(JHBtn->BtnMain));
}

// Clear extra blueprint delegates on JHCommon_Btn_Free_C.
void ClearJHCommonBtnFreeBindings(UWidget* Widget)
{
	if (!Widget || !Widget->IsA(UJHCommon_Btn_Free_C::StaticClass()))
		return;
	auto base = reinterpret_cast<uintptr_t>(Widget);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0590)); // BtnClick
}

static UClass* GetRegionTitleClass()
{
	static UClass* Cached = nullptr;
	static bool Tried = false;
	if (!Tried)
	{
		Tried = true;
		Cached = UObject::FindClassFast("BPVE_RegionTitle_C");
		if (!Cached)
			Cached = UObject::FindClass("BPVE_RegionTitle_C");
	}
	return Cached;
}

// Clear extra blueprint delegates on BPVE_RegionTitle_C.
void ClearRegionTitleBindings(UWidget* Widget)
{
	UClass* RegionTitleClass = GetRegionTitleClass();
	if (!Widget || !RegionTitleClass || !Widget->IsA(RegionTitleClass))
		return;
	auto base = reinterpret_cast<uintptr_t>(Widget);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0318)); // BtnClick
}
void SanitizeSingleWidget(UWidget* Widget, bool bDisableInteraction)
{
	if (!Widget)
		return;

	if (Widget->IsA(UButton::StaticClass()))
	{
		ClearButtonBindings(Widget);
		static_cast<UButton*>(Widget)->IsFocusable = false;
	}
	ClearNeoUICommonButtonBindings(Widget);
	ClearJHGamepadConfirmButtonBindings(Widget);
	ClearJHCommonBtnFreeBindings(Widget);
	ClearRegionTitleBindings(Widget);
	if (Widget->IsA(USlider::StaticClass()))
		ClearSliderBindings(static_cast<USlider*>(Widget));
	if (Widget->IsA(UComboBoxString::StaticClass()))
		ClearComboBoxBindings(static_cast<UComboBoxString*>(Widget));
	if (Widget->IsA(UEditableTextBox::StaticClass()))
		ClearEditableTextBindings(static_cast<UEditableTextBox*>(Widget));

	if (Widget->IsA(UUserWidget::StaticClass()))
	{
		auto* UserWidget = static_cast<UUserWidget*>(Widget);
		ClearUserWidgetBindings(UserWidget);
		UserWidget->bIsFocusable = false;
		UserWidget->SetInputActionBlocking(true);
		UserWidget->StopListeningForAllInputActions();
		UserWidget->StopAllAnimations();
		UserWidget->StopAnimationsAndLatentActions();
		UserWidget->CancelLatentActions();
		UserWidget->TickFrequency = EWidgetTickFrequency::Never;
	}

	if (bDisableInteraction)
	{
		// NeoUI controls may still query VM/focus paths when entering a tab.
		// Force display-only visual mode and disable interaction path.
		if (Widget->IsA(UNeoUIVisualBase::StaticClass()) || Widget->IsA(UNeoUIButtonBase::StaticClass()))
		{
			UJHNeoUIUtilLib::SetVisual_PureDisplay(Widget, true);
			UJHNeoUIUtilLib::SetVisual_UserInteraction(Widget, false, false);
		}
		Widget->SetIsEnabled(false);
	}
}
void SanitizeWidgetTree(UWidget* Root, bool bDisableInteraction)
{
	if (!Root)
		return;

	SanitizeSingleWidget(Root, bDisableInteraction);

	if (Root->IsA(UUserWidget::StaticClass()))
	{
		auto* UserWidget = static_cast<UUserWidget*>(Root);
		if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget
		    && UserWidget->WidgetTree->RootWidget != Root)
		{
			SanitizeWidgetTree(UserWidget->WidgetTree->RootWidget, bDisableInteraction);
		}
	}

	if (Root->IsA(UPanelWidget::StaticClass()))
	{
		auto* Panel = static_cast<UPanelWidget*>(Root);
		int32 ChildCount = Panel->GetChildrenCount();
		for (int32 i = 0; i < ChildCount; i++)
		{
			if (UWidget* Child = Panel->GetChildAt(i))
				SanitizeWidgetTree(Child, bDisableInteraction);
		}
	}
}
UWidget* ReuseDetachedWidget(UWidget* Widget)
{
	if (!Widget)
		return nullptr;

	if (Widget->GetParent())
		Widget->RemoveFromParent();

	Widget->SetVisibility(ESlateVisibility::Visible);
	Widget->SetIsEnabled(true);
	return Widget;
}
UObject* FindFirstObjectOfClass(UClass* TargetClass)
{
	if (!TargetClass) return nullptr;
	auto* ObjArray = UObject::GObjects.GetTypedPtr();
	if (!ObjArray) return nullptr;
	UObject* CDO = TargetClass->ClassDefaultObject;
	int32 Num = ObjArray->Num();
	for (int32 i = 0; i < Num; i++)
	{
		UObject* Obj = ObjArray->GetByIndex(i);
		if (Obj && Obj != CDO && Obj->IsA(TargetClass))
			return Obj;
	}
	return nullptr;
}
UDataTable* GetItemDataTableFromManager(UItemResManager* ResMgr)
{
	if (!ResMgr)
		return nullptr;
	return *reinterpret_cast<UDataTable**>(reinterpret_cast<uintptr_t>(ResMgr) + 0x0030);
}
UWidget* CreateRawWidget(UClass* WidgetClass, UObject* Outer)
{
	if (!WidgetClass)
		return nullptr;

	static uintptr_t Base = (uintptr_t)GetModuleHandle(nullptr);

	// StaticConstructObject_Internal takes a const FStaticConstructObjectParameters*
	using StaticConstructObjectFn = UObject* (__fastcall*)(const void*);
	static auto StaticConstructObject = reinterpret_cast<StaticConstructObjectFn>(Base + 0x17C6140);

	// FStaticConstructObjectParameters (0x40 bytes, see sub_1417AA210 for layout)
	struct alignas(8) FParams {
		UClass*  Class;           // +0x00
		UObject* Outer;           // +0x08
		uint64_t Name;            // +0x10  FName (0 = NAME_None)
		uint32_t SetFlags;        // +0x18  EObjectFlags
		uint32_t InternalFlags;   // +0x1C  EInternalObjectFlags
		uint8_t  bCopyTransients; // +0x20
		uint8_t  bAssumeTemplate; // +0x21
		uint8_t  Pad[6];         // +0x22
		UObject* Template;        // +0x28
		void*    InstanceGraph;   // +0x30
		void*    ExternalPkg;     // +0x38
	};
	static_assert(sizeof(FParams) == 0x40, "FStaticConstructObjectParameters size mismatch");

	FParams Params = {};
	Params.Class = WidgetClass;
	Params.Outer = Outer;
	Params.SetFlags = 8; // RF_Transactional (matches UWidgetTree::ConstructWidget)

	UObject* Obj = StaticConstructObject(&Params);
	if (Obj)
	{
		MarkAsGCRoot(Obj); // Prevent GC from reclaiming
		std::cout << "[SDK] CreateRawWidget: created widget at " << (void*)Obj << " (GC rooted)\n";
	}
	else
		std::cout << "[SDK] CreateRawWidget: StaticConstructObject returned null\n";

	return static_cast<UWidget*>(Obj);
}
UWidget* CreateRawWidgetFromTemplate(UClass* WidgetClass, UObject* Outer, UObject* TemplateObj, const char* Tag)
{
	if (!WidgetClass || !Outer || !TemplateObj)
		return nullptr;

	static uintptr_t Base = (uintptr_t)GetModuleHandle(nullptr);
	using StaticConstructObjectFn = UObject* (__fastcall*)(const void*);
	static auto StaticConstructObject = reinterpret_cast<StaticConstructObjectFn>(Base + 0x17C6140);

	struct alignas(8) FParams {
		UClass*  Class;           // +0x00
		UObject* Outer;           // +0x08
		uint64_t Name;            // +0x10
		uint32_t SetFlags;        // +0x18
		uint32_t InternalFlags;   // +0x1C
		uint8_t  bCopyTransients; // +0x20
		uint8_t  bAssumeTemplate; // +0x21
		uint8_t  Pad[6];          // +0x22
		UObject* Template;        // +0x28
		void*    InstanceGraph;   // +0x30
		void*    ExternalPkg;     // +0x38
	};
	static_assert(sizeof(FParams) == 0x40, "FStaticConstructObjectParameters size mismatch");

	FParams Params = {};
	Params.Class = WidgetClass;
	Params.Outer = Outer;
	Params.SetFlags = 8;
	Params.bCopyTransients = 1;
	Params.bAssumeTemplate = 1;
	Params.Template = TemplateObj;

	UObject* Obj = StaticConstructObject(&Params);
	if (!Obj)
	{
		std::cout << "[SDK] CreateRawWidgetFromTemplate(" << (Tag ? Tag : "?")
		          << "): null\n";
		return nullptr;
	}

	MarkAsGCRoot(Obj);
	std::cout << "[SDK] CreateRawWidgetFromTemplate(" << (Tag ? Tag : "?")
	          << "): " << (void*)Obj << " from template=" << (void*)TemplateObj << "\n";
	return static_cast<UWidget*>(Obj);
}
UTextBlock* CreateRawTextLabel(UObject* Outer, const wchar_t* Text)
{
	auto* Label = static_cast<UTextBlock*>(CreateRawWidget(UTextBlock::StaticClass(), Outer));
	if (!Label)
		return nullptr;

	Label->SetText(MakeText(Text));
	Label->SetAutoWrapText(true);
	return Label;
}

// ── Slot content helpers ──

