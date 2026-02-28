#pragma once

#include <Windows.h>
#include <atomic>
#include <vector>

#include "SDK/Engine_classes.hpp"
#include "SDK/UMG_classes.hpp"
#include "SDK/NeoUI_classes.hpp"
#include "SDK/BPMV_ConfigView2_classes.hpp"
#include "SDK/BP_JHConfigTabBtn_classes.hpp"
#include "SDK/JHGPCBtn_classes.hpp"
#include "SDK/JHGPCBtn_ActiveBG_classes.hpp"
#include "SDK/JHCommon_Btn_Free_classes.hpp"
#include "SDK/JH_classes.hpp"
#include "SDK/BPVE_JHConfigVideoItem2_classes.hpp"
#include "SDK/BPVE_JHConfigVolumeItem2_classes.hpp"
#include "VTHook.hpp"

using namespace SDK;

/// PostRender Hook 原始函数签名
using GVCPostRenderFn = void(__fastcall*)(void* /* this */, void* /* Canvas */);

/// 面板缩放比例 (1.1x 整体放大)
inline constexpr float kInternalPanelScale = 1.1f;

/// UBP_JHConfigTabBtn_C 内部指向父级 ConfigView 的偏移 (用于 PatchTabBtnRuntimeContext)
inline constexpr uintptr_t kConfigTabBtnParentCtxOffset = 0x02E0;

/// 子模块面板 (如 VideoPanel2) 内部指向父级 ConfigView 的偏移
inline constexpr uintptr_t kConfigModuleView2ParentCtxOffset = 0x03B0;

/// 物品缓存条目: 从 DataTable 解析的物品信息
struct CachedItem {
	int32 DefId;            // 物品定义 ID
	wchar_t Name[64];       // 物品名称
	uint8 Quality;          // 品质等级
	uint8 SubType;          // 子类型 (用于分类过滤)
	uint8 IconData[0x28];   // TSoftObjectPtr<UTexture2D> 原始数据 (40字节)
	bool HasIcon;           // 是否有有效图标数据
};

/// 物品网格布局常量
inline constexpr int32 ITEM_GRID_COLS = 6;
inline constexpr int32 ITEM_GRID_ROWS = 4;
inline constexpr int32 ITEMS_PER_PAGE = ITEM_GRID_COLS * ITEM_GRID_ROWS; // 24

// ── Hook 与核心状态 ──

extern GVCPostRenderFn OriginalGVCPostRender;   // PostRender 原始函数指针
extern VTableHook GVCPostRenderHook;             // VTable Hook 实例
extern UUserWidget* InternalWidget;              // 作弊面板 Widget 实例
extern bool InternalWidgetVisible;               // 面板可见性标记
extern std::atomic<bool> GIsUnloading;           // DLL 正在卸载
extern std::atomic<bool> GHooksRemoved;          // Hook 已移除
extern std::atomic<bool> GConsoleClosed;         // 控制台已关闭

// ── 面板 UI 引用 ──

extern UButton* GCachedBtnExit;                  // 缓存的关闭按钮 (每帧检测点击)
extern std::vector<UObject*> GRootedObjects;     // 已标记 GC Root 的对象列表
extern UWidget* GOriginalLanPanel;               // 保留的原生语言面板
extern UWidget* GOriginalInputMappingPanel;      // 保留的原生键位面板
extern UJHCommon_Btn_Free_C* GOriginalResetButton; // 保留的原生重置按钮

// ── 物品浏览器状态 ──

extern std::vector<CachedItem> GAllItems;        // 全量物品缓存
extern std::vector<int32> GFilteredIndices;       // 当前分类过滤后的索引列表
extern bool GItemCacheBuilt;                      // 缓存是否已构建
extern int32 GItemCurrentPage;                    // 当前页码 (0-based)
extern int32 GItemTotalPages;                     // 总页数

extern UBPVE_JHConfigVideoItem2_C* GItemCategoryDD; // 物品分类下拉框
extern int32 GItemLastCatIdx;                     // 上次选中的分类索引
extern UHorizontalBox* GItemPagerRow;             // 翻页按钮行容器
extern UJHCommon_Btn_Free_C* GItemPrevPageBtn;    // 上一页按钮
extern UJHCommon_Btn_Free_C* GItemNextPageBtn;    // 下一页按钮
extern UTextBlock* GItemPageLabel;                // 页码显示标签
extern bool GItemPrevWasPressed;                  // 上一页按钮上帧按下状态 (边沿触发)
extern bool GItemNextWasPressed;                  // 下一页按钮上帧按下状态 (边沿触发)
extern UHorizontalBox* GItemQuantityRow;          // 添加数量输入行容器
extern UEditableTextBox* GItemQuantityEdit;       // 添加数量输入框
extern int32 GItemAddQuantity;                    // 当前添加数量值
extern UUniformGridPanel* GItemGridPanel;         // 6x4 物品图标网格
extern UButton* GItemSlotButtons[ITEMS_PER_PAGE]; // 24 个物品槽按钮
extern UImage* GItemSlotImages[ITEMS_PER_PAGE];   // 24 个物品槽图标
extern int32 GItemSlotItemIndices[ITEMS_PER_PAGE]; // 每个槽位对应的物品索引 (-1=空)
extern bool GItemSlotWasPressed[ITEMS_PER_PAGE];  // 物品槽上帧按下状态 (边沿触发)

// ── 动态 Tab (6/7/8) ──

extern UBP_JHConfigTabBtn_C* GDynTabBtn6;         // Tab 6 (队友) 按钮
extern UBP_JHConfigTabBtn_C* GDynTabBtn7;         // Tab 7 (任务) 按钮
extern UBP_JHConfigTabBtn_C* GDynTabBtn8;         // Tab 8 (控件) 按钮
extern UVerticalBox* GDynTabContent6;              // Tab 6 内容容器
extern UVerticalBox* GDynTabContent7;              // Tab 7 内容容器
extern UVerticalBox* GDynTabContent8;              // Tab 8 内容容器

// ── 队友 Tab 控件 ──

extern UBPVE_JHConfigVideoItem2_C* GTeammateFollowToggle;  // 设置队友跟随数量开关
extern UBPVE_JHConfigVolumeItem2_C* GTeammateFollowCount;   // 跟随数量滑块
extern UBPVE_JHConfigVideoItem2_C* GTeammateAddDD;          // 添加队友下拉框
extern UBPVE_JHConfigVideoItem2_C* GTeammateReplaceToggle;  // 替换指定队友开关
extern UBPVE_JHConfigVideoItem2_C* GTeammateReplaceDD;      // 指定队友下拉框

// ── 任务 Tab 控件 ──

extern UBPVE_JHConfigVideoItem2_C* GQuestToggle;   // 接到/完成任务开关
extern UBPVE_JHConfigVideoItem2_C* GQuestTypeDD;   // 执行类型下拉框 (接到/完成)
