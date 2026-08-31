#include "falconm-time.hpp"

#include <CoreFoundation/CoreFoundation.h>

#include <chrono>
#include <vector>

namespace xbotgo {

static std::string cf_string_to_utf8(CFStringRef value)
{
	if (!value) {
		return {};
	}
	const CFIndex maximum_size =
		CFStringGetMaximumSizeForEncoding(CFStringGetLength(value), kCFStringEncodingUTF8);
	if (maximum_size < 0) {
		return {};
	}
	std::vector<char> buffer(static_cast<size_t>(maximum_size) + 1, '\0');
	if (!CFStringGetCString(value, buffer.data(), static_cast<CFIndex>(buffer.size()), kCFStringEncodingUTF8)) {
		return {};
	}
	return buffer.data();
}

falconm_rtc_clock falconm_read_system_rtc_clock()
{
	falconm_rtc_clock result;
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch());
	result.timestamp = static_cast<uint64_t>(seconds.count());

	CFTimeZoneRef timezone = CFTimeZoneCopySystem();
	if (!timezone) {
		return result;
	}
	const std::string timezone_id = cf_string_to_utf8(CFTimeZoneGetName(timezone));
	if (!timezone_id.empty()) {
		result.timezone = static_cast<int32_t>(CFTimeZoneGetSecondsFromGMT(timezone, CFAbsoluteTimeGetCurrent()));
		result.timezone_id = timezone_id;
	}
	CFRelease(timezone);
	return result;
}

} // namespace xbotgo
