#pragma once

#include <Windows.h>
#include <sstream>
#include <string>

namespace Logging
{
inline constexpr bool kLoggingEnabled = false;

enum class LogLevel : unsigned char
{
	Trace = 0,
	Debug,
	Info,
	Warn,
	Error,
	Fatal
};

void SetMinLevel(LogLevel Level);
void Write(LogLevel Level, const char* Tag, const std::string& Message);

class LogLine final
{
public:
	LogLine(LogLevel Level, const char* Tag);
	LogLine(const LogLine&) = delete;
	LogLine& operator=(const LogLine&) = delete;
	LogLine(LogLine&& Other) noexcept;
	LogLine& operator=(LogLine&& Other) noexcept;
	~LogLine();

	LogLine& operator<<(const char* Value);
	LogLine& operator<<(const std::string& Value);
	LogLine& operator<<(const wchar_t* Value);
	LogLine& operator<<(const std::wstring& Value);
	LogLine& operator<<(char Value);
	LogLine& operator<<(signed char Value);
	LogLine& operator<<(unsigned char Value);

	LogLine& operator<<(std::ostream& (*Manip)(std::ostream&));
	LogLine& operator<<(std::ios_base& (*Manip)(std::ios_base&));

	template <typename TValue>
	LogLine& operator<<(const TValue& Value)
	{
		if (Active_)
			Buffer_ << Value;
		return *this;
	}

private:
	LogLevel Level_;
	const char* Tag_;
	bool Active_;
	std::ostringstream Buffer_;
};
}

#define LOGT_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Trace, (tag))
#define LOGD_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Debug, (tag))
#define LOGI_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Info, (tag))
#define LOGW_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Warn, (tag))
#define LOGE_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Error, (tag))
#define LOGF_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Fatal, (tag))

void SetupLocalLogging(HMODULE Module);

void ShutdownLocalLogging();

LONG WINAPI UnhandledExceptionLogger(EXCEPTION_POINTERS* ExceptionInfo);
