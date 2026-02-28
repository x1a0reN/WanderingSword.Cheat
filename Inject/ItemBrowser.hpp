#pragma once

#include "CheatState.hpp"

/// 从 UItemResManager::ItemDataTable 的 RowMap 遍历所有物品，
/// 解析 FItemInfoSetting 结构并缓存到 GAllItems (DefId/名称/品质/子类型/图标)。
void BuildItemCache();

/// 通过 ProcessEvent 调用 UImage::SetBrushFromSoftTexture，
/// 将 28 字节的 TSoftObjectPtr<UTexture2D> 数据设置为图片控件的画刷。
void SetImageFromSoftTextureBySDK(UImage* ImageWidget, const uint8* SoftTextureData28);

/// 按分类过滤物品缓存: 0=全部, 1=武器, 2=防具, 3=消耗品, 4=其他。
/// 结果写入 GFilteredIndices，并重置当前页为第 0 页。
void FilterItems(int32 category);

/// 刷新当前页的物品显示: 根据 GItemCurrentPage 和 GFilteredIndices
/// 更新 24 格物品图标网格 + 翻页按钮状态 + 页码标签。
void RefreshItemPage();

/// 清理物品浏览器的所有 Widget 状态指针 (网格/翻页按钮/输入框等)，
/// 在面板关闭或销毁时调用。
void ClearItemBrowserState();
