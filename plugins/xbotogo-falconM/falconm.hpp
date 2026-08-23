#pragma once

#include "falconm-stream.hpp"

#include <obs-module.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace xbotgo {
struct falconm_source {
	obs_source_t *source = nullptr;
	std::unique_ptr<FalconMStream> stream;
	std::atomic<bool> stopping{false};
	std::atomic<bool> active{false};
	std::mutex control_mutex;
	std::condition_variable control_cv;
	std::thread control_thread;
	bool worker_stop = false;
	bool requested_active = false;
	uint64_t request_serial = 0;
	std::string device_id = "Xbt-F-6c092e";
	std::string broker_address = "169.254.184.18";
	uint16_t broker_port = 1883;
	uint32_t video_ssrc = 0;
	uint32_t audio_ssrc = 0;
};

} // namespace xbotgo
