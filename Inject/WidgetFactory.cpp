#include <Windows.h>
#include <iostream>

#include "WidgetFactory.hpp"
#include "GCManager.hpp"
#include "WidgetUtils.hpp"
void HideTabIcon(UJHNeoUIConfigV2TabBtn* TabBtn)
{
	if (!TabBtn)
		return;
	if (TabBtn->IMG_Icon)
		TabBtn->IMG_Icon->SetVisibility(ESlateVisibility::Collapsed);
}
void SetupTab(UJHNeoUIConfigV2TabBtn* TabBtn, int32_t Index, const wchar_t* Title)
{
	if (!TabBtn)
		return;

	TabBtn->TabIndex = Index;
	FText T = MakeText(Title);
	TabBtn->TXT_Title = T;
	if (TabBtn->txt)
		TabBtn->txt->SetText(T);
	HideTabIcon(TabBtn);
}
void PatchTabBtnRuntimeContext(UJHNeoUIConfigV2TabBtn* TabBtn, UBPMV_ConfigView2_C* CV, const char* SourceTag)
{
	if (!TabBtn || !CV)
		return;

	void* ParentCtx = nullptr;
	if (CV->BTN_Sound)
	{
		ParentCtx = *reinterpret_cast<void**>(
			reinterpret_cast<uintptr_t>(CV->BTN_Sound) + kConfigTabBtnParentCtxOffset);
	}
	if (!ParentCtx)
	{
		ParentCtx = reinterpret_cast<void*>(
			reinterpret_cast<uintptr_t>(CV) + kConfigModuleView2ParentCtxOffset);
	}

	*reinterpret_cast<void**>(
		reinterpret_cast<uintptr_t>(TabBtn) + kConfigTabBtnParentCtxOffset) = ParentCtx;

	// Keep hover path simple for custom/injected tab buttons.
	TabBtn->AutoFocusForMouseEntering = false;

	// Defensive clamp: showcase/demo button should not hold out-of-range index.
	if (CV->CT_Contents)
	{
		const int32 MaxTabIndex = CV->CT_Contents->GetNumWidgets() - 1;
		if (TabBtn->TabIndex < 0 || TabBtn->TabIndex > MaxTabIndex)
			TabBtn->TabIndex = 0;
	}

	if (TabBtn->BTN_Normal)
		TabBtn->RegisterMainInteractiveWidget(TabBtn->BTN_Normal);
	if (TabBtn->IMG_Active)
		TabBtn->RegisterActiveDisplayWidget(TabBtn->IMG_Active);

	std::cout << "[SDK] PatchTabBtnRuntimeContext("
		<< (SourceTag ? SourceTag : "?")
		<< "): tab=" << (void*)TabBtn
		<< " parentCtx=" << ParentCtx
		<< " tabIndex=" << TabBtn->TabIndex << "\n";
}
UBP_JHConfigTabBtn_C* CreateTabButton(APlayerController* PC)
{
	UClass* TabClass = UObject::FindClassFast("BP_JHConfigTabBtn_C");
	if (!TabClass)
		TabClass = UObject::FindClass("BP_JHConfigTabBtn_C");
	if (!TabClass)
		return nullptr;

	auto* Tab = static_cast<UBP_JHConfigTabBtn_C*>(
		UWidgetBlueprintLibrary::Create(PC, TabClass, PC));
	if (Tab)
	{
		MarkAsGCRoot(Tab);
		Tab->Construct();
		SanitizeWidgetTree(Tab, false);
	}
	return Tab;
}

// ── GC Root prevention ──
// EObjectFlags::MarkAsRootSet = 0x80, at UObject offset 0x0008
// Prevents UE4 garbage collector from reclaiming dynamically created widgets


