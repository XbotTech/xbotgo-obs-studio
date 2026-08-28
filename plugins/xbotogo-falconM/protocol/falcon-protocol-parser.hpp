#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xbotgo {

enum class ModeType : uint16_t {
	Team = 0,
	Soccer5v5Over14 = 1,
	Soccer5v5Under14 = 2,
	Soccer7v7Over14 = 3,
	Soccer7v7Under14 = 4,
	BasketballWholeOver14 = 5,
	BasketballWholeUnder14 = 6,
	BasketballHalfOver14 = 7,
	BasketballHalfUnder14 = 8,
	Soccer11v11Over14 = 11,
	Soccer11v11Under14 = 12,
	RugbyWholeOver14 = 13,
	RugbyWholeUnder14 = 14,
	LacrosseWholeOver14 = 15,
	LacrosseWholeUnder14 = 16,
	IceHockeyWholeOver14 = 17,
	IceHockeyWholeUnder14 = 18,
	WheelchairSoccer = 19,
	FollowMe = 20,
	TennisDouble = 23,
	TennisSingle = 24,
	HandballWholeOver14 = 25,
	HandballWholeUnder14 = 26,
	HandballHalfOver14 = 27,
	HandballHalfUnder14 = 28,
	BroomballWholeOver14 = 29,
	BroomballWholeUnder14 = 30,
	PickleballDouble = 31,
	PickleballSingle = 32,
	BadmintonDouble = 33,
	BadmintonSingle = 34,
	BasketballWholeOver14High = 36,
	BasketballWholeUnder14High = 37,
	BasketballHalfOver14High = 38,
	BasketballHalfUnder14High = 39,
	Volleyball = 40,
	KeyPlayerHalf = 41,
	KeyPlayerFull = 42,
	KeyPlayerHalfHigh = 43,
	KeyPlayerFullHigh = 44,
	AmericanFootballCloseHigh = 45,
	AmericanFootballMediumHigh = 46,
	AmericanFootballFarHigh = 47,
	RugbyHigh = 48,
	FlagFootballHigh = 49,
	Baseball = 50,
};

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

inline bool falconm_is_basketball_mode(ModeType mode)
{
	switch (mode) {
	case ModeType::BasketballWholeOver14:
	case ModeType::BasketballWholeUnder14:
	case ModeType::BasketballHalfOver14:
	case ModeType::BasketballHalfUnder14:
	case ModeType::BasketballWholeOver14High:
	case ModeType::BasketballWholeUnder14High:
	case ModeType::BasketballHalfOver14High:
	case ModeType::BasketballHalfUnder14High:
		return true;
	default:
		return false;
	}
}

} // namespace xbotgo
