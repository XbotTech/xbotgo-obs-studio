#pragma once

#include "falcon-event.hpp"
#include "falcon-protocol-parser.hpp"

#include <utility>

namespace xbotgo {

namespace detail {

inline uint16_t read_u16_be(const uint8_t *p)
{
	return uint16_t(uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

inline uint32_t read_u32_be(const uint8_t *p)
{
	return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

} // namespace detail

class SupportedModesEvent final : public FalconEvent {
public:
	static constexpr std::string_view kTopic = "BPA";
	std::string_view topic() const override { return kTopic; }
	bool parse(const uint8_t *payload, size_t size) override
	{
		if (!payload || size < 2) {
			return false;
		}
		falconm_supported_modes parsed;
		parsed.current_mode = detail::read_u16_be(payload);
		for (size_t offset = 2; offset + 2 < size; offset += 3) {
			parsed.modes.push_back({detail::read_u16_be(payload + offset), payload[offset + 2] != 0});
		}
		modes_ = std::move(parsed);
		return true;
	}
	const falconm_supported_modes &modes() const { return modes_; }

private:
	falconm_supported_modes modes_;
};

class CaptureModeResultEvent final : public FalconEvent {
public:
	static constexpr std::string_view kTopic = "AVA";
	std::string_view topic() const override { return kTopic; }
	bool parse(const uint8_t *payload, size_t size) override
	{
		if (!payload || size != 1 || payload[0] > 1) {
			return false;
		}
		success_ = payload[0] == 1;
		return true;
	}
	bool success() const { return success_; }

private:
	bool success_ = false;
};

class MotorAngleEvent final : public FalconEvent {
public:
	static constexpr std::string_view kTopic = "BXA";
	std::string_view topic() const override { return kTopic; }
	bool parse(const uint8_t *payload, size_t size) override
	{
		if (!payload || size < 10) {
			return false;
		}
		falconm_motor_angle parsed;
		parsed.result = detail::read_u16_be(payload);
		parsed.horizontal = static_cast<int32_t>(detail::read_u32_be(payload + 2));
		parsed.vertical = static_cast<int32_t>(detail::read_u32_be(payload + 6));
		angle_ = parsed;
		return true;
	}
	const falconm_motor_angle &angle() const { return angle_; }

private:
	falconm_motor_angle angle_;
};

class MotorAngleReportEvent final : public FalconEvent {
public:
	static constexpr std::string_view kTopic = "DFA";
	std::string_view topic() const override { return kTopic; }
	bool parse(const uint8_t *payload, size_t size) override
	{
		if (!payload || size < 12) {
			return false;
		}
		falconm_motor_angle parsed;
		parsed.result = detail::read_u16_be(payload);
		parsed.horizontal = static_cast<int32_t>(detail::read_u32_be(payload + 2));
		parsed.horizontal_limit = payload[6];
		parsed.vertical = static_cast<int32_t>(detail::read_u32_be(payload + 7));
		parsed.vertical_limit = payload[11];
		angle_ = parsed;
		return true;
	}
	const falconm_motor_angle &angle() const { return angle_; }

private:
	falconm_motor_angle angle_;
};

} // namespace xbotgo
