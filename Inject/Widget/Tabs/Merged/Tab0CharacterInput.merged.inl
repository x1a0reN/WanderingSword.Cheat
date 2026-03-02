void PollTab0CharacterInput(bool bTab0Active)
{
	Tab0Trace("InputPoll.Begin", "tab0Active=", (bTab0Active ? 1 : 0), " bindingsBefore=", GTab0Bindings.size());

	// 方案1: 清理降频 - 每500ms运行一次清理，而不是每帧都运行
	const ULONGLONG Now = GetTickCount64();
	const bool bDoCleanup = !GTab0LastCleanupTick || (Now - GTab0LastCleanupTick) >= kTab0CleanupIntervalMs;
	if (bDoCleanup)
	{
		GTab0Bindings.erase(
			std::remove_if(
				GTab0Bindings.begin(),
				GTab0Bindings.end(),
				[](const FTab0Binding& Binding)
				{
					return !Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit));
				}),
			GTab0Bindings.end());
		Tab0Trace("InputPoll.BindingsPruned", "bindingsAfter=", GTab0Bindings.size());

		if (GTab0LastFocusedEdit &&
			!IsSafeLiveObject(static_cast<UObject*>(GTab0LastFocusedEdit)))
		{
			GTab0LastFocusedEdit = nullptr;
		}
		// 同时清理焦点缓存中无效的条目
		if (GTab0FocusCacheEdit && !IsSafeLiveObject(static_cast<UObject*>(GTab0FocusCacheEdit)))
		{
			GTab0FocusCacheEdit = nullptr;
		}
		GTab0LastCleanupTick = Now;
	}

	const bool EnterDown = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
	const bool EnterTriggered = EnterDown && !GTab0EnterWasDown;
	Tab0Trace("InputPoll.KeyState", "enterDown=", (EnterDown ? 1 : 0), " enterTriggered=", (EnterTriggered ? 1 : 0));
	if (!bTab0Active)
	{
		PollTab0RoleDropdowns(nullptr, false);
		GTab0EnterWasDown = EnterDown;
		GTab0LastFocusedEdit = nullptr;
		GTab0FocusCacheEdit = nullptr;  // Tab 不活跃时清空焦点缓存
		Tab0Trace("InputPoll.End", "reason=TabInactive");
		return;
	}

	APlayerController* PC = nullptr;
	if (UWorld* World = UWorld::GetWorld())
		PC = UGameplayStatics::GetPlayerController(World, 0);
	Tab0Trace("InputPoll.Controller", "pc=", (void*)PC);
	if (!PC)
		Tab0Trace("InputPoll.Controller", "reason=PlayerControllerNull");

	// 降频轮询: 每100ms运行一次下拉框和滑块轮询
	const bool bDoUiPoll = !GTab0LastUiPollTick || (Now - GTab0LastUiPollTick) >= kTab0UiPollIntervalMs;
	if (bDoUiPoll)
	{
		// 下拉框轮询
		PollTab0RoleDropdowns(PC, true);
		// Tab0 三个倍率滑块：实时同步到游戏属性
		PollTab0RatioSliders(PC);
		GTab0LastUiPollTick = Now;
	}

	// 方案2: 焦点缓存 - 只有在 EnterTriggered 或缓存过期时才扫描焦点
	// 计算缓存是否过期
	const bool bCacheExpired = !GTab0FocusCacheTick || (Now - GTab0FocusCacheTick) >= kTab0FocusCacheDurationMs;
	// 检查缓存的编辑框是否仍然有效
	const bool bCachedEditValid = GTab0FocusCacheEdit && IsSafeLiveObject(static_cast<UObject*>(GTab0FocusCacheEdit));
	// 是否需要进行焦点扫描: EnterTriggered 时必须扫描，或者缓存已过期，或者缓存的编辑框已失效
	const bool bNeedFocusScan = EnterTriggered || bCacheExpired || !bCachedEditValid;

	FTab0Binding* Focused = nullptr;
	if (bNeedFocusScan)
	{
		// 执行真正的焦点扫描
		for (auto& Binding : GTab0Bindings)
		{
			if (!Binding.Edit || !IsSafeLiveObject(static_cast<UObject*>(Binding.Edit)))
				continue;
			if (Binding.Edit->HasKeyboardFocus())
			{
				Focused = &Binding;
				break;
			}
		}
		// 更新缓存
		GTab0FocusCacheEdit = Focused ? Focused->Edit : nullptr;
		GTab0FocusCacheTick = Now;
	}
	else
	{
		// 使用缓存的结果
		Focused = FindTab0BindingByEdit(GTab0FocusCacheEdit);
	}
	Tab0Trace("InputPoll.FocusScan",
		"focused=", (void*)(Focused ? Focused->Edit : nullptr),
		" lastFocused=", (void*)GTab0LastFocusedEdit,
		" cached=", (!bNeedFocusScan ? 1 : 0));

	// 某些编辑框按下回车瞬间会丢失键盘焦点，回车提交时回退到“上一次有焦点的编辑框”。
	if (EnterTriggered && !Focused && GTab0LastFocusedEdit)
	{
		Focused = FindTab0BindingByEdit(GTab0LastFocusedEdit);
		Tab0Trace("InputPoll.FallbackFocus", "fromLast=", (void*)GTab0LastFocusedEdit, " to=", (void*)(Focused ? Focused->Edit : nullptr));
		if (kTab0VerboseLog && Focused)
			std::cout << "[SDK][Tab0Input] EnterTriggered fallbackFocus field="
			          << Tab0FieldToString(Focused->Field) << "\n";
	}

	if (Focused && Focused->Edit)
	{
		const std::string RawFocused = Focused->Edit->GetText().ToString();
		const std::string SanitizedFocused = SanitizeNumericInputText(RawFocused, Focused->bInteger);
		if (SanitizedFocused != RawFocused)
		{
			const std::wstring Wide = AsciiToWide(SanitizedFocused);
			Focused->Edit->SetText(MakeText(Wide.c_str()));
			Tab0Trace("InputPoll.SanitizeLive",
				"field=", Tab0FieldToString(Focused->Field),
				" from=", RawFocused,
				" to=", SanitizedFocused);
		}
	}

	if (!EnterTriggered &&
		GTab0LastFocusedEdit &&
		(!Focused || Focused->Edit != GTab0LastFocusedEdit))
	{
		FTab0Binding* LastFocusedBinding = FindTab0BindingByEdit(GTab0LastFocusedEdit);
		if (LastFocusedBinding)
		{
			Tab0Trace("InputPoll.RollbackOnBlur", "field=", Tab0FieldToString(LastFocusedBinding->Field));
			RefreshSingleTab0BindingText(*LastFocusedBinding, PC);
		}
	}

	if (EnterTriggered)
	{
		if (kTab0VerboseLog && !Focused)
			std::cout << "[SDK][Tab0Input] EnterTriggered but no focused editable binding\n";
		if (!Focused)
			Tab0Trace("InputPoll.Commit.Skip", "reason=NoFocusedBinding");

		if (Focused && Focused->Edit &&
			IsSafeLiveObject(static_cast<UObject*>(Focused->Edit)))
		{
			std::string Raw = Focused->Edit->GetText().ToString();
			Raw = SanitizeNumericInputText(Raw, Focused->bInteger);
			const std::wstring Wide = AsciiToWide(Raw);
			Focused->Edit->SetText(MakeText(Wide.c_str()));

			char* EndPtr = nullptr;
			const double Parsed = std::strtod(Raw.c_str(), &EndPtr);
			const bool bParsedAll = (EndPtr && *EndPtr == '\0');
			if (kTab0VerboseLog)
				std::cout << "[SDK][Tab0Input] EnterTriggered field=" << Tab0FieldToString(Focused->Field)
				          << " raw=" << Raw << " parsed=" << Parsed
				          << " parsedAll=" << (bParsedAll ? 1 : 0) << "\n";
			Tab0Trace("InputPoll.Commit.Parse",
				"field=", Tab0FieldToString(Focused->Field),
				" raw=", Raw,
				" parsed=", Parsed,
				" parsedAll=", (bParsedAll ? 1 : 0));
			if (EndPtr != Raw.c_str() && bParsedAll)
			{
				FTab0HeroContext Ctx = BuildTab0HeroContext(PC);
				Tab0Trace("InputPoll.Commit.Try", "field=", Tab0FieldToString(Focused->Field), " parsed=", Parsed);
				if (TrySetTab0FieldValue(Ctx, Focused->Field, Parsed))
				{
					RefreshTab0BindingsText(PC);
					std::cout << "[SDK] Tab0Commit: " << (Focused->Title ? "ok" : "unnamed")
					          << " raw=" << Raw << "\n";
					Tab0Trace("InputPoll.Commit.Ok", "field=", Tab0FieldToString(Focused->Field));
				}
				else
				{
					std::cout << "[SDK][Tab0Input] Commit failed field=" << Tab0FieldToString(Focused->Field)
					          << " raw=" << Raw << "\n";
					Tab0Trace("InputPoll.Commit.Fail", "field=", Tab0FieldToString(Focused->Field));
				}
			}
			else
			{
				if (kTab0VerboseLog)
					std::cout << "[SDK][Tab0Input] Invalid numeric input, rollback field="
					          << Tab0FieldToString(Focused->Field) << " raw=" << Raw << "\n";
				RefreshSingleTab0BindingText(*Focused, PC);
				Tab0Trace("InputPoll.Commit.InvalidInput", "field=", Tab0FieldToString(Focused->Field), " raw=", Raw);
			}
		}
	}

	GTab0LastFocusedEdit = Focused ? Focused->Edit : nullptr;
	GTab0EnterWasDown = EnterDown;
	Tab0Trace("InputPoll.End", "focusedNow=", (void*)GTab0LastFocusedEdit, " enterWasDown=", (GTab0EnterWasDown ? 1 : 0));
}


