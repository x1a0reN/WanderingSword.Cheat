#include <Windows.h>
#include <algorithm>
#include <cstring>

#include "WidgetUtils.hpp"
#include "GCManager.hpp"
#include "Logging.hpp"

namespace
{
	constexpr bool kEnableUICreateLog = false;
}

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
	if (!Obj)
		return false;
	if (!IsPointerInLiveObjectArray(Obj))
		return false;
	if (Obj->Flags & EObjectFlags::BeginDestroyed)
		return false;
	if (Obj->Flags & EObjectFlags::FinishDestroyed)
		return false;
	return UKismetSystemLibrary::IsValid(Obj);
}

bool IsSafeLiveObjectOfClass(UObject* Obj, UClass* ExpectedClass)
{
	if (!ExpectedClass || !IsSafeLiveObject(Obj))
		return false;
	return Obj->IsA(ExpectedClass);
}


FWeakObjectPtr ReadWeakPtrAt(UObject* Obj, uintptr_t Offset)
{
	FWeakObjectPtr Out{};
	if (!Obj)
		return Out;
	const uint8* Base = reinterpret_cast<const uint8*>(Obj);
	Out = *reinterpret_cast<const FWeakObjectPtr*>(Base + Offset);
	return Out;
}

void WriteWeakPtrAt(UObject* Obj, uintptr_t Offset, const FWeakObjectPtr& Value)
{
	if (!Obj)
		return;
	uint8* Base = reinterpret_cast<uint8*>(Obj);
	*reinterpret_cast<FWeakObjectPtr*>(Base + Offset) = Value;
}

bool IsWeakPtrFilled(const FWeakObjectPtr& Weak)
{
	return Weak.ObjectIndex >= 0 && Weak.ObjectSerialNumber > 0;
}

bool IsSameWeak(const FWeakObjectPtr& A, const FWeakObjectPtr& B)
{
	return A.ObjectIndex == B.ObjectIndex &&
		A.ObjectSerialNumber == B.ObjectSerialNumber;
}

UObject* ResolveWeakPtrLoose(const FWeakObjectPtr& Weak)
{
	if (!IsWeakPtrFilled(Weak))
		return nullptr;
	UObject* Obj = Weak.Get();
	if (!Obj)
		return nullptr;
	if (!IsSafeLiveObject(static_cast<UObject*>(Obj)))
		return nullptr;
	if (Obj->IsDefaultObject())
		return nullptr;
	return Obj;
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
	if (!GItemBrowser.QuantityEdit)
		return GItemBrowser.AddQuantity;

	FText T = GItemBrowser.QuantityEdit->GetText();
	FString S = UKismetTextLibrary::Conv_TextToString(T);
	const wchar_t* WS = S.CStr();
	if (!WS || !WS[0])
		return 1;

	int32 V = _wtoi(WS);
	if (V < 1) V = 1;
	if (V > 9999) V = 9999;
	return V;
}

void ClearDelegate(void* DelegatePtr)
{
	if (DelegatePtr)
		memset(DelegatePtr, 0, 0x10);
}

void ClearSliderBindings(USlider* Slider)
{
	if (!Slider) return;
	auto base = reinterpret_cast<uintptr_t>(Slider);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0498));
	ClearDelegate(reinterpret_cast<void*>(base + 0x04A8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x04B8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x04C8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x04D8));
}

void ClearSliderGameBinding(USlider* Slider)
{
	if (!Slider) return;
	auto base = reinterpret_cast<uintptr_t>(Slider);
	ClearDelegate(reinterpret_cast<void*>(base + 0x04A8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x04C8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x04D8));
}

void ClearButtonBindings(UWidget* Btn)
{
	if (!Btn) return;
	auto base = reinterpret_cast<uintptr_t>(Btn);
	ClearDelegate(reinterpret_cast<void*>(base + 0x03C8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x03D8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x03E8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x03F8));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0408));
}

void ClearComboBoxBindings(UComboBoxString* CB)
{
	if (!CB) return;
	auto base = reinterpret_cast<uintptr_t>(CB);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0D90));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0DA0));
}

void ClearComboBoxGameBinding(UComboBoxString* CB)
{
	if (!CB) return;
	auto base = reinterpret_cast<uintptr_t>(CB);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0D90));
}

