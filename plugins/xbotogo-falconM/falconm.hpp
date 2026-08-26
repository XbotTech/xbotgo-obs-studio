#pragma once

#include "falconm-stream.hpp"

#include <obs-module.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace xbotgo {

enum class StreamingResolution : int64_t {
	P1080,
	P1080_60,
	K4,
};

struct falconm_scene_item_key {
	std::string scene_uuid;
	int64_t item_id = 0;
};

struct falconm_source {
	obs_source_t *source = nullptr;
	std::unique_ptr<FalconMStream> stream;
	std::atomic<bool> stopping{false};
	std::atomic<bool> scene_fit_task_pending{false};
	std::mutex scene_fit_mutex;
	bool scene_fit_all_requested = true;
	std::vector<falconm_scene_item_key> scene_fit_pending_items;
	bool scene_fit_ready = false;
	uint64_t scene_fit_dropped_frames = 0;
	uint64_t scene_fit_total_dropped_frames = 0;
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
	uint32_t last_base_width = 0;
	uint32_t last_base_height = 0;
};

extern obs_source_info falconm_source_info;
void falconm_scene_fitting_init();
void falconm_scene_fitting_shutdown();
void falconm_register_proc_handler(falconm_source *source);
bool falconm_query_supported_modes(falconm_source *source, uint8_t max_version);
falconm_supported_modes falconm_get_supported_modes(const falconm_source *source);

} // namespace xbotgo
