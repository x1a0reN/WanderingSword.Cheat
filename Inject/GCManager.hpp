#pragma once

#include "CheatState.hpp"

void MarkAsGCRoot(UObject* Obj);
void ClearGCRoot(UObject* Obj);
void ClearAllGCRoots();
