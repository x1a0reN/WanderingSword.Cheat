#pragma once

#include "CheatState.hpp"

void BuildItemCache();
void SetImageFromSoftTextureBySDK(UImage* ImageWidget, const uint8* SoftTextureData28);
void FilterItems(int32 category);
void RefreshItemPage();
void ClearItemBrowserState();
