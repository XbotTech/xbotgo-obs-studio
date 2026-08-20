#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

namespace xbotgo {

struct falconm_mode_info {
	uint16_t mode = 0;
	bool beta = false;
};

struct falconm_supported_modes {
	uint16_t current_mode = 0;
	std::vector<falconm_mode_info> modes;
};

std::array<uint8_t, 1> falconm_build_supported_modes_request(uint8_t max_version);
bool falconm_parse_supported_modes(const uint8_t *payload, size_t size, falconm_supported_modes &result);
std::array<uint8_t, 2> falconm_build_capture_mode_request(uint16_t mode);
bool falconm_parse_capture_mode_result(const uint8_t *payload, size_t size, bool &success);
inline bool falconm_is_basketball_mode(uint16_t mode)
{
	switch (mode) {
	case 5:
	case 6:
	case 7:
	case 8:
	case 36:
	case 37:
	case 38:
	case 39:
		return true;
	default:
		return false;
	}
}

} // namespace xbotgo
