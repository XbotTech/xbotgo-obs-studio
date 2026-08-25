#pragma once

#include "falconm-stream.hpp"

#include <obs-module.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace xbotgo {

enum class StreamingResolution : int64_t {
	P1080,
	P1080_60,
	K4,
};

struct falconm_source {
	obs_source_t *source = nullptr;
	std::unique_ptr<FalconMStream> stream;
	std::atomic<bool> stopping{false};
	std::mutex control_mutex;
	std::condition_variable control_cv;
	std::thread control_thread;
	bool worker_stop = false;
	uint64_t request_serial = 0;
	std::string device_id = "Xbt-F-6c092e";
	std::string broker_address = "169.254.184.18";
	uint16_t broker_port = 1883;
	StreamingResolution streaming_resolution = StreamingResolution::K4;
	uint32_t video_ssrc = 0;
	uint32_t audio_ssrc = 0;
};

void falconm_register_proc_handler(falconm_source *source);
bool falconm_query_supported_modes(falconm_source *source, uint8_t max_version);
falconm_supported_modes falconm_get_supported_modes(const falconm_source *source);

} // namespace xbotgo
