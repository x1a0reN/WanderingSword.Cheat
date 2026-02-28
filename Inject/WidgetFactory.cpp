#include <Windows.h>
#include <algorithm>
#include <iostream>

#include "WidgetFactory.hpp"
#include "GCManager.hpp"
#include "WidgetUtils.hpp"
#include "SDK/VE_JHVideoPanel2_classes.hpp"
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

	// Enable hover-to-switch: NeoUI focus system calls HandleMainBtn() on mouse enter,
	// which triggers EVT_SyncTabIndex(TabIndex) — same as native tabs.
	TabBtn->AutoFocusForMouseEntering = true;

	// NOTE: Do NOT clamp TabIndex to Switcher range.
	// Dynamic tabs use index 6/7/8 which are out of Switcher range — that's intentional.
	// EVT_SyncTabIndex(6+) will silently fail on SetActiveWidgetIndex (no crash),
	// but it correctly deactivates all native tabs via EVT_UpdateActiveStatus.
	// FrameHook handles the content switch for dynamic tabs.

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
	UClass* TabClass = UBP_JHConfigTabBtn_C::StaticClass();
	if (!TabClass)
		TabClass = UObject::FindClassFast("BP_JHConfigTabBtn_C");
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
	}
	return Tab;
}

// ── GC Root prevention ──
// EObjectFlags::MarkAsRootSet = 0x80, at UObject offset 0x0008
// Prevents UE4 garbage collector from reclaiming dynamically created widgets


UJHCommon_Btn_Free_C* CreateGameStyleButton(
	APlayerController* PC,
	const wchar_t* LabelText,
	const char* LogTag,
	float Width,
	float Height,
	UWidget** OutLayoutWidget)
{
	if (!PC)
		return nullptr;

	auto* Btn = static_cast<UJHCommon_Btn_Free_C*>(
		UWidgetBlueprintLibrary::Create(PC, UJHCommon_Btn_Free_C::StaticClass(), PC));
	if (!Btn)
	{
		std::cout << "[SDK] " << (LogTag ? LogTag : "CreateGameStyleButton")
		          << ": create failed\n";
		return nullptr;
	}
	MarkAsGCRoot(Btn);

	Btn->SetVisibility(ESlateVisibility::Visible);
	Btn->SetIsEnabled(true);
	Btn->SetRenderOpacity(1.0f);
	Btn->SetRenderTranslation(FVector2D{ 0.0f, 0.0f });
	Btn->SetRenderScale(FVector2D{ 1.0f, 1.0f });
	Btn->GotoNormalStatus();

	if (LabelText && LabelText[0])
	{
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

	float W = (Width  > 0.0f) ? Width  : 160.0f;
	float H = (Height > 0.0f) ? Height : 56.0f;

	UWidget* LayoutWidget = Btn;
	auto* Box = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), PC));
	if (Box)
	{
		MarkAsGCRoot(Box);
		Box->SetWidthOverride(W);
		Box->SetHeightOverride(H);
		Box->SetContent(Btn);
		Box->SetVisibility(ESlateVisibility::Visible);
		LayoutWidget = Box;
	}

	if (OutLayoutWidget)
		*OutLayoutWidget = LayoutWidget;

	Btn->ForceLayoutPrepass();
	std::cout << "[SDK] " << (LogTag ? LogTag : "CreateGameStyleButton")
	          << ": ok btn=" << (void*)Btn << "\n";
	return Btn;
}
UWidget* CreateShowcaseResetButton(UBPMV_ConfigView2_C* CV, UObject* Outer, APlayerController* PC)
{
	if (!CV || !Outer || !PC)
		return nullptr;

	return CreateGameStyleButton(PC, L"\u4E0B\u4E00\u9875", "CreateShowcaseResetButton");
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
	static UClass* Cls = UBPVE_JHConfigVideoItem2_C::StaticClass();
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

	// Only clear the game-effect delegate — keeps dropdown UI fully functional
	ClearComboBoxGameBinding(Item->CB_Main);

	if (Item->TXT_Title)
		Item->TXT_Title->SetText(MakeText(Title));
	if (Item->IMG_Icon)
		Item->IMG_Icon->SetVisibility(ESlateVisibility::Collapsed);

	std::cout << "[SDK] CreateVideoItem: created OK (game binding cleared)\n";
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
	static UClass* Cls = UBPVE_JHConfigVolumeItem2_C::StaticClass();
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
	Item->SoundCls = EJHSoundClass::PlaceHolder;

	ClearSliderGameBinding(Item->VolumeSlider);
	if (Item->BTN_Minus)
		ClearButtonBindings(static_cast<UWidget*>(Item->BTN_Minus));
	if (Item->BTN_Plus)
		ClearButtonBindings(static_cast<UWidget*>(Item->BTN_Plus));

	if (Item->TXT_Title)
		Item->TXT_Title->SetText(MakeText(Title));
	if (Item->IMG_Icon)
		Item->IMG_Icon->SetVisibility(ESlateVisibility::Collapsed);

	GVolumeItems.push_back(Item);
	const float InitValue = Item->VolumeSlider ? Item->VolumeSlider->GetValue() : 0.0f;
	GVolumeLastValues.push_back(InitValue);
	GVolumeMinusWasPressed.push_back(false);
	GVolumePlusWasPressed.push_back(false);
	if (Item->TXT_CurrentValue)
	{
		float MinValue = 0.0f;
		float MaxValue = 1.0f;
		if (Item->VolumeSlider)
		{
			MinValue = Item->VolumeSlider->MinValue;
			MaxValue = Item->VolumeSlider->MaxValue;
		}
		float Norm = InitValue;
		if (MaxValue > MinValue)
			Norm = (InitValue - MinValue) / (MaxValue - MinValue);
		if (Norm < 0.0f) Norm = 0.0f;
		if (Norm > 1.0f) Norm = 1.0f;
		const int32 DisplayValue = static_cast<int32>(Norm * 100.0f + 0.5f);
		wchar_t Buf[16] = {};
		swprintf_s(Buf, 16, L"%d", DisplayValue);
		Item->TXT_CurrentValue->SetText(MakeText(Buf));
	}

	std::cout << "[SDK] CreateVolumeItem: created OK\n";
	return Item;
}

