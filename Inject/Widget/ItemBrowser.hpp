#pragma once

#include "CheatState.hpp"

void BuildItemCache();

void SetImageFromSoftTextureBySDK(UImage* ImageWidget, const uint8* SoftTextureData28);

void FilterItems(int32 category);

void RefreshItemPage();

void PollItemBrowserHoverTips();

void OnItemBrowserTabShown();

void CacheEntryInitContextWeakB(const FWeakObjectPtr& WeakB, const char* SourceTag = nullptr);

bool RefreshEntryInitContextFromAnchorScan(bool* OutChanged = nullptr);

void SetItemSearchEditBox(UEditableTextBox* Edit);

bool UpdateItemSearchKeywordFromEdit();

void ClearItemBrowserState();
