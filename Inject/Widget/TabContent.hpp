#pragma once

#include "CheatState.hpp"

UPanelWidget* GetOrCreateSlotContainer(UBPMV_ConfigView2_C* CV, UNeoUINamedSlot* Slot, const char* SlotName);

void PopulateTab_Character(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void PopulateTab_Items(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void PopulateTab_Battle(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void PopulateTab_Life(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void PopulateTab_Social(UBPMV_ConfigView2_C* CV, APlayerController* PC);
int32 GetGiftQualityMinIndex();
void SetGiftQualityMinIndex(int32 QualityIndex);

void PopulateTab_System(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void PopulateTab_Teammates(UBPMV_ConfigView2_C* CV, APlayerController* PC);
void PollTab6NpcPrototypeSelection(bool bTab6Active);
UObject* GetTab6NpcConfirmAlertForProcessEventHook();
void HandleTab6NpcConfirmProcessEventAction(bool bConfirmButton);

void PopulateTab_Quests(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void PopulateTab_Controls(UBPMV_ConfigView2_C* CV, APlayerController* PC);

void PollTab0CharacterInput(bool bTab0Active);
