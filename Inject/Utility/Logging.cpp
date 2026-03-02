#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include "Logging.hpp"

namespace {
std::ofstream GLocalLogFile;
std::string GLocalLogPath;
std::mutex GLogMutex;
Logging::LogLevel GMinLevel = Logging::LogLevel::Trace;

const char* LevelToString(Logging::LogLevel Level)
{
	switch (Level)
	{
	case Logging::LogLevel::Trace: return "TRACE";
	case Logging::LogLevel::Debug: return "DEBUG";
	case Logging::LogLevel::Info:  return "INFO";
	case Logging::LogLevel::Warn:  return "WARN";
	case Logging::LogLevel::Error: return "ERROR";
	case Logging::LogLevel::Fatal: return "FATAL";
	default: return "INFO";
	}
}

std::string WideToUtf8(const wchar_t* WText)
{
	if (!WText || !WText[0])
		return std::string();

	const int WideLen = static_cast<int>(wcslen(WText));
	const int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, WText, WideLen, nullptr, 0, nullptr, nullptr);
	if (SizeNeeded <= 0)
		return std::string();

	std::string Utf8(static_cast<size_t>(SizeNeeded), '\0');
	WideCharToMultiByte(CP_UTF8, 0, WText, WideLen, Utf8.data(), SizeNeeded, nullptr, nullptr);
	return Utf8;
}

std::wstring Utf8ToWide(const char* Text)
{
	if (!Text || !Text[0])
		return std::wstring();

	const int SrcLen = static_cast<int>(strlen(Text));
	int WideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Text, SrcLen, nullptr, 0);
	if (WideLen <= 0)
	{
		WideLen = MultiByteToWideChar(CP_UTF8, 0, Text, SrcLen, nullptr, 0);
		if (WideLen <= 0)
			return std::wstring();
	}

	std::wstring Wide(static_cast<size_t>(WideLen), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, Text, SrcLen, Wide.data(), WideLen);
	return Wide;
}

std::string BuildTimestamp()
{
	using namespace std::chrono;
	const auto Now = system_clock::now();
	const auto MsPart = duration_cast<milliseconds>(Now.time_since_epoch()) % 1000;
	const std::time_t NowTime = system_clock::to_time_t(Now);

	std::tm LocalTm{};
	localtime_s(&LocalTm, &NowTime);

	std::ostringstream Oss;
	Oss << std::put_time(&LocalTm, "%Y-%m-%d %H:%M:%S")
		<< '.'
		<< std::setw(3) << std::setfill('0') << MsPart.count();
	return Oss.str();
}

void WriteConsole(const std::string& Line)
{
	HANDLE StdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!StdoutHandle || StdoutHandle == INVALID_HANDLE_VALUE)
		return;
	if (GetFileType(StdoutHandle) == FILE_TYPE_UNKNOWN)
		return;

	DWORD ConsoleMode = 0;
	if (GetConsoleMode(StdoutHandle, &ConsoleMode))
	{
		const std::wstring WideLine = Utf8ToWide(Line.c_str());
		if (!WideLine.empty())
		{
			DWORD WrittenChars = 0;
			::WriteConsoleW(
				StdoutHandle,
				WideLine.data(),
				static_cast<DWORD>(WideLine.size()),
				&WrittenChars,
				nullptr);
			return;
		}
	}

	DWORD Written = 0;
	WriteFile(
		StdoutHandle,
		Line.data(),
		static_cast<DWORD>(Line.size()),
		&Written,
		nullptr);
}

std::string NormalizeMessage(std::string Message)
{
	while (!Message.empty() && (Message.back() == '\n' || Message.back() == '\r'))
		Message.pop_back();
	return Message;
}
} // namespace

