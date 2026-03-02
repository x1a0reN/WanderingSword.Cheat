void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC)
{
	if (!GDynTabContent8 || !CV || !PC) return;
	GDynTabContent8->ClearChildren();

	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(
		reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree)
		: static_cast<UObject*>(CV);
	UWidget* BtnLayout = nullptr;
	auto* ResetBtn = CreateGameStyleButton(PC, L"下一页", "Tab8Showcase",
		0.0f, 0.0f, &BtnLayout);
	if (BtnLayout)
	{
		GDynTabContent8->AddChild(BtnLayout);
	}
}








