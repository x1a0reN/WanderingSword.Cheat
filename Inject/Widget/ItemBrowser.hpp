#pragma once

#include "CheatState.hpp"

/// 浠?UItemResManager::ItemDataTable 鐨?RowMap 閬嶅巻鎵€鏈夌墿鍝侊紝
/// 瑙ｆ瀽 FItemInfoSetting 缁撴瀯骞剁紦瀛樺埌 GItemBrowser.AllItems (DefId/鍚嶇О/鍝佽川/瀛愮被鍨?鍥炬爣)銆?
void BuildItemCache();

/// 閫氳繃 ProcessEvent 璋冪敤 UImage::SetBrushFromSoftTexture锛?
/// 灏?28 瀛楄妭鐨?TSoftObjectPtr<UTexture2D> 鏁版嵁璁剧疆涓哄浘鐗囨帶浠剁殑鐢诲埛銆?
void SetImageFromSoftTextureBySDK(UImage* ImageWidget, const uint8* SoftTextureData28);

/// 鎸夊垎绫昏繃婊ょ墿鍝佺紦瀛? 0=鍏ㄩ儴, 1=姝﹀櫒, 2=闃插叿, 3=娑堣€楀搧, 4=鍏朵粬銆?
/// 缁撴灉鍐欏叆 GFilteredIndices锛屽苟閲嶇疆褰撳墠椤典负绗?0 椤点€?
void FilterItems(int32 category);

/// 鍒锋柊褰撳墠椤电殑鐗╁搧鏄剧ず: 鏍规嵁 GItemBrowser.CurrentPage 鍜?GItemBrowser.FilteredIndices
/// 鏇存柊 24 鏍肩墿鍝佸浘鏍囩綉鏍?+ 缈婚〉鎸夐挳鐘舵€?+ 椤电爜鏍囩銆?
void RefreshItemPage();

/// 轮询物品格子悬浮状态，显示/隐藏游戏原生物品 Tips。
void PollItemBrowserHoverTips();

/// 在 Tab8 真正显示后触发：先做布局预热，再刷新物品页，避免初始化抢跑导致 entry 不生成。
void OnItemBrowserTabShown();

/// 由外部（如 Backpack ProcessEvent）注入可用的 entry-init context（grid+0x8A0）。
void CacheEntryInitContextWeakB(const FWeakObjectPtr& WeakB, const char* SourceTag = nullptr);

/// 使用锚点窗口（anchor±window）扫描现存 ItemGrid 的 entry-init ctx（+0x8A0）并缓存。
/// OutChanged=true 表示与上次缓存相比发生变化，调用方可据此重建物品管理器。
bool RefreshEntryInitContextFromAnchorScan(bool* OutChanged = nullptr);

/// 绑定物品浏览器搜索输入框。
void SetItemSearchEditBox(UEditableTextBox* Edit);

/// 从搜索输入框读取文本并更新过滤关键字；返回是否发生变化。
bool UpdateItemSearchKeywordFromEdit();

/// 娓呯悊鐗╁搧娴忚鍣ㄧ殑鎵€鏈?Widget 鐘舵€佹寚閽?(缃戞牸/缈婚〉鎸夐挳/杈撳叆妗嗙瓑)锛?
/// 鍦ㄩ潰鏉垮叧闂垨閿€姣佹椂璋冪敤銆?
void ClearItemBrowserState();
