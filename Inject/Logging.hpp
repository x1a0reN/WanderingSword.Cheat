#pragma once

#include <Windows.h>

void SetupLocalLogging(HMODULE Module);
void ShutdownLocalLogging();
LONG WINAPI UnhandledExceptionLogger(EXCEPTION_POINTERS* ExceptionInfo);
