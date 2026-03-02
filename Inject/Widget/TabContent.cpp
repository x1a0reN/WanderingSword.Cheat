#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <cctype>
#include <utility>
#include "TabContent.hpp"
#include "GCManager.hpp"
#include "ItemBrowser.hpp"
#include "InlineHook.hpp"
#include "WidgetFactory.hpp"
#include "WidgetUtils.hpp"
#include "SDK/BPEntry_Item_classes.hpp"
#include "SDK/BP_ItemGridWDT_classes.hpp"
#include "SDK/JH_structs.hpp"
#include "Logging.hpp"

namespace
{
	template <typename... TParts>
	void UILog(const char* Tag, const TParts&... Parts)
	{
		(void)Tag;
		(void)sizeof...(Parts);
	}

}

UPanelWidget* GetOrCreateSlotContainer(UBPMV_ConfigView2_C* CV, UNeoUINamedSlot* Slot, const char* SlotName)
{
	if (!Slot)
	{
		UILog("CreateSlot", "[SDK] ", SlotName, ": slot pointer is null");
		return nullptr;
	}

	
	
	
	while (Slot->GetChildrenCount() > 0)
	{
		UWidget* Child = Slot->GetChildAt(0);
		if (Child)
		{
			
			if (Slot == CV->LanSlot && !GOriginalLanPanel)
			{
				GOriginalLanPanel = Child;
				MarkAsGCRoot(GOriginalLanPanel);
				UILog("CreateSlot", "[SDK] Captured original Lan panel: ", (void*)Child);
			}
			else if (Slot == CV->InputSlot && !GOriginalInputMappingPanel)
			{
				GOriginalInputMappingPanel = Child;
				MarkAsGCRoot(GOriginalInputMappingPanel);
				UILog("CreateSlot", "[SDK] Captured original InputMapping panel: ", (void*)Child);
			}

			UILog("CreateSlot", "[SDK] ", SlotName, ": removing game panel ", (void*)Child);
			Child->RemoveFromParent();
		}
		else
			break;
	}

	
	auto* WidgetTree = *reinterpret_cast<UWidgetTree**>(reinterpret_cast<uintptr_t>(CV) + 0x01D8);
	UObject* Outer = WidgetTree ? static_cast<UObject*>(WidgetTree) : static_cast<UObject*>(CV);



	UILog("CreateSlot", "[SDK] ", SlotName, ": creating UVerticalBox (WidgetTree=", (void*)WidgetTree, ")");

	auto* VBox = static_cast<UVerticalBox*>(
		CreateRawWidget(UVerticalBox::StaticClass(), Outer));
	if (!VBox)
	{
		UILog("CreateSlot", "[SDK] ", SlotName, ": failed to create UVerticalBox");
		return nullptr;
	}

	Slot->AddChild(VBox);
	UILog("CreateSlot", "[SDK] ", SlotName, ": UVerticalBox created and added to slot");
	return VBox;
}

#include "Tabs/Pages/Tab0Character.inl"
#include "Tabs/Pages/Tab1Items.inl"
#include "Tabs/Pages/Tab2Battle.inl"
#include "Tabs/Pages/Tab3Life.inl"
#include "Tabs/Pages/Tab4Social.inl"
#include "Tabs/Pages/Tab5System.inl"
#include "Tabs/Pages/Tab6Teammates.inl"
#include "Tabs/Pages/Tab7Quests.inl"