namespace Logging
{
void SetMinLevel(LogLevel Level)
{
	std::lock_guard<std::mutex> Lock(GLogMutex);
	GMinLevel = Level;
}

void Write(LogLevel Level, const char* Tag, const std::string& Message)
{
	std::lock_guard<std::mutex> Lock(GLogMutex);
	if (static_cast<int>(Level) < static_cast<int>(GMinLevel))
		return;

	const std::string Normalized = NormalizeMessage(Message);
	const char* SafeTag = (Tag && Tag[0]) ? Tag : "General";
	std::ostringstream Line;
	Line << "[Inject]"
		<< "[" << BuildTimestamp() << "]"
		<< "[" << LevelToString(Level) << "]"
		<< "[" << SafeTag << "] "
		<< Normalized
		<< '\n';
	const std::string FinalLine = Line.str();

	if (GLocalLogFile.is_open())
	{
		GLocalLogFile << FinalLine;
		GLocalLogFile.flush();
	}
	OutputDebugStringA(FinalLine.c_str());
	WriteConsole(FinalLine);
}

LogLine::LogLine(LogLevel Level, const char* Tag)
	: Level_(Level), Tag_(Tag), Active_(true), Buffer_()
{
}

LogLine::LogLine(LogLine&& Other) noexcept
	: Level_(Other.Level_), Tag_(Other.Tag_), Active_(Other.Active_), Buffer_(std::move(Other.Buffer_))
{
	Other.Active_ = false;
}

LogLine& LogLine::operator=(LogLine&& Other) noexcept
{
	if (this == &Other)
		return *this;
	Level_ = Other.Level_;
	Tag_ = Other.Tag_;
	Active_ = Other.Active_;
	Buffer_.str(Other.Buffer_.str());
	Buffer_.clear();
	Other.Active_ = false;
	return *this;
}

LogLine::~LogLine()
{
	if (!Active_)
		return;
	Write(Level_, Tag_, Buffer_.str());
}

LogLine& LogLine::operator<<(const char* Value)
{
	if (Value)
		Buffer_ << Value;
	return *this;
}

LogLine& LogLine::operator<<(const std::string& Value)
{
	Buffer_ << Value;
	return *this;
}

LogLine& LogLine::operator<<(const wchar_t* Value)
{
	Buffer_ << WideToUtf8(Value);
	return *this;
}

LogLine& LogLine::operator<<(const std::wstring& Value)
{
	Buffer_ << WideToUtf8(Value.c_str());
	return *this;
}

LogLine& LogLine::operator<<(char Value)
{
	Buffer_ << Value;
	return *this;
}

LogLine& LogLine::operator<<(signed char Value)
{
	Buffer_ << static_cast<int>(Value);
	return *this;
}

LogLine& LogLine::operator<<(unsigned char Value)
{
	Buffer_ << static_cast<unsigned int>(Value);
	return *this;
}

LogLine& LogLine::operator<<(std::ostream& (*Manip)(std::ostream&))
{
	Manip(Buffer_);
	return *this;
}

LogLine& LogLine::operator<<(std::ios_base& (*Manip)(std::ios_base&))
{
	Manip(Buffer_);
	return *this;
}
} // namespace Logging

void SetupLocalLogging(HMODULE Module)
{
	std::lock_guard<std::mutex> Lock(GLogMutex);
	if (GLocalLogFile.is_open())
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
		return;
	GLocalLogFile.setf(std::ios::unitbuf);
}

void ShutdownLocalLogging()
{
	std::lock_guard<std::mutex> Lock(GLogMutex);
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
		Logging::Write(Logging::LogLevel::Fatal, "CRASH", "Unhandled exception with empty ExceptionInfo");
		return EXCEPTION_CONTINUE_SEARCH;
	}

	auto* Rec = ExceptionInfo->ExceptionRecord;
	const uintptr_t CrashIP = reinterpret_cast<uintptr_t>(Rec->ExceptionAddress);
	const uint64_t AccessAddr = (Rec->NumberParameters > 1)
		? static_cast<uint64_t>(Rec->ExceptionInformation[1])
		: 0ULL;

	std::ostringstream Oss;
	Oss << "code=0x" << std::hex << Rec->ExceptionCode
		<< " ip=0x" << CrashIP
		<< " access=0x" << AccessAddr
		<< std::dec
		<< " logPath=" << GLocalLogPath;
	Logging::Write(Logging::LogLevel::Fatal, "CRASH", Oss.str());
	return EXCEPTION_CONTINUE_SEARCH;
}
