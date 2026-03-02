#pragma once

#include <Windows.h>

/// 初始化日志系统: 分配控制台窗口 + 打开日志文件 (WanderingSword.Inject.runtime.log)，
/// 使用 TeeBuf 实现 std::cout 同时输出到控制台和文件。
void SetupLocalLogging(HMODULE Module);

/// 关闭日志系统: 恢复 std::cout 原始缓冲区，关闭日志文件。
void ShutdownLocalLogging();

/// 未处理异常过滤器: 记录崩溃信息 (异常代码/指令地址/访问地址) 到日志，
/// 然后交给系统默认处理。注册方式: SetUnhandledExceptionFilter(UnhandledExceptionLogger)。
LONG WINAPI UnhandledExceptionLogger(EXCEPTION_POINTERS* ExceptionInfo);
