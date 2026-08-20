#pragma once

#include "falcon-request.hpp"
#include "falcon-protocol-parser.hpp"

#include <algorithm>

namespace xbotgo {

class QuerySupportedModesRequest final : public FalconRequest {
public:
	explicit QuerySupportedModesRequest(uint8_t version) : version_(version) {}
	std::string_view topic() const override { return "BPR"; }
	std::vector<uint8_t> encodePayload() const override { return {version_}; }

private:
	uint8_t version_;
};

class SetCaptureModeRequest final : public FalconRequest {
public:
	explicit SetCaptureModeRequest(uint16_t mode) : mode_(mode) {}
	std::string_view topic() const override { return "AVR"; }
	std::vector<uint8_t> encodePayload() const override
	{
		return {static_cast<uint8_t>(mode_ >> 8), static_cast<uint8_t>(mode_)};
	}

private:
	uint16_t mode_;
};

class QueryCaptureParametersRequest final : public FalconRequest {
public:
	std::string_view topic() const override { return "ANR"; }
	std::vector<uint8_t> encodePayload() const override { return {0}; }
};

class QueryDefaultCaptureParametersRequest final : public FalconRequest {
public:
	std::string_view topic() const override { return "AXR"; }
	std::vector<uint8_t> encodePayload() const override { return {0}; }
};

class SetCaptureParametersRequest final : public FalconRequest {
public:
	explicit SetCaptureParametersRequest(falconm_capture_parameters parameters) : parameters_(std::move(parameters))
	{
	}

	std::string_view topic() const override { return "AOR"; }
	std::vector<uint8_t> encodePayload() const override
	{
		constexpr size_t kResolutionStringSize = 64;
		constexpr size_t kResolutionEntrySize = 65;
		constexpr size_t kBaseSize = 80;
		constexpr size_t kMaxResolutionEntries = 20;
		const size_t resolution_count =
			parameters_.has_supported_resolutions
				? std::min(parameters_.supported_resolutions.size(), kMaxResolutionEntries)
				: 0;
		std::vector<uint8_t> payload(kBaseSize + resolution_count * kResolutionEntrySize, 0);
		payload[0] = 0xff;
		payload[1] = 0xff;
		payload[2] = parameters_.watermark ? 1 : 0;
		payload[3] = parameters_.mute ? 1 : 0;
		payload[4] = parameters_.resolution_id;
		write_fixed_string(parameters_.resolution, payload.data() + 5, kResolutionStringSize);
		payload[69] = parameters_.auto_zoom ? 1 : 0;
		payload[70] = parameters_.auto_tracking ? 1 : 0;
		write_u16_be(parameters_.angle_range, payload.data() + 71);
		write_u16_be(parameters_.accel_speed, payload.data() + 73);
		write_u16_be(parameters_.has_countdown_time ? parameters_.countdown_time : 0, payload.data() + 75);
		payload[77] = parameters_.has_flicker_set ? parameters_.flicker_set : 0;
		payload[78] = static_cast<uint8_t>(resolution_count);
		for (size_t index = 0, offset = 79; index < resolution_count; ++index, offset += kResolutionEntrySize) {
			payload[offset] = parameters_.supported_resolutions[index].id;
			write_fixed_string(parameters_.supported_resolutions[index].value, payload.data() + offset + 1,
					   kResolutionStringSize);
		}
		return payload;
	}

private:
	static void write_u16_be(uint16_t value, uint8_t *destination)
	{
		destination[0] = static_cast<uint8_t>(value >> 8);
		destination[1] = static_cast<uint8_t>(value);
	}

	static void write_fixed_string(const std::string &value, uint8_t *destination, size_t capacity)
	{
		const size_t length = std::min(value.size(), capacity - 1);
		std::copy_n(value.begin(), length, destination);
	}

	falconm_capture_parameters parameters_;
};

} // namespace xbotgo