static UJHCommon_Btn_Free_C* FindNativeGameResetButton()
{
	// Prefer cached native button if still valid.
	if (GNativeGameResetButton
		&& GNativeGameResetButton != GOriginalResetButton
		&& GNativeGameResetButton->GetParent()
		&& GNativeGameResetButton->IsVisible())
	{
		std::cout << "[SDK] ReuseResetBtnInstance(cache): btn=" << (void*)GNativeGameResetButton
		          << " parent=" << (void*)GNativeGameResetButton->GetParent()
		          << " vis=" << ToVisName(GNativeGameResetButton->GetVisibility())
		          << " opacity=" << GNativeGameResetButton->GetRenderOpacity() << "\n";
		return GNativeGameResetButton;
	}

	UClass* ConfigViewClass = UBPMV_ConfigView2_C::StaticClass();
	if (!ConfigViewClass)
		return nullptr;

	auto* ObjArray = UObject::GObjects.GetTypedPtr();
	if (!ObjArray)
		return nullptr;

	UJHCommon_Btn_Free_C* Best = nullptr;
	int Logged = 0;
	int32 Num = ObjArray->Num();
	for (int32 i = 0; i < Num; i++)
	{
		UObject* Obj = ObjArray->GetByIndex(i);
		if (!Obj || Obj == InternalWidget || !Obj->IsA(ConfigViewClass))
			continue;

		auto* CV = static_cast<UBPMV_ConfigView2_C*>(Obj);
		if (!CV || !CV->Btn_Revert2)
			continue;
		if (CV->Btn_Revert2 == GOriginalResetButton)
			continue;

		auto* Btn = CV->Btn_Revert2;
		auto* Parent = Btn->GetParent();
		const bool bViewport = CV->IsInViewport();
		const bool bVisible = Btn->IsVisible();

		if (Logged < 8)
		{
			std::cout << "[SDK] RevertCandidate: CV=" << (void*)CV
			          << " inViewport=" << (bViewport ? 1 : 0)
			          << " btn=" << (void*)Btn
			          << " parent=" << (void*)Parent
			          << " vis=" << ToVisName(Btn->GetVisibility())
			          << " opacity=" << Btn->GetRenderOpacity()
			          << "\n";
			Logged++;
		}

		// Accept only a live native settings panel instance.
		if (bViewport && Parent && bVisible)
		{
			Best = Btn;
			break;
		}
	}

	if (Best)
	{
		GNativeGameResetButton = Best;
		MarkAsGCRoot(GNativeGameResetButton);
	}
	else
	{
		std::cout << "[SDK] ReuseResetBtnInstance: no native viewport candidate matched\n";
	}

	return Best;
}
UJHCommon_Btn_Free_C* CreateGameStyleButton(
	APlayerController* PC,
	UJHCommon_Btn_Free_C* ReuseSource,
	const wchar_t* LabelText,
	const char* LogTag)
{
	UJHCommon_Btn_Free_C* Btn = ReuseSource;
	if (!Btn)
	{
		if (!PC)
			return nullptr;
		Btn = static_cast<UJHCommon_Btn_Free_C*>(
			UWidgetBlueprintLibrary::Create(PC, UJHCommon_Btn_Free_C::StaticClass(), PC));
		if (!Btn)
		{
			std::cout << "[SDK] " << (LogTag ? LogTag : "CreateGameStyleButton")
			          << ": fallback create failed\n";
			return nullptr;
		}
		MarkAsGCRoot(Btn);
	}
	else
	{
		ReuseDetachedWidget(Btn);
	}

	Btn->SetVisibility(ESlateVisibility::Visible);
	Btn->SetIsEnabled(true);
	Btn->SetRenderOpacity(1.0f);
	Btn->SetRenderTranslation(FVector2D{ 0.0f, 0.0f });
	Btn->SetRenderScale(FVector2D{ 1.0f, 1.0f });
	Btn->GotoNormalStatus();

	if (LabelText && LabelText[0])
	{
		// JHCommon_Btn_Free_C display text comes from NeoUI1Btn1TxtComp fields.
		// Writing only ChangeIFAData can be overridden back to default text ("确认").
		FText Label = MakeText(LabelText);
		Btn->TXT_Title = Label;
		Btn->UpdateTitle(Label);
		if (Btn->txt)
			Btn->txt->SetText(Label);
	}

	if (Btn->JHGPCBtn)
	{
		Btn->JHGPCBtn->SetVisibility(ESlateVisibility::Visible);
		Btn->JHGPCBtn->SetIsEnabled(true);
		Btn->JHGPCBtn->SetRenderOpacity(1.0f);
	}
	if (Btn->JHGPCBtn_ActiveBG)
		Btn->JHGPCBtn_ActiveBG->SetVisibility(ESlateVisibility::Collapsed);

	Btn->ForceLayoutPrepass();
	std::cout << "[SDK] " << (LogTag ? LogTag : "CreateGameStyleButton")
	          << ": ok btn=" << (void*)Btn
	          << " vis=" << ToVisName(Btn->GetVisibility())
	          << " parent=" << (void*)Btn->GetParent()
	          << "\n";
	return Btn;
}
UWidget* CreateShowcaseResetButton(UBPMV_ConfigView2_C* CV, UObject* Outer, APlayerController* PC)
{
	if (!CV || !Outer || !PC)
		return nullptr;

	UJHCommon_Btn_Free_C* Source = FindNativeGameResetButton();
	if (Source)
	{
		std::cout << "[SDK] ReuseResetBtnInstance(pre): btn=" << (void*)Source
		          << " parent=" << (void*)Source->GetParent()
		          << " vis=" << ToVisName(Source->GetVisibility())
		          << " opacity=" << Source->GetRenderOpacity() << "\n";
		if (auto* Reused = CreateGameStyleButton(PC, Source, L"\u4E0B\u4E00\u9875", "CreateGameStyleButton(reuse-reset)"))
			return Reused;
	}

	auto* Fresh = CreateGameStyleButton(PC, nullptr, L"\u4E0B\u4E00\u9875", "CreateGameStyleButton(fallback-reset)");
	if (!Fresh)
		return nullptr;
	std::cout << "[SDK] ReuseResetBtnInstance(fallback fresh): " << (void*)Fresh << "\n";
	return Fresh;
}
UWidget* CreateShowcaseConfigTabBtn(UBPMV_ConfigView2_C* CV, UObject* Outer)
{
	if (!CV || !Outer)
		return nullptr;

	UBP_JHConfigTabBtn_C* Source = CV->BTN_Sound ? CV->BTN_Sound : CV->BTN_Video;
	if (!Source)
		return nullptr;

	UWidget* Cloned = CreateRawWidgetFromTemplate(Source->Class, Outer, Source, "ConfigTabBtnClone");
	if (!Cloned)
	{
		std::cout << "[SDK] ConfigTabBtnClone failed\n";
		return nullptr;
	}
	return Cloned;
}
UWidget* CreateShowcaseWidgetByClassName(
	APlayerController* PC, const char* ClassName, bool bInvokeConstruct, UClass* FallbackClass)
{
	if (!PC)
		return nullptr;

	auto TryCreateWidget = [&](UClass* WidgetClass, const char* SourceName) -> UWidget*
	{
		if (!WidgetClass)
			return nullptr;
		auto* Widget = static_cast<UWidget*>(UWidgetBlueprintLibrary::Create(PC, WidgetClass, PC));
		if (!Widget)
		{
			std::cout << "[SDK] CreateShowcaseWidgetByClassName: create failed: "
			          << (SourceName ? SourceName : "<null>") << "\n";
			return nullptr;
		}
		MarkAsGCRoot(Widget);
		if (bInvokeConstruct && Widget->IsA(UUserWidget::StaticClass()))
			static_cast<UUserWidget*>(Widget)->Construct();
		return Widget;
	};

	if (ClassName && ClassName[0])
	{
		UClass* WidgetClass = UObject::FindClassFast(ClassName);
		if (!WidgetClass)
			WidgetClass = UObject::FindClass(ClassName);
		if (WidgetClass)
		{
			if (UWidget* W = TryCreateWidget(WidgetClass, ClassName))
				return W;
		}
		else
		{
			std::cout << "[SDK] CreateShowcaseWidgetByClassName: class not found: " << ClassName << "\n";
		}
	}

	if (FallbackClass)
	{
		std::cout << "[SDK] CreateShowcaseWidgetByClassName: fallback class try -> "
		          << (void*)FallbackClass << "\n";
		if (UWidget* W = TryCreateWidget(FallbackClass, "fallback"))
			return W;
	}

	return nullptr;
}
UBPVE_JHConfigVideoItem2_C* CreateVideoItem(APlayerController* PC, const wchar_t* Title)
{
	static UClass* Cls = nullptr;
	if (!Cls)
	{
		Cls = UObject::FindClassFast("BPVE_JHConfigVideoItem2_C");
		if (!Cls)
			Cls = UObject::FindClass("BPVE_JHConfigVideoItem2_C");
	}
	if (!Cls)
	{
		std::cout << "[SDK] CreateVideoItem: class BPVE_JHConfigVideoItem2_C not found\n";
		return nullptr;
	}

	auto* Item = static_cast<UBPVE_JHConfigVideoItem2_C*>(
		UWidgetBlueprintLibrary::Create(PC, Cls, PC));
	if (!Item)
	{
		std::cout << "[SDK] CreateVideoItem: UWidgetBlueprintLibrary::Create returned null\n";
		return nullptr;
	}

	MarkAsGCRoot(Item); // Prevent GC from reclaiming

	// Call Construct() directly — BPVE_JHConfigVideoItem2_functions.cpp is now compiled
	Item->Construct();

	// Clear original blueprint bindings — prevents dropdown from changing game settings
	ClearComboBoxBindings(Item->CB_Main);

	if (Item->TXT_Title)
		Item->TXT_Title->SetText(MakeText(Title));
	if (Item->IMG_Icon)
		Item->IMG_Icon->SetVisibility(ESlateVisibility::Collapsed);
	SanitizeWidgetTree(Item, false);

	std::cout << "[SDK] CreateVideoItem: created OK (bindings cleared)\n";
	return Item;
}

