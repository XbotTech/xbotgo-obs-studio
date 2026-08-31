#pragma once

#include "falconm-protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <obs.h>

extern "C" {
#include <libavutil/avutil.h>
}

namespace xbotgo {

enum class falconm_codec {
	h264,
	h265,
};

struct falconm_compressed_packet {
	const uint8_t *data = nullptr;
	size_t size = 0;
	int64_t pts = AV_NOPTS_VALUE;
	int64_t dts = AV_NOPTS_VALUE;
	bool keyframe = false;
	falconm_codec codec = falconm_codec::h264;
};

struct falconm_capture_mode_result {
	uint64_t sequence = 0;
	bool success = false;
};

struct falconm_device_state {
	falconm_supported_modes supported_modes;
	uint64_t supported_modes_sequence = 0;
	falconm_capture_mode_result capture_mode_result;
	falconm_capture_parameters capture_parameters;
	uint64_t capture_parameters_sequence = 0;
	falconm_capture_parameters default_capture_parameters;
	uint64_t default_capture_parameters_sequence = 0;
	falconm_motor_angle motor_angle;
	falconm_hall_calibration_status hall_calibration_status = falconm_hall_calibration_status::uncalibrated;
	uint64_t hall_calibration_sequence = 0;
	uint8_t current_zoom = 10;
	uint64_t current_zoom_sequence = 0;
};

struct falconm_video_encoder_options {
	std::optional<int> width = 3840;
	std::optional<int> height = 2160;
	std::optional<int> fps = 30;
	std::optional<int> bitrate = 52 * 1000 * 1000;
};

class FalconMStream {
public:
	using decoded_callback = std::function<void(const obs_source_frame &)>;
	using audio_callback = std::function<void(const obs_source_audio &)>;
	using signaling_callback = std::function<void(const std::string &, const std::vector<uint8_t> &)>;
	using supported_modes_callback = std::function<void(const falconm_supported_modes &)>;
	using motor_angle_report_callback = std::function<void(const falconm_motor_angle &)>;

	virtual ~FalconMStream() = default;
	/* Encoder options are retained until the peer connects, then passed to startStreaming. */
	virtual bool connect(const std::string &device_id, const std::string &broker_address, uint16_t broker_port,
			     const falconm_video_encoder_options &encoder_options) = 0;
	/* Empty encoder options default to 4K/30 at 52 Mbps. Explicit values must be
	 * positive; otherwise streaming fails without calling the SDK. */
	virtual bool startStreaming(const uint32_t video_ssrc, const uint32_t audio_ssrc, const uint32_t data_ssrc,
				    const falconm_video_encoder_options &encoder_options = {}) = 0;
	virtual bool stopStreaming() = 0;
	virtual bool isStreaming() const = 0;
	/* Thread-safe. True while the control connection accepts device commands. */
	virtual bool isConnected() const = 0;
	virtual void disconnect() = 0;
	virtual void setDecodedFrameCallback(decoded_callback callback) = 0;
	virtual void setAudioCallback(audio_callback callback) = 0;
	virtual void setSignalingCallback(signaling_callback callback) = 0;
	virtual void setSupportedModesCallback(supported_modes_callback callback) = 0;
	virtual void setMotorAngleReportCallback(motor_angle_report_callback callback) = 0;
	virtual bool send(const FalconRequest &request) = 0;
	virtual falconm_device_state state() const = 0;
};

std::unique_ptr<FalconMStream> falconm_stream_create();

} // namespace xbotgo
