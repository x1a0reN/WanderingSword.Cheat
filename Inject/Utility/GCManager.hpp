#pragma once

#include "CheatState.hpp"

/// 将 UObject 标记为 GC Root: 设置 UObject+0x08 处的 RF_MarkAsRootSet (0x80) 标志位，
/// 防止 UE4 垃圾回收器回收动态创建的 Widget。
/// 同时将对象加入 GRootedObjects 列表以便统一管理。
void MarkAsGCRoot(UObject* Obj);

/// 清除指定 UObject 的 GC Root 标记，并从 GRootedObjects 列表中移除。
void ClearGCRoot(UObject* Obj);

/// 清除所有已标记的 GC Root (遍历 GRootedObjects 逐个清除标志位)，
/// 在 DLL 卸载或面板销毁时调用，允许引擎正常回收这些对象。
void ClearAllGCRoots();
