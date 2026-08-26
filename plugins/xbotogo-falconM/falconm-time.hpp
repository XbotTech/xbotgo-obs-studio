#pragma once

#include <cstdint>
#include <string>

namespace xbotgo {

struct falconm_rtc_clock {
	uint64_t timestamp = 0;
	int32_t timezone = 0;
	std::string timezone_id = "UTC";
};

/* Thread-safe. Returns the current Unix time in seconds and the system time
 * zone's offset at that instant. Falls back to UTC when the system zone name
 * cannot be read. */
falconm_rtc_clock falconm_read_system_rtc_clock();

} // namespace xbotgo
