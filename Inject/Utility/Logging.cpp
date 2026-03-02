#include <iostream>
#include <fstream>
#include <memory>
#include <streambuf>
#include <string>

#include "Logging.hpp"

namespace {
class TeeBuf final : public std::streambuf
{
public:
	TeeBuf(std::streambuf* A, std::streambuf* B) : BufA(A), BufB(B) {}

protected:
	int overflow(int ch) override
	{
		if (ch == EOF)
			return !EOF;

		const int rA = BufA ? BufA->sputc(static_cast<char>(ch)) : ch;
		const int rB = BufB ? BufB->sputc(static_cast<char>(ch)) : ch;
		return (rA == EOF || rB == EOF) ? EOF : ch;
	}

	int sync() override
	{
		const int rA = BufA ? BufA->pubsync() : 0;
		const int rB = BufB ? BufB->pubsync() : 0;
		return (rA == 0 && rB == 0) ? 0 : -1;
	}

private:
	std::streambuf* BufA;
	std::streambuf* BufB;
};

std::ofstream GLocalLogFile;
std::streambuf* GOriginalCoutBuf = nullptr;
std::unique_ptr<TeeBuf> GCoutTeeBuf;
std::string GLocalLogPath;
}
void SetupLocalLogging(HMODULE Module)
{
	if (GCoutTeeBuf)
		return;

	char ModulePath[MAX_PATH] = {};
	if (Module && GetModuleFileNameA(Module, ModulePath, MAX_PATH) > 0)
	{
		std::string DirPath = ModulePath;
		size_t SlashPos = DirPath.find_last_of("\\/");
		if (SlashPos != std::string::npos)
			DirPath.resize(SlashPos + 1);
		else
			DirPath.clear();
		GLocalLogPath = DirPath + "WanderingSword.Inject.runtime.log";
	}
	else
	{
		GLocalLogPath = "WanderingSword.Inject.runtime.log";
	}

	GLocalLogFile.open(GLocalLogPath, std::ios::out | std::ios::app);
	if (!GLocalLogFile.is_open())
	{
		std::cout << "[SDK] Failed to open local log file: " << GLocalLogPath << "\n";
		return;
	}

	GOriginalCoutBuf = std::cout.rdbuf();
	GCoutTeeBuf = std::make_unique<TeeBuf>(GOriginalCoutBuf, GLocalLogFile.rdbuf());
	std::cout.rdbuf(GCoutTeeBuf.get());
	std::cout.setf(std::ios::unitbuf);
	GLocalLogFile.setf(std::ios::unitbuf);

	std::cout << "[SDK] Local log enabled: " << GLocalLogPath << "\n";
}
void ShutdownLocalLogging()
{
	if (GCoutTeeBuf)
	{
		std::cout.flush();
		if (GOriginalCoutBuf)
			std::cout.rdbuf(GOriginalCoutBuf);
		GCoutTeeBuf.reset();
		GOriginalCoutBuf = nullptr;
	}

	if (GLocalLogFile.is_open())
	{
		GLocalLogFile.flush();
		GLocalLogFile.close();
	}
}
LONG WINAPI UnhandledExceptionLogger(EXCEPTION_POINTERS* ExceptionInfo)
{
	if (!ExceptionInfo || !ExceptionInfo->ExceptionRecord)
	{
		std::cout << "[SDK][CRASH] Unhandled exception with empty ExceptionInfo\n";
		return EXCEPTION_CONTINUE_SEARCH;
	}

	auto* Rec = ExceptionInfo->ExceptionRecord;
	const uintptr_t CrashIP = reinterpret_cast<uintptr_t>(Rec->ExceptionAddress);
	const uint64_t AccessAddr = (Rec->NumberParameters > 1)
		? static_cast<uint64_t>(Rec->ExceptionInformation[1])
		: 0ULL;

	std::cout << "[SDK][CRASH] code=0x" << std::hex << Rec->ExceptionCode
	          << " ip=0x" << CrashIP
	          << " access=0x" << AccessAddr
	          << std::dec << "\n";
	std::cout << "[SDK][CRASH] Local log path: " << GLocalLogPath << "\n";
	return EXCEPTION_CONTINUE_SEARCH;
}

