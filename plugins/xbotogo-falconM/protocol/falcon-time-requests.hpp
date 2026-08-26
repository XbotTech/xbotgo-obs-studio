#pragma once

#include "falcon-request.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace xbotgo {

/* Value-owning request whose encoding is safe to call from any thread.
 * The IANA time zone ID is truncated to 63 bytes and NUL-padded to the
 * protocol's fixed 64-byte field; encoding has no failure state. */
class RtcClockRequest final : public FalconRequest {
public:
	RtcClockRequest(uint64_t timestamp, int32_t timezone, std::string timezone_id)
		: timestamp_(timestamp),
		  timezone_(timezone),
		  timezone_id_(std::move(timezone_id))
	{
	}

	std::string_view topic() const override { return "ATR"; }
	std::vector<uint8_t> encodePayload() const override
	{
		constexpr size_t kTimestampSize = sizeof(timestamp_);
		constexpr size_t kTimezoneSize = sizeof(timezone_);
		constexpr size_t kTimezoneIdSize = 64;
		std::vector<uint8_t> payload(kTimestampSize + kTimezoneSize + kTimezoneIdSize, 0);

		write_u64_le(timestamp_, payload.data());
		write_u32_le(static_cast<uint32_t>(timezone_), payload.data() + kTimestampSize);
		const size_t timezone_id_size = std::min(timezone_id_.size(), kTimezoneIdSize - 1);
		std::copy_n(timezone_id_.begin(), timezone_id_size,
			    payload.begin() + kTimestampSize + kTimezoneSize);
		return payload;
	}

private:
	static void write_u64_le(uint64_t value, uint8_t *destination)
	{
		for (size_t index = 0; index < sizeof(value); ++index) {
			destination[index] = static_cast<uint8_t>(value >> (index * 8));
		}
	}

	static void write_u32_le(uint32_t value, uint8_t *destination)
	{
		for (size_t index = 0; index < sizeof(value); ++index) {
			destination[index] = static_cast<uint8_t>(value >> (index * 8));
		}
	}

	uint64_t timestamp_;
	int32_t timezone_;
	std::string timezone_id_;
};

} // namespace xbotgo