UBPVE_JHConfigVolumeItem2_C* CreateVolumeEditBoxItem(
	APlayerController* PC,
	UObject* Outer,
	UPanelWidget* FallbackContainer,
	const wchar_t* Title,
	const wchar_t* Hint,
	const wchar_t* DefaultValue)
{
	auto* Item = CreateVolumeItem(PC, Title);
	if (!Item)
		return nullptr;

	// 该行只用于编辑框展示，不参与滑块轮询接管
	auto It = std::find(GVolumeItems.begin(), GVolumeItems.end(), Item);
	if (It != GVolumeItems.end())
	{
		const size_t Idx = static_cast<size_t>(std::distance(GVolumeItems.begin(), It));
		GVolumeItems.erase(It);
		if (Idx < GVolumeLastValues.size())      GVolumeLastValues.erase(GVolumeLastValues.begin() + Idx);
		if (Idx < GVolumeMinusWasPressed.size()) GVolumeMinusWasPressed.erase(GVolumeMinusWasPressed.begin() + Idx);
		if (Idx < GVolumePlusWasPressed.size())  GVolumePlusWasPressed.erase(GVolumePlusWasPressed.begin() + Idx);
	}

	if (!Outer)
		Outer = PC;

	auto* Edit = static_cast<UEditableTextBox*>(
		CreateRawWidget(UEditableTextBox::StaticClass(), Outer));
	if (!Edit)
		return Item;

	Edit->SetHintText(MakeText(Hint));
	Edit->SetText(MakeText(DefaultValue));
	Edit->SetJustification(ETextJustify::Right);
	Edit->MinimumDesiredWidth = 320.0f;
	Edit->SelectAllTextWhenFocused = true;
	Edit->ClearKeyboardFocusOnCommit = false;
	Edit->Font.Size = 22;
	Edit->WidgetStyle.Font.Size = 22;
	Edit->WidgetStyle.Padding.Left = 10.0f;
	Edit->WidgetStyle.Padding.Top = 8.0f;
	Edit->WidgetStyle.Padding.Right = 10.0f;
	Edit->WidgetStyle.Padding.Bottom = 8.0f;
	ClearEditableTextBindings(Edit);

	auto MakeSlateColor = [](float R, float G, float B, float A) -> FSlateColor
	{
		FSlateColor C{};
		C.SpecifiedColor = FLinearColor{ R, G, B, A };
		C.ColorUseRule = ESlateColorStylingMode::UseColor_Specified;
		return C;
	};

	// 输入框透明，让底部保留原滑块行黑底样式
	Edit->ForegroundColor = FLinearColor{ 0.95f, 0.95f, 0.95f, 1.0f };
	Edit->BackgroundColor = FLinearColor{ 0.0f, 0.0f, 0.0f, 0.0f };
	Edit->WidgetStyle.ForegroundColor = MakeSlateColor(0.95f, 0.95f, 0.95f, 1.0f);
	Edit->WidgetStyle.BackgroundColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
	Edit->WidgetStyle.ReadOnlyForegroundColor = MakeSlateColor(0.75f, 0.75f, 0.75f, 1.0f);
	Edit->WidgetStyle.BackgroundImageNormal.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
	Edit->WidgetStyle.BackgroundImageHovered.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
	Edit->WidgetStyle.BackgroundImageFocused.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);
	Edit->WidgetStyle.BackgroundImageReadOnly.TintColor = MakeSlateColor(0.0f, 0.0f, 0.0f, 0.0f);

	UWidget* InputWidget = Edit;
	auto* InputSize = static_cast<USizeBox*>(CreateRawWidget(USizeBox::StaticClass(), Outer));
	if (InputSize)
	{
		InputSize->SetWidthOverride(340.0f);
		InputSize->SetHeightOverride(50.0f);
		InputSize->SetContent(InputWidget);
		InputWidget = InputSize;
	}

	UPanelWidget* ValuePanel = nullptr;
	const char* ValuePanelSource = "none";
	int ValuePanelScore = -1;

	auto GetPanelTypeName = [](UPanelWidget* P) -> const char*
	{
		if (!P) return "null";
		if (P->IsA(UHorizontalBox::StaticClass())) return "HorizontalBox";
		if (P->IsA(UOverlay::StaticClass())) return "Overlay";
		if (P->IsA(UCanvasPanel::StaticClass())) return "CanvasPanel";
		if (P->IsA(UVerticalBox::StaticClass())) return "VerticalBox";
		return "PanelWidget";
	};

	auto GetPanelScore = [](UPanelWidget* P) -> int
	{
		if (!P) return -1;
		if (P->IsA(UHorizontalBox::StaticClass())) return 100;
		if (P->IsA(UOverlay::StaticClass())) return 90;
		if (P->IsA(UCanvasPanel::StaticClass())) return 60;
		if (P->IsA(UVerticalBox::StaticClass())) return 50;
		return 40;
	};

	auto ConsiderPanel = [&](UPanelWidget* Candidate, const char* SourceTag, int Depth)
	{
		if (!Candidate)
			return;
		const int Score = GetPanelScore(Candidate) - (Depth * 4);
		if (Score > ValuePanelScore)
		{
			ValuePanelScore = Score;
			ValuePanel = Candidate;
			ValuePanelSource = SourceTag ? SourceTag : "unknown";
		}
	};

	auto ProbeParentChain = [&](UWidget* Leaf, const char* SourceTag)
	{
		UWidget* Cursor = Leaf;
		for (int Depth = 0; Cursor && Depth < 6; ++Depth)
		{
			auto* Parent = Cursor->GetParent();
			if (!Parent)
				break;
			ConsiderPanel(Parent, SourceTag, Depth);
			Cursor = static_cast<UWidget*>(Parent);
		}
	};

	// 优先从数值文本链路找右侧容器，避免误拿到根 Canvas 导致输入框落在左侧(0,0)。
	ProbeParentChain(Item->TXT_CurrentValue, "TXT_CurrentValue");
	ProbeParentChain(static_cast<UWidget*>(Item->BTN_Plus), "BTN_Plus");
	ProbeParentChain(static_cast<UWidget*>(Item->BTN_Minus), "BTN_Minus");
	ProbeParentChain(Item->VolumeSlider, "VolumeSlider");

	if (Item->BTN_Minus)
	{
		Item->BTN_Minus->SetVisibility(ESlateVisibility::Collapsed);
		Item->BTN_Minus->SetIsEnabled(false);
	}
	if (Item->BTN_Plus)
	{
		Item->BTN_Plus->SetVisibility(ESlateVisibility::Collapsed);
		Item->BTN_Plus->SetIsEnabled(false);
	}
	if (Item->VolumeSlider)
		Item->VolumeSlider->SetVisibility(ESlateVisibility::Collapsed);
	if (Item->TXT_CurrentValue)
		Item->TXT_CurrentValue->SetVisibility(ESlateVisibility::Collapsed);

	if (ValuePanel)
	{
		UPanelSlot* AddedSlot = nullptr;
		if (ValuePanel->IsA(UHorizontalBox::StaticClass()))
		{
			AddedSlot = static_cast<UHorizontalBox*>(ValuePanel)->AddChildToHorizontalBox(InputWidget);
		}
		else if (ValuePanel->IsA(UCanvasPanel::StaticClass()))
		{
			auto* CanvasSlot = static_cast<UCanvasPanel*>(ValuePanel)->AddChildToCanvas(InputWidget);
			AddedSlot = CanvasSlot;
			if (CanvasSlot)
			{
				FAnchors Anchors{};
				Anchors.Minimum = FVector2D{ 0.56f, 0.0f };
				Anchors.Maximum = FVector2D{ 0.98f, 1.0f };
				CanvasSlot->SetAnchors(Anchors);

				FMargin Offsets{};
				Offsets.Left = 0.0f;
				Offsets.Top = 0.0f;
				Offsets.Right = 0.0f;
				Offsets.Bottom = 0.0f;
				CanvasSlot->SetOffsets(Offsets);
				CanvasSlot->SetAlignment(FVector2D{ 0.0f, 0.5f });
			}
		}
		else
		{
			AddedSlot = ValuePanel->AddChild(InputWidget);
		}

		if (AddedSlot && AddedSlot->IsA(UHorizontalBoxSlot::StaticClass()))
		{
			auto* HSlot = static_cast<UHorizontalBoxSlot*>(AddedSlot);
			HSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
			HSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
			FSlateChildSize AutoSize{};
			AutoSize.Value = 0.0f;
			AutoSize.SizeRule = ESlateSizeRule::Automatic;
			HSlot->SetSize(AutoSize);
			FMargin Padding{};
			Padding.Left = 8.0f;
			Padding.Top = 0.0f;
			Padding.Right = 0.0f;
			Padding.Bottom = 0.0f;
			HSlot->SetPadding(Padding);
		}

		std::cout << "[SDK] CreateVolumeEditBoxItem: item=" << (void*)Item
		          << " panel=" << (void*)ValuePanel
		          << " panelType=" << GetPanelTypeName(ValuePanel)
		          << " panelSource=" << ValuePanelSource
		          << " panelScore=" << ValuePanelScore
		          << " children=" << ValuePanel->GetChildrenCount()
		          << " addedSlot=" << (void*)AddedSlot << "\n";
	}
	else if (FallbackContainer)
	{
		// 兜底：仅在完全找不到内部面板时才加到外层容器，并做右移避免压到左侧标题
		FallbackContainer->AddChild(InputWidget);
		InputWidget->SetRenderTranslation(FVector2D{ 260.0f, 0.0f });
		std::cout << "[SDK] CreateVolumeEditBoxItem: fallback add to container="
		          << (void*)FallbackContainer << "\n";
	}

	return Item;
}

