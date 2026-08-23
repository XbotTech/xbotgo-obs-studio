#pragma once

#include "falcon-event.hpp"
#include "falcon-protocol-parser.hpp"

#include <cstring>
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

inline std::string read_fixed_string(const uint8_t *p, size_t size)
{
	const auto *end = static_cast<const uint8_t *>(std::memchr(p, 0, size));
	return std::string(reinterpret_cast<const char *>(p), end ? static_cast<size_t>(end - p) : size);
}

inline bool parse_capture_parameters(const uint8_t *payload, size_t size, falconm_capture_parameters &parameters)
{
	constexpr size_t kBaseSize = 75;
	constexpr size_t kResolutionStringSize = 64;
	constexpr size_t kResolutionEntrySize = 65;
	constexpr uint8_t kMaxResolutionEntries = 20;
	if (!payload || size < kBaseSize) {
		return false;
	}
	falconm_capture_parameters parsed;
	parsed.mode = read_u16_be(payload);
	parsed.watermark = payload[2] != 0;
	parsed.mute = payload[3] != 0;
	parsed.resolution_id = payload[4];
	parsed.resolution = read_fixed_string(payload + 5, kResolutionStringSize);
	parsed.auto_zoom = payload[69] != 0;
	parsed.auto_tracking = payload[70] != 0;
	parsed.angle_range = read_u16_be(payload + 71);
	parsed.accel_speed = read_u16_be(payload + 73);
	size_t offset = kBaseSize;
	if (size == offset) {
		parameters = std::move(parsed);
		return true;
	}
	if (size < offset + 2) {
		parameters = std::move(parsed);
		return true;
	}
	parsed.has_countdown_time = true;
	parsed.countdown_time = read_u16_be(payload + offset);
	offset += 2;
	if (size == offset) {
		parameters = std::move(parsed);
		return true;
	}
	if (size < offset + 1) {
		parameters = std::move(parsed);
		return true;
	}
	parsed.has_flicker_set = true;
	parsed.flicker_set = payload[offset++];
	if (size == offset) {
		parameters = std::move(parsed);
		return true;
	}
	const uint8_t resolution_count = payload[offset++];
	if (resolution_count > kMaxResolutionEntries) {
		return false;
	}
	if (size != offset + size_t(resolution_count) * kResolutionEntrySize) {
		return false;
	}
	parsed.has_supported_resolutions = true;
	parsed.supported_resolutions.reserve(resolution_count);
	for (uint8_t index = 0; index < resolution_count; ++index) {
		parsed.supported_resolutions.push_back(
			{payload[offset], read_fixed_string(payload + offset + 1, kResolutionStringSize)});
		offset += kResolutionEntrySize;
	}
	parameters = std::move(parsed);
	return true;
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

class CaptureParametersEvent final : public FalconEvent {
public:
	static constexpr std::string_view kTopic = "ANA";
	std::string_view topic() const override { return kTopic; }
	bool parse(const uint8_t *payload, size_t size) override
	{
		return detail::parse_capture_parameters(payload, size, parameters_);
	}
	const falconm_capture_parameters &parameters() const { return parameters_; }

private:
	falconm_capture_parameters parameters_;
};

class DefaultCaptureParametersEvent final : public FalconEvent {
public:
	static constexpr std::string_view kTopic = "AXA";
	std::string_view topic() const override { return kTopic; }
	bool parse(const uint8_t *payload, size_t size) override
	{
		return detail::parse_capture_parameters(payload, size, parameters_);
	}
	const falconm_capture_parameters &parameters() const { return parameters_; }

private:
	falconm_capture_parameters parameters_;
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