void ClearEditableTextBindings(UEditableTextBox* Edit)
{
	if (!Edit) return;
	auto base = reinterpret_cast<uintptr_t>(Edit);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0A08));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0A18));
}

void ClearUserWidgetBindings(UUserWidget* UserWidget)
{
	if (!UserWidget) return;
	auto base = reinterpret_cast<uintptr_t>(UserWidget);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0168));
}

void ClearNeoUICommonButtonBindings(UWidget* Btn)
{
	if (!Btn || !Btn->IsA(UNeoUICommonButton::StaticClass()))
		return;
	auto base = reinterpret_cast<uintptr_t>(Btn);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0270));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0280));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0290));
	ClearDelegate(reinterpret_cast<void*>(base + 0x02A0));
}

void ClearJHGamepadConfirmButtonBindings(UWidget* Btn)
{
	if (!Btn || !Btn->IsA(UJHNeoUIGamepadConfirmButton::StaticClass()))
		return;

	auto* JHBtn = static_cast<UJHNeoUIGamepadConfirmButton*>(Btn);
	auto base = reinterpret_cast<uintptr_t>(JHBtn);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0540));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0550));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0560));
	ClearDelegate(reinterpret_cast<void*>(base + 0x0570));

	if (JHBtn->BtnMain)
		ClearButtonBindings(static_cast<UWidget*>(JHBtn->BtnMain));
}

void ClearJHCommonBtnFreeBindings(UWidget* Widget)
{
	if (!Widget || !Widget->IsA(UJHCommon_Btn_Free_C::StaticClass()))
		return;
	auto base = reinterpret_cast<uintptr_t>(Widget);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0590));
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

void ClearRegionTitleBindings(UWidget* Widget)
{
	UClass* RegionTitleClass = GetRegionTitleClass();
	if (!Widget || !RegionTitleClass || !Widget->IsA(RegionTitleClass))
		return;
	auto base = reinterpret_cast<uintptr_t>(Widget);
	ClearDelegate(reinterpret_cast<void*>(base + 0x0318));
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

	using StaticConstructObjectFn = UObject* (__fastcall*)(const void*);
	static auto StaticConstructObject = reinterpret_cast<StaticConstructObjectFn>(Base + 0x17C6140);

	struct alignas(8) FParams {
		UClass*  Class;
		UObject* Outer;
		uint64_t Name;
		uint32_t SetFlags;
		uint32_t InternalFlags;
		uint8_t  bCopyTransients;
		uint8_t  bAssumeTemplate;
		uint8_t  Pad[6];
		UObject* Template;
		void*    InstanceGraph;
		void*    ExternalPkg;
	};
	static_assert(sizeof(FParams) == 0x40, "FStaticConstructObjectParameters size mismatch");

	FParams Params = {};
	Params.Class = WidgetClass;
	Params.Outer = Outer;
	Params.SetFlags = 8;

	UObject* Obj = StaticConstructObject(&Params);
	if (Obj)
	{
		MarkAsGCRoot(Obj);
		if (kEnableUICreateLog)
			LOGI_STREAM("WidgetUtils") << "[SDK] CreateRawWidget: created widget at " << (void*)Obj << " (GC rooted)\n";
	}
	else if (kEnableUICreateLog)
	{
		LOGI_STREAM("WidgetUtils") << "[SDK] CreateRawWidget: StaticConstructObject returned null\n";
	}

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
		UClass*  Class;
		UObject* Outer;
		uint64_t Name;
		uint32_t SetFlags;
		uint32_t InternalFlags;
		uint8_t  bCopyTransients;
		uint8_t  bAssumeTemplate;
		uint8_t  Pad[6];
		UObject* Template;
		void*    InstanceGraph;
		void*    ExternalPkg;
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
		if (kEnableUICreateLog)
		{
			LOGI_STREAM("WidgetUtils") << "[SDK] CreateRawWidgetFromTemplate(" << (Tag ? Tag : "?")
			          << "): null\n";
		}
		return nullptr;
	}

	MarkAsGCRoot(Obj);
	if (kEnableUICreateLog)
	{
		LOGI_STREAM("WidgetUtils") << "[SDK] CreateRawWidgetFromTemplate(" << (Tag ? Tag : "?")
		          << "): " << (void*)Obj << " from template=" << (void*)TemplateObj << "\n";
	}
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