UVE_JHVideoPanel2_C* CreateCollapsiblePanel(APlayerController* PC, const wchar_t* Title, bool bStartCollapsed)
{
	static UClass* Cls = UVE_JHVideoPanel2_C::StaticClass();
	if (!Cls)
	{
		Cls = UObject::FindClassFast("VE_JHVideoPanel2_C");
		if (!Cls)
			Cls = UObject::FindClass("VE_JHVideoPanel2_C");
	}
	if (!Cls)
	{
		std::cout << "[SDK] CreateCollapsiblePanel: class not found\n";
		return nullptr;
	}

	auto* Panel = static_cast<UVE_JHVideoPanel2_C*>(
		UWidgetBlueprintLibrary::Create(PC, Cls, PC));
	if (!Panel)
	{
		std::cout << "[SDK] CreateCollapsiblePanel: Create returned null\n";
		return nullptr;
	}
	MarkAsGCRoot(Panel);

	// 先执行 Construct，确保蓝图子控件(txt/CT_Contents/SlotContents)已创建
	Panel->Construct();

	const FText TitleText = MakeText(Title ? Title : L"");
	Panel->TXT_Title = TitleText;
	Panel->UpdateTitle(TitleText);
	if (Panel->txt)
		Panel->txt->SetText(TitleText);

	Panel->IsCollapsed = bStartCollapsed;
	if (Panel->CT_Contents)
	{
		Panel->CT_Contents->SetVisibility(
			bStartCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	else if (Panel->SlotContents)
	{
		// 兜底：若蓝图没有正确赋值 CT_Contents，则手动创建并挂到 NamedSlot
		auto* FallbackContents = static_cast<UNeoUIVerticalBox*>(
			CreateRawWidget(UNeoUIVerticalBox::StaticClass(), Panel));
		if (FallbackContents)
		{
			Panel->SlotContents->SetContent(FallbackContents);
			Panel->CT_Contents = FallbackContents;
			Panel->CT_Contents->SetVisibility(
				bStartCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
	}

	std::cout << "[SDK] CreateCollapsiblePanel: created OK, title="
	          << (Title ? "set" : "null")
	          << " txt=" << (void*)Panel->txt
	          << " contents=" << (void*)Panel->CT_Contents
	          << " slotContents=" << (void*)Panel->SlotContents
	          << " collapsed=" << (Panel->IsCollapsed ? 1 : 0) << "\n";
	return Panel;
}
