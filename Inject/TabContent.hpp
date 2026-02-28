#pragma once

#include "CheatState.hpp"

UPanelWidget* GetOrCreateSlotContainer(UBPMV_ConfigView2_C* CV, UNeoUINamedSlot* Slot, const char* SlotName);
void PopulateTab_Character(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_Life(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_Social(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_System(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC);
