#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
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

struct falconm_motor_angle {
	int32_t horizontal = 0;
	int32_t vertical = 0;
	uint8_t horizontal_limit = 0;
	uint8_t vertical_limit = 0;
	uint16_t result = 0xffff;
};

struct falconm_video_parameter {
	uint8_t id = 0;
	std::string value;
};

struct falconm_capture_parameters {
	uint16_t mode = 0;
	bool watermark = false;
	bool mute = false;
	uint8_t resolution_id = 0;
	std::string resolution;
	bool auto_zoom = false;
	bool auto_tracking = false;
	uint16_t angle_range = 0;
	uint16_t accel_speed = 0;
	bool has_countdown_time = false;
	uint16_t countdown_time = 0;
	bool has_flicker_set = false;
	uint8_t flicker_set = 0;
	bool has_supported_resolutions = false;
	std::vector<falconm_video_parameter> supported_resolutions;
};

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
