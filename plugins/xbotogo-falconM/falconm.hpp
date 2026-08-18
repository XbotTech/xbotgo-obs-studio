#pragma once

#include "falconm-stream.hpp"

#include <obs-module.h>

#include <atomic>

namespace xbotgo {
struct falconm_source {
	obs_source_t *source = nullptr;
	std::unique_ptr<FalconMStream> stream;
	std::atomic<bool> stopping{false};
	std::atomic<bool> active{false};
	std::string device_id;
	std::string broker_address;
	uint16_t broker_port = 1883;
	uint32_t video_ssrc = 0;
	uint32_t audio_ssrc = 0;
};

} // namespace xbotgo
