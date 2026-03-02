#pragma once

#include <Windows.h>
#include <sstream>
#include <string>

namespace Logging
{
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
		Buffer_ << Value;
		return *this;
	}

private:
	LogLevel Level_;
	const char* Tag_;
	bool Active_;
	std::ostringstream Buffer_;
};
} // namespace Logging

// 统一日志宏: 所有输出必须通过 Logging.cpp
#define LOGT_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Trace, (tag))
#define LOGD_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Debug, (tag))
#define LOGI_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Info, (tag))
#define LOGW_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Warn, (tag))
#define LOGE_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Error, (tag))
#define LOGF_STREAM(tag) ::Logging::LogLine(::Logging::LogLevel::Fatal, (tag))

/// 初始化日志系统: 打开日志文件 (WanderingSword.Inject.runtime.log)，
/// 后续所有 LOG*_STREAM 会统一写入该文件，并带统一前缀格式。
void SetupLocalLogging(HMODULE Module);

/// 关闭日志系统: 刷新并关闭日志文件。
void ShutdownLocalLogging();

/// 未处理异常过滤器: 记录崩溃信息 (异常代码/指令地址/访问地址) 到日志，
/// 然后交给系统默认处理。注册方式: SetUnhandledExceptionFilter(UnhandledExceptionLogger)。
LONG WINAPI UnhandledExceptionLogger(EXCEPTION_POINTERS* ExceptionInfo);
