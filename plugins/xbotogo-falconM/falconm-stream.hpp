#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
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

class FalconMStream {
public:
	using decoded_callback = std::function<void(const obs_source_frame &)>;
	using audio_callback = std::function<void(const obs_source_audio &)>;
	using signaling_callback = std::function<void(const std::string &, const std::vector<uint8_t> &)>;

	virtual ~FalconMStream() = default;
	virtual bool connect(const std::string &device_id, const std::string &broker_address,
			    uint16_t broker_port) = 0;
	virtual bool startStreaming(uint32_t video_ssrc, uint32_t audio_ssrc) = 0;
	virtual bool stopStreaming() = 0;
	virtual bool isStreaming() const = 0;
	virtual void disconnect() = 0;
	virtual void setDecodedFrameCallback(decoded_callback callback) = 0;
	virtual void setAudioCallback(audio_callback callback) = 0;
	virtual void setSignalingCallback(signaling_callback callback) = 0;
	virtual bool sendSignalingMessage(const std::string &topic, const uint8_t *data, size_t size) = 0;
};

std::unique_ptr<FalconMStream> falconm_stream_create();

} // namespace xbotgo
