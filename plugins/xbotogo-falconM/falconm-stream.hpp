#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
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

enum class falconm_direction : uint8_t { up = 0, down = 1, left = 2, right = 3, center = 4 };
enum class falconm_operation : uint8_t { short_press = 0, long_press = 1, release = 2 };

struct falconm_motor_angle {
	int32_t horizontal = 0;
	int32_t vertical = 0;
	uint8_t horizontal_limit = 0;
	uint8_t vertical_limit = 0;
	uint16_t result = 0xffff;
};

class FalconMStream {
public:
	using decoded_callback = std::function<void(const obs_source_frame &)>;
	using audio_callback = std::function<void(const obs_source_audio &)>;
	using signaling_callback = std::function<void(const std::string &, const std::vector<uint8_t> &)>;

	virtual ~FalconMStream() = default;
	virtual bool connect(const std::string &device_id, const std::string &broker_address,
			    uint16_t broker_port) = 0;
	virtual bool startStreaming(const uint32_t video_ssrc , const uint32_t audio_ssrc, const uint32_t dataSsrc) = 0;
	virtual bool stopStreaming() = 0;
	virtual bool isStreaming() const = 0;
	virtual void disconnect() = 0;
	virtual void setDecodedFrameCallback(decoded_callback callback) = 0;
	virtual void setAudioCallback(audio_callback callback) = 0;
	virtual void setSignalingCallback(signaling_callback callback) = 0;
	virtual bool sendSignalingMessage(const std::string &topic, const uint8_t *data, size_t size) = 0;
	virtual bool sendDirection(falconm_direction direction, falconm_operation operation) = 0;
	virtual bool queryMotorAngle() = 0;
	virtual bool setMotorAngleReportEnabled(bool enabled) = 0;
	virtual falconm_motor_angle motorAngle() const = 0;
};

std::unique_ptr<FalconMStream> falconm_stream_create();

} // namespace xbotgo