// Create a dropdown item with preset options (for toggle on/off or selection)
UBPVE_JHConfigVideoItem2_C* CreateVideoItemWithOptions(
	APlayerController* PC, const wchar_t* Title,
	std::initializer_list<const wchar_t*> Options)
{
	auto* Item = CreateVideoItem(PC, Title);
	if (!Item || !Item->CB_Main)
		return Item;

	Item->CB_Main->ClearOptions();
	for (const wchar_t* Opt : Options)
		Item->CB_Main->AddOption(FString(Opt));

	// Select first option by default
	if (Item->CB_Main->GetOptionCount() > 0)
		Item->CB_Main->SetSelectedIndex(0);

	return Item;
}

// Create a toggle (on/off dropdown)
UBPVE_JHConfigVideoItem2_C* CreateToggleItem(APlayerController* PC, const wchar_t* Title)
{
	return CreateVideoItemWithOptions(PC, Title, { L"\u5173", L"\u5F00" }); // 关, 开
}

// Create a slider/volume item for numeric values
UBPVE_JHConfigVolumeItem2_C* CreateVolumeItem(APlayerController* PC, const wchar_t* Title)
{
	static UClass* Cls = nullptr;
	if (!Cls)
	{
		Cls = UObject::FindClassFast("BPVE_JHConfigVolumeItem2_C");
		if (!Cls)
			Cls = UObject::FindClass("BPVE_JHConfigVolumeItem2_C");
	}
	if (!Cls)
	{
		std::cout << "[SDK] CreateVolumeItem: class not found\n";
		return nullptr;
	}

	auto* Item = static_cast<UBPVE_JHConfigVolumeItem2_C*>(
		UWidgetBlueprintLibrary::Create(PC, Cls, PC));
	if (!Item)
	{
		std::cout << "[SDK] CreateVolumeItem: Create returned null\n";
		return nullptr;
	}

	MarkAsGCRoot(Item); // Prevent GC from reclaiming

	Item->Construct();

	// Clear original blueprint bindings — prevents slider from changing game volume etc.
	ClearSliderBindings(Item->VolumeSlider);
	ClearButtonBindings(Item->BTN_Minus);
	ClearButtonBindings(Item->BTN_Plus);

	if (Item->TXT_Title)
		Item->TXT_Title->SetText(MakeText(Title));
	if (Item->IMG_Icon)
		Item->IMG_Icon->SetVisibility(ESlateVisibility::Collapsed);
	SanitizeWidgetTree(Item, false);

	std::cout << "[SDK] CreateVolumeItem: created OK (bindings cleared)\n";
	return Item;
}


