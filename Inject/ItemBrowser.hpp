#pragma once

#include "CheatState.hpp"

/// 浠?UItemResManager::ItemDataTable 鐨?RowMap 閬嶅巻鎵€鏈夌墿鍝侊紝
/// 瑙ｆ瀽 FItemInfoSetting 缁撴瀯骞剁紦瀛樺埌 GAllItems (DefId/鍚嶇О/鍝佽川/瀛愮被鍨?鍥炬爣)銆?
void BuildItemCache();

/// 閫氳繃 ProcessEvent 璋冪敤 UImage::SetBrushFromSoftTexture锛?
/// 灏?28 瀛楄妭鐨?TSoftObjectPtr<UTexture2D> 鏁版嵁璁剧疆涓哄浘鐗囨帶浠剁殑鐢诲埛銆?
void SetImageFromSoftTextureBySDK(UImage* ImageWidget, const uint8* SoftTextureData28);

/// 鎸夊垎绫昏繃婊ょ墿鍝佺紦瀛? 0=鍏ㄩ儴, 1=姝﹀櫒, 2=闃插叿, 3=娑堣€楀搧, 4=鍏朵粬銆?
/// 缁撴灉鍐欏叆 GFilteredIndices锛屽苟閲嶇疆褰撳墠椤典负绗?0 椤点€?
void FilterItems(int32 category);

/// 鍒锋柊褰撳墠椤电殑鐗╁搧鏄剧ず: 鏍规嵁 GItemCurrentPage 鍜?GFilteredIndices
/// 鏇存柊 24 鏍肩墿鍝佸浘鏍囩綉鏍?+ 缈婚〉鎸夐挳鐘舵€?+ 椤电爜鏍囩銆?
void RefreshItemPage();

/// 轮询物品格子悬浮状态，显示/隐藏游戏原生物品 Tips。
void PollItemBrowserHoverTips();

/// 娓呯悊鐗╁搧娴忚鍣ㄧ殑鎵€鏈?Widget 鐘舵€佹寚閽?(缃戞牸/缈婚〉鎸夐挳/杈撳叆妗嗙瓑)锛?
/// 鍦ㄩ潰鏉垮叧闂垨閿€姣佹椂璋冪敤銆?
void ClearItemBrowserState();
