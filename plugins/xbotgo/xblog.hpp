#pragma once

#include <util/base.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#ifndef XBOTGO_FALCONM_INFO_LOG_ENABLED
#define XBOTGO_FALCONM_INFO_LOG_ENABLED 1
#endif

namespace xbotgo {

inline std::string xblog_timestamp()
{
	const auto now = std::chrono::system_clock::now();
	const auto milliseconds =
		std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
	const std::time_t time = std::chrono::system_clock::to_time_t(now);
	std::tm local_time = {};
	if (!localtime_r(&time, &local_time)) {
		return "time-unavailable";
	}

	char timestamp[24];
	std::snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
		      local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday, local_time.tm_hour,
		      local_time.tm_min, local_time.tm_sec, static_cast<long long>(milliseconds));
	return timestamp;
}

inline const char *xblog_file_name(const char *file)
{
	const char *slash = std::strrchr(file, '/');
	const char *backslash = std::strrchr(file, '\\');
	const char *separator = slash;
	if (!separator || (backslash && backslash > separator)) {
		separator = backslash;
	}
	return separator ? separator + 1 : file;
}

#if !defined(_MSC_VER)
__attribute__((__format__(__printf__, 6, 7)))
#endif
inline void xblog(int log_level, const char *log_type, const char *file, int line, const char *tag,
		  const char *format, ...)
{
	va_list args;
	va_start(args, format);
	va_list args_copy;
	va_copy(args_copy, args);
	const int message_length = std::vsnprintf(nullptr, 0, format, args_copy);
	va_end(args_copy);

	if (message_length < 0) {
		va_end(args);
		(blog)(LOG_ERROR, "%s %s %s:%d [error] failed to format log message", xblog_timestamp().c_str(),
		       tag && *tag ? tag : "FalconM", xblog_file_name(file), line);
		return;
	}

	std::vector<char> message(static_cast<size_t>(message_length) + 1);
	std::vsnprintf(message.data(), message.size(), format, args);
	va_end(args);

	(blog)(log_level, "%s %s %s:%d [%s] %s", xblog_timestamp().c_str(), tag && *tag ? tag : "FalconM",
	       xblog_file_name(file), line, log_type, message.data());
}

} // namespace xbotgo

#define XBLOG_TAG_DEBUG(tag, ...) ::xbotgo::xblog(LOG_DEBUG, "debug", __FILE__, __LINE__, tag, __VA_ARGS__)
#define XBLOG_TAG_WARNING(tag, ...) ::xbotgo::xblog(LOG_WARNING, "warning", __FILE__, __LINE__, tag, __VA_ARGS__)
#define XBLOG_TAG_ERROR(tag, ...) ::xbotgo::xblog(LOG_ERROR, "error", __FILE__, __LINE__, tag, __VA_ARGS__)

#if XBOTGO_FALCONM_INFO_LOG_ENABLED
#define XBLOG_TAG_INFO(tag, ...) ::xbotgo::xblog(LOG_INFO, "info", __FILE__, __LINE__, tag, __VA_ARGS__)
#else
#define XBLOG_TAG_INFO(tag, ...) \
	((void)sizeof((::xbotgo::xblog(LOG_INFO, "info", __FILE__, __LINE__, tag, __VA_ARGS__), 0)))
#endif

#define XBLOG_DEBUG(...) XBLOG_TAG_DEBUG("FalconM", __VA_ARGS__)
#define XBLOG_INFO(...) XBLOG_TAG_INFO("FalconM", __VA_ARGS__)
#define XBLOG_WARNING(...) XBLOG_TAG_WARNING("FalconM", __VA_ARGS__)
#define XBLOG_ERROR(...) XBLOG_TAG_ERROR("FalconM", __VA_ARGS__)
