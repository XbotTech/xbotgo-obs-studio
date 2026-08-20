#include "falconm-protocol.hpp"

#include <utility>

namespace xbotgo {

static uint16_t read_u16_be(const uint8_t *p)
{
	return uint16_t(uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

std::array<uint8_t, 1> falconm_build_supported_modes_request(uint8_t max_version)
{
	return {max_version};
}

bool falconm_parse_supported_modes(const uint8_t *payload, size_t size, falconm_supported_modes &result)
{
	if (!payload || size < 2) {
		return false;
	}

	falconm_supported_modes parsed;
	parsed.current_mode = read_u16_be(payload);
	for (size_t offset = 2; offset + 2 < size; offset += 3) {
		parsed.modes.push_back({read_u16_be(payload + offset), payload[offset + 2] != 0});
	}
	result = std::move(parsed);
	return true;
}

std::array<uint8_t, 2> falconm_build_capture_mode_request(uint16_t mode)
{
	return {static_cast<uint8_t>(mode >> 8), static_cast<uint8_t>(mode)};
}

bool falconm_parse_capture_mode_result(const uint8_t *payload, size_t size, bool &success)
{
	if (!payload || size != 1 || payload[0] > 1) {
		return false;
	}
	success = payload[0] == 1;
	return true;
}

} // namespace xbotgo
