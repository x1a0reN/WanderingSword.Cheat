#include <iostream>
#include <algorithm>

#include "GCManager.hpp"
void MarkAsGCRoot(UObject* Obj)
{
	if (!Obj)
		return;
	auto* FlagsPtr = reinterpret_cast<int32*>(reinterpret_cast<uintptr_t>(Obj) + 0x0008);
	*FlagsPtr |= 0x80; // RF_MarkAsRootSet
	if (std::find(GRootedObjects.begin(), GRootedObjects.end(), Obj) == GRootedObjects.end())
		GRootedObjects.push_back(Obj);
}
void ClearGCRoot(UObject* Obj)
{
	if (!Obj)
		return;
	auto* FlagsPtr = reinterpret_cast<int32*>(reinterpret_cast<uintptr_t>(Obj) + 0x0008);
	*FlagsPtr &= ~0x80; // clear RF_MarkAsRootSet
	auto it = std::remove(GRootedObjects.begin(), GRootedObjects.end(), Obj);
	if (it != GRootedObjects.end())
		GRootedObjects.erase(it, GRootedObjects.end());
}
void ClearAllGCRoots()
{
	if (GRootedObjects.empty())
		return;

	std::vector<UObject*> RootedSnapshot = GRootedObjects;
	for (UObject* Obj : RootedSnapshot)
		ClearGCRoot(Obj);
	std::cout << "[SDK] ClearAllGCRoots: released " << RootedSnapshot.size() << " rooted objects\n";
}

// ── Delegate clearing ──
// TMulticastInlineDelegate is 0x10 bytes. Zeroing it removes all blueprint bindings.
// This prevents cloned widgets from triggering the original game logic
// (e.g., VolumeItem slider → changes actual game volume).


