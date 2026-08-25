#pragma once

#include <util/base.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

#ifndef XBOTGO_FALCONM_INFO_LOG_ENABLED
#define XBOTGO_FALCONM_INFO_LOG_ENABLED 1
#endif

namespace xbotgo {

inline std::string falconm_log_timestamp()
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

} // namespace xbotgo

#if XBOTGO_FALCONM_INFO_LOG_ENABLED
#define FALCONM_LOG_INFO(format, ...) \
	blog(LOG_INFO, "[%s] " format, xbotgo::falconm_log_timestamp().c_str(), __VA_ARGS__)
#else
#define FALCONM_LOG_INFO(format, ...) \
	((void)sizeof((blog(LOG_INFO, "[%s] " format, xbotgo::falconm_log_timestamp().c_str(), __VA_ARGS__), 0)))
#endif
