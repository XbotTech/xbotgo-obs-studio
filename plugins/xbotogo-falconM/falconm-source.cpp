#include "falconm.hpp"
#include "falconm-log.hpp"

#ifdef XBOTGO_DEVICE_DISCOVERY
#include <XBotGoDeviceSearchDialog.hpp>
#include <QApplication>
#endif

#include <util/base.h>

#include <limits>
#include <pthread.h>
#include <cstring>
#include <callback/calldata.h>

namespace xbotgo {

static constexpr uint16_t DEFAULT_MQTT_PORT = 1883;
static constexpr char STREAMING_RESOLUTION_SETTING[] = "streaming_resolution";

static StreamingResolution get_streaming_resolution(obs_data_t *settings)
{
	const auto resolution =
		static_cast<StreamingResolution>(obs_data_get_int(settings, STREAMING_RESOLUTION_SETTING));
	switch (resolution) {
	case StreamingResolution::P1080:
	case StreamingResolution::P1080_60:
	case StreamingResolution::K4:
		return resolution;
	}

	blog(LOG_WARNING, "FalconM: invalid streaming resolution=%lld; falling back to 1080p/30",
	     obs_data_get_int(settings, STREAMING_RESOLUTION_SETTING));
	return StreamingResolution::P1080;
}

static falconm_video_encoder_options get_encoder_options(StreamingResolution resolution)
{
	switch (resolution) {
	case StreamingResolution::P1080:
		return {1920, 1080, 30, 10 * 1000 * 1000};
	case StreamingResolution::P1080_60:
		return {1920, 1080, 60, 10 * 1000 * 1000};
	case StreamingResolution::K4:
		return {3840, 2160, 30, 52 * 1000 * 1000};
	}

	return {1920, 1080, 30, 10 * 1000 * 1000};
}

static void log_source_callback_thread(const char *callback_name)
{
	uint64_t thread_id = 0;
	const bool is_main_thread = pthread_main_np() != 0;
	if (pthread_threadid_np(nullptr, &thread_id) != 0) {
		blog(LOG_WARNING, "FalconM: callback=%s thread_id=unavailable is_main_thread=%s", callback_name,
		     is_main_thread ? "true" : "false");
		return;
	}

	FALCONM_LOG_INFO("FalconM: callback=%s thread_id=%llu is_main_thread=%s", callback_name,
			 static_cast<unsigned long long>(thread_id), is_main_thread ? "true" : "false");
}

static uint16_t get_broker_port(obs_data_t *settings)
{
	const long long port = obs_data_get_int(settings, "broker_port");
	return port > 0 && port <= std::numeric_limits<uint16_t>::max() ? static_cast<uint16_t>(port)
									: DEFAULT_MQTT_PORT;
}

static const char *falconm_get_name(void *)
{
	log_source_callback_thread("get_name");
	return obs_module_text("XBotGoFalconM");
}

static void output_video(falconm_source *d, const obs_source_frame &in)
{
	if (d->stopping || !in.width || !in.height) {
		return;
	}
	obs_source_output_video(d->source, &in);
}

static void output_audio(falconm_source *d, const obs_source_audio &in)
{
	if (d->stopping || !in.frames) {
		return;
	}
	obs_source_output_audio(d->source, &in);
}

static void falconm_control_worker(falconm_source *d)
{
	uint64_t handled_serial = 0;
	bool session_active = false;

	for (;;) {
		std::string broker_address;
		std::string device_id;
		uint16_t broker_port = DEFAULT_MQTT_PORT;
		StreamingResolution streaming_resolution = StreamingResolution::P1080;
		uint64_t request_serial = 0;

		{
			std::unique_lock<std::mutex> lock(d->control_mutex);
			d->control_cv.wait(lock, [d, handled_serial] {
				return d->worker_stop || d->request_serial != handled_serial;
			});

			if (d->worker_stop) {
				break;
			}

			broker_address = d->broker_address;
			device_id = d->device_id;
			broker_port = d->broker_port;
			streaming_resolution = d->streaming_resolution;
			request_serial = d->request_serial;
			handled_serial = request_serial;
		}

		if (session_active) {
			d->stream->disconnect();
			session_active = false;
		}

		/* A newer request may have arrived while the SDK was disconnecting.
		 * Skip this stale connection attempt and process the newest request. */
		{
			std::lock_guard<std::mutex> lock(d->control_mutex);
			if (d->worker_stop) {
				break;
			}
			if (d->request_serial != request_serial) {
				continue;
			}
		}

		d->stopping = false;
		if (!d->stream->connect(device_id, broker_address, broker_port,
					get_encoder_options(streaming_resolution))) {
			blog(LOG_ERROR, "FalconM: asynchronous connect failed for device '%s'", device_id.c_str());
			d->stopping = true;
			continue;
		}
		session_active = true;
	}

	d->stopping = true;
	if (session_active) {
		d->stream->disconnect();
	}
}

static void falconm_request_reconnect(falconm_source *d)
{
	{
		std::lock_guard<std::mutex> lock(d->control_mutex);
		++d->request_serial;
	}
	d->control_cv.notify_one();
}

static void *falconm_create(obs_data_t *s, obs_source_t *source)
{
	log_source_callback_thread("create");
	FALCONM_LOG_INFO(
		"FalconM: falconm_create settings=%p source=%p broker_address='%s' device_id='%s' broker_port=%lld",
		(void *)s, (void *)source, obs_data_get_string(s, "broker_address"), obs_data_get_string(s, "device_id"),
		obs_data_get_int(s, "broker_port"));
	auto *d = new falconm_source;
	d->source = source;
	d->stream = falconm_stream_create();
	d->broker_address = obs_data_get_string(s, "broker_address");
	d->device_id = obs_data_get_string(s, "device_id");
	d->broker_port = get_broker_port(s);
	d->streaming_resolution = get_streaming_resolution(s);
	d->stream->setDecodedFrameCallback([d](const obs_source_frame &f) {
		output_video(d, f);
	});
	d->stream->setAudioCallback([d](const obs_source_audio &f) {
		output_audio(d, f);
	});
	d->control_thread = std::thread(falconm_control_worker, d);
	falconm_request_reconnect(d);
	FALCONM_LOG_INFO("FalconM: falconm_create result=%p broker_address='%s' device_id='%s' broker_port=%u",
			 (void *)d, d->broker_address.c_str(), d->device_id.c_str(), d->broker_port);
	falconm_register_proc_handler(d);
	return d;
}
static void falconm_destroy(void *p)
{
	log_source_callback_thread("destroy");
	auto *d = (falconm_source *)p;
	d->stopping = true;
	obs_source_output_video(d->source, nullptr);
	{
		std::lock_guard<std::mutex> lock(d->control_mutex);
		d->worker_stop = true;
		++d->request_serial;
	}
	d->control_cv.notify_one();
	if (d->control_thread.joinable()) {
		d->control_thread.join();
	}
	d->stream->setDecodedFrameCallback({});
	d->stream->setAudioCallback({});
	d->stream->setSignalingCallback({});
	delete d;
}
static void falconm_update(void *p, obs_data_t *s)
{
	log_source_callback_thread("update");
	auto *d = (falconm_source *)p;
	bool notify_worker = false;
	{
		std::lock_guard<std::mutex> lock(d->control_mutex);
		const std::string broker_address = obs_data_get_string(s, "broker_address");
		const std::string device_id = obs_data_get_string(s, "device_id");
		const uint16_t broker_port = get_broker_port(s);
		const StreamingResolution streaming_resolution = get_streaming_resolution(s);
		const bool connection_changed = d->broker_address != broker_address || d->device_id != device_id ||
					 d->broker_port != broker_port || d->streaming_resolution != streaming_resolution;

		FALCONM_LOG_INFO(
			"FalconM: falconm_update source_data=%p settings=%p settings.broker_address='%s' "
			"settings.device_id='%s' settings.broker_port=%lld current.broker_address='%s' "
			"current.device_id='%s' current.broker_port=%u settings.streaming_resolution=%lld "
			"current.streaming_resolution=%lld",
			p, (void *)s, broker_address.c_str(), device_id.c_str(), obs_data_get_int(s, "broker_port"),
			d->broker_address.c_str(), d->device_id.c_str(), d->broker_port,
			obs_data_get_int(s, STREAMING_RESOLUTION_SETTING), static_cast<long long>(d->streaming_resolution));

		d->broker_address = broker_address;
		d->device_id = device_id;
		d->broker_port = broker_port;
		d->streaming_resolution = streaming_resolution;
		if (connection_changed) {
			d->stopping = true;
			++d->request_serial;
			notify_worker = true;
		}
	}
	if (notify_worker) {
		d->control_cv.notify_one();
	}
}

static void falconm_send_direction(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long direction = -1, operation = -1;
	if (!calldata_get_int(cd, "direction", &direction) || !calldata_get_int(cd, "operation", &operation) ||
	    direction < 0 || direction > 4 || operation < 0 || operation > 2 || !d->stream) {
		return;
	}
	calldata_set_bool(cd, "success",
			  d->stream->send(SendDirectionRequest{static_cast<falconm_direction>(direction),
							       static_cast<falconm_operation>(operation)}));
}

static void falconm_get_connection_state(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	calldata_set_bool(cd, "connected", d && d->stream && d->stream->isConnected());
}

static void falconm_query_angle(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	calldata_set_bool(cd, "success", d->stream && d->stream->send(QueryMotorAngleRequest{}));
}

static void falconm_set_angle_reporting(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	bool enabled = false;
	if (!calldata_get_bool(cd, "enabled", &enabled) || !d->stream) {
		return;
	}
	calldata_set_bool(cd, "success", d->stream->send(SetMotorAngleReportingRequest{enabled}));
}

static void falconm_set_buzzer_mode(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long mode = -1;
	if (!calldata_get_int(cd, "mode", &mode) || mode < 0 || mode > 5 || !d->stream) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	calldata_set_bool(cd, "success", d->stream->send(SetBuzzerModeRequest{static_cast<uint8_t>(mode)}));
}

static void falconm_query_hall_calibration(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	calldata_set_bool(cd, "success", d->stream && d->stream->send(QueryHallCalibrationRequest{}));
}

static void falconm_start_hall_calibration(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	calldata_set_bool(cd, "success", d->stream && d->stream->send(StartHallCalibrationRequest{}));
}

static void falconm_get_hall_calibration(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d->stream) {
		return;
	}
	const auto state = d->stream->state();
	calldata_set_int(cd, "sequence", static_cast<long long>(state.hall_calibration_sequence));
	calldata_set_int(cd, "status", static_cast<long long>(state.hall_calibration_status));
}

static void falconm_send_manual_zoom(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long type = -1, value = 0;
	if (!d->stream || !calldata_get_int(cd, "type", &type) || !calldata_get_int(cd, "value", &value) ||
	    (type == static_cast<long long>(falconm_zoom_type::relative) && value != -1 && value != 1) ||
	    (type == static_cast<long long>(falconm_zoom_type::absolute) && (value < 1 || value > 3)) ||
	    (type != static_cast<long long>(falconm_zoom_type::relative) &&
	     type != static_cast<long long>(falconm_zoom_type::absolute))) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	calldata_set_bool(
		cd, "success",
		d->stream->send(ManualZoomRequest{static_cast<falconm_zoom_type>(type), static_cast<int8_t>(value)}));
}

static void falconm_query_current_zoom(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	calldata_set_bool(cd, "success", d->stream && d->stream->send(QueryCurrentZoomRequest{}));
}

static void falconm_get_current_zoom(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d->stream) {
		return;
	}
	const auto state = d->stream->state();
	calldata_set_int(cd, "sequence", static_cast<long long>(state.current_zoom_sequence));
	calldata_set_int(cd, "value", state.current_zoom);
}

static void falconm_get_angle(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d->stream) {
		return;
	}
	const auto angle = d->stream->state().motor_angle;
	calldata_set_int(cd, "result", angle.result);
	calldata_set_float(cd, "horizontal", angle.horizontal / 100.0);
	calldata_set_float(cd, "vertical", angle.vertical / 100.0);
	calldata_set_int(cd, "horizontal_limit", angle.horizontal_limit);
	calldata_set_int(cd, "vertical_limit", angle.vertical_limit);
}

static void falconm_query_supported_modes_proc(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long version = 0;
	if (!calldata_get_int(cd, "version", &version) || version < 0 || version > UINT8_MAX || !d->stream) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	calldata_set_bool(cd, "success", d->stream->send(QuerySupportedModesRequest{static_cast<uint8_t>(version)}));
}

static void falconm_get_supported_modes(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d->stream) {
		return;
	}
	const auto state = d->stream->state();
	const auto &modes = state.supported_modes;
	calldata_set_int(cd, "sequence", static_cast<long long>(state.supported_modes_sequence));
	calldata_set_int(cd, "current_mode", modes.current_mode);
	calldata_set_int(cd, "count", static_cast<long long>(modes.modes.size()));
}

static void falconm_get_supported_mode(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long index = -1;
	if (!calldata_get_int(cd, "index", &index) || index < 0 || !d->stream) {
		return;
	}
	const auto modes = d->stream->state().supported_modes;
	if (static_cast<size_t>(index) >= modes.modes.size()) {
		return;
	}
	const auto &mode = modes.modes[static_cast<size_t>(index)];
	calldata_set_int(cd, "mode", mode.mode);
	calldata_set_bool(cd, "beta", mode.beta);
}

static void falconm_set_capture_mode(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long mode = -1;
	if (!calldata_get_int(cd, "mode", &mode) || mode < 0 || mode > UINT16_MAX || !d->stream) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	calldata_set_bool(cd, "success", d->stream->send(SetCaptureModeRequest{static_cast<uint16_t>(mode)}));
}

static void falconm_get_capture_mode_result(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d->stream) {
		return;
	}
	const auto result = d->stream->state().capture_mode_result;
	calldata_set_int(cd, "sequence", static_cast<long long>(result.sequence));
	calldata_set_bool(cd, "success", result.success);
}

static void falconm_query_capture_parameters(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	calldata_set_bool(cd, "success", d->stream && d->stream->send(QueryCaptureParametersRequest{}));
}

static void falconm_query_default_capture_parameters(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	calldata_set_bool(cd, "success", d->stream && d->stream->send(QueryDefaultCaptureParametersRequest{}));
}

template<typename Update>
static bool falconm_update_capture_parameters(falconm_source *source, Update &&update)
{
	if (!source || !source->stream) {
		return false;
	}
	auto parameters = source->stream->state().capture_parameters;
	update(parameters);
	return source->stream->send(SetCaptureParametersRequest{std::move(parameters)});
}

static void falconm_set_capture_parameters(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long resolution_id = -1, angle_range = -1, accel_speed = -1, countdown = -1, flicker = -1;
	bool watermark = false, mute = false, auto_zoom = false, auto_tracking = false;
	const char *resolution = nullptr;
	if (!d->stream || !calldata_get_bool(cd, "watermark", &watermark) || !calldata_get_bool(cd, "mute", &mute) ||
	    !calldata_get_int(cd, "resolution_id", &resolution_id) ||
	    !calldata_get_string(cd, "resolution", &resolution) || !resolution ||
	    !calldata_get_bool(cd, "auto_zoom", &auto_zoom) ||
	    !calldata_get_bool(cd, "auto_tracking", &auto_tracking) ||
	    !calldata_get_int(cd, "angle_range", &angle_range) || !calldata_get_int(cd, "accel_speed", &accel_speed) ||
	    !calldata_get_int(cd, "countdown", &countdown) || !calldata_get_int(cd, "flicker", &flicker) ||
	    resolution_id < 0 || resolution_id > UINT8_MAX || angle_range < 0 || angle_range > UINT16_MAX ||
	    accel_speed < 0 || accel_speed > UINT16_MAX || countdown < 0 || countdown > UINT16_MAX || flicker < 0 ||
	    flicker > 2 || std::strlen(resolution) > 63) {
		calldata_set_bool(cd, "success", false);
		return;
	}

	const bool success = falconm_update_capture_parameters(d, [&](falconm_capture_parameters &parameters) {
		parameters.watermark = watermark;
		parameters.mute = mute;
		parameters.resolution_id = static_cast<uint8_t>(resolution_id);
		parameters.resolution = resolution;
		parameters.auto_zoom = auto_zoom;
		parameters.auto_tracking = auto_tracking;
		parameters.angle_range = static_cast<uint16_t>(angle_range);
		parameters.accel_speed = static_cast<uint16_t>(accel_speed);
		parameters.has_countdown_time = true;
		parameters.countdown_time = static_cast<uint16_t>(countdown);
		parameters.has_flicker_set = true;
		parameters.flicker_set = static_cast<uint8_t>(flicker);
	});
	calldata_set_bool(cd, "success", success);
}

static void falconm_set_capture_auto_tracking(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	bool auto_tracking = false;
	if (!d->stream || !calldata_get_bool(cd, "auto_tracking", &auto_tracking)) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	const bool success = falconm_update_capture_parameters(
		d, [auto_tracking](falconm_capture_parameters &parameters) {
			parameters.auto_tracking = auto_tracking;
		});
	calldata_set_bool(cd, "success", success);
}

static void falconm_set_capture_auto_zoom(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	bool auto_zoom = false;
	if (!d->stream || !calldata_get_bool(cd, "auto_zoom", &auto_zoom)) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	const bool success = falconm_update_capture_parameters(
		d, [auto_zoom](falconm_capture_parameters &parameters) {
			parameters.auto_zoom = auto_zoom;
		});
	calldata_set_bool(cd, "success", success);
}

static void falconm_set_capture_angle_range(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long angle_range = -1;
	if (!d->stream || !calldata_get_int(cd, "angle_range", &angle_range) || angle_range < 60 ||
	    angle_range > 150) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	const bool success = falconm_update_capture_parameters(
		d, [angle_range](falconm_capture_parameters &parameters) {
			parameters.angle_range = static_cast<uint16_t>(angle_range);
		});
	calldata_set_bool(cd, "success", success);
}

static void falconm_set_capture_tracking_and_angle_range(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	bool auto_tracking = false;
	long long angle_range = -1;
	if (!d->stream || !calldata_get_bool(cd, "auto_tracking", &auto_tracking) ||
	    !calldata_get_int(cd, "angle_range", &angle_range) || angle_range < 60 || angle_range > 150) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	const bool success = falconm_update_capture_parameters(
		d, [auto_tracking, angle_range](falconm_capture_parameters &parameters) {
			parameters.auto_tracking = auto_tracking;
			parameters.angle_range = static_cast<uint16_t>(angle_range);
		});
	calldata_set_bool(cd, "success", success);
}

static void falconm_set_capture_zoom_tracking_and_angle_range(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	bool auto_zoom = false, auto_tracking = false;
	long long angle_range = -1;
	if (!d->stream || !calldata_get_bool(cd, "auto_zoom", &auto_zoom) ||
	    !calldata_get_bool(cd, "auto_tracking", &auto_tracking) ||
	    !calldata_get_int(cd, "angle_range", &angle_range) || angle_range < 60 || angle_range > 150) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	const bool success = falconm_update_capture_parameters(
		d, [auto_zoom, auto_tracking, angle_range](falconm_capture_parameters &parameters) {
			parameters.auto_zoom = auto_zoom;
			parameters.auto_tracking = auto_tracking;
			parameters.angle_range = static_cast<uint16_t>(angle_range);
		});
	calldata_set_bool(cd, "success", success);
}

static void falconm_get_capture_parameters(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d->stream) {
		return;
	}
	const auto state = d->stream->state();
	const auto &parameters = state.capture_parameters;
	calldata_set_int(cd, "sequence", static_cast<long long>(state.capture_parameters_sequence));
	calldata_set_int(cd, "mode", parameters.mode);
	calldata_set_bool(cd, "watermark", parameters.watermark);
	calldata_set_bool(cd, "mute", parameters.mute);
	calldata_set_int(cd, "resolution_id", parameters.resolution_id);
	calldata_set_string(cd, "resolution", parameters.resolution.c_str());
	calldata_set_bool(cd, "auto_zoom", parameters.auto_zoom);
	calldata_set_bool(cd, "auto_tracking", parameters.auto_tracking);
	calldata_set_int(cd, "angle_range", parameters.angle_range);
	calldata_set_int(cd, "accel_speed", parameters.accel_speed);
	calldata_set_bool(cd, "has_countdown", parameters.has_countdown_time);
	calldata_set_int(cd, "countdown", parameters.countdown_time);
	calldata_set_bool(cd, "has_flicker", parameters.has_flicker_set);
	calldata_set_int(cd, "flicker", parameters.flicker_set);
	calldata_set_int(cd, "supported_resolution_count",
			 static_cast<long long>(parameters.supported_resolutions.size()));
}

static void falconm_get_capture_supported_resolution(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long index = -1;
	if (!d->stream || !calldata_get_int(cd, "index", &index) || index < 0) {
		return;
	}
	const auto parameters = d->stream->state().capture_parameters;
	if (static_cast<size_t>(index) >= parameters.supported_resolutions.size()) {
		return;
	}
	const auto &resolution = parameters.supported_resolutions[static_cast<size_t>(index)];
	calldata_set_int(cd, "resolution_id", resolution.id);
	calldata_set_string(cd, "resolution", resolution.value.c_str());
}

static void falconm_get_default_capture_parameters(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d->stream) {
		return;
	}
	const auto state = d->stream->state();
	const auto &parameters = state.default_capture_parameters;
	calldata_set_int(cd, "sequence", static_cast<long long>(state.default_capture_parameters_sequence));
	calldata_set_int(cd, "mode", parameters.mode);
	calldata_set_bool(cd, "watermark", parameters.watermark);
	calldata_set_bool(cd, "mute", parameters.mute);
	calldata_set_int(cd, "resolution_id", parameters.resolution_id);
	calldata_set_string(cd, "resolution", parameters.resolution.c_str());
	calldata_set_bool(cd, "auto_zoom", parameters.auto_zoom);
	calldata_set_bool(cd, "auto_tracking", parameters.auto_tracking);
	calldata_set_int(cd, "angle_range", parameters.angle_range);
	calldata_set_int(cd, "accel_speed", parameters.accel_speed);
	calldata_set_bool(cd, "has_countdown", parameters.has_countdown_time);
	calldata_set_int(cd, "countdown", parameters.countdown_time);
	calldata_set_bool(cd, "has_flicker", parameters.has_flicker_set);
	calldata_set_int(cd, "flicker", parameters.flicker_set);
	calldata_set_int(cd, "supported_resolution_count",
			 static_cast<long long>(parameters.supported_resolutions.size()));
}

static void falconm_get_default_capture_supported_resolution(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	long long index = -1;
	if (!d->stream || !calldata_get_int(cd, "index", &index) || index < 0) {
		return;
	}
	const auto parameters = d->stream->state().default_capture_parameters;
	if (static_cast<size_t>(index) >= parameters.supported_resolutions.size()) {
		return;
	}
	const auto &resolution = parameters.supported_resolutions[static_cast<size_t>(index)];
	calldata_set_int(cd, "resolution_id", resolution.id);
	calldata_set_string(cd, "resolution", resolution.value.c_str());
}

void falconm_register_proc_handler(falconm_source *d)
{
	proc_handler_t *ph = obs_source_get_proc_handler(d->source);
	proc_handler_add(ph, "void get_connection_state(out bool connected)", falconm_get_connection_state, d);
	proc_handler_add(ph, "void send_direction(int direction, int operation, out bool success)",
			 falconm_send_direction, d);
	proc_handler_add(ph, "void query_motor_angle(out bool success)", falconm_query_angle, d);
	proc_handler_add(ph, "void set_motor_angle_reporting(bool enabled, out bool success)",
			 falconm_set_angle_reporting, d);
	proc_handler_add(ph, "void set_buzzer_mode(int mode, out bool success)", falconm_set_buzzer_mode, d);
	proc_handler_add(ph, "void query_hall_calibration(out bool success)", falconm_query_hall_calibration, d);
	proc_handler_add(ph, "void start_hall_calibration(out bool success)", falconm_start_hall_calibration, d);
	proc_handler_add(ph, "void get_hall_calibration(out int sequence, out int status)",
			 falconm_get_hall_calibration, d);
	proc_handler_add(ph, "void send_manual_zoom(int type, int value, out bool success)", falconm_send_manual_zoom,
			 d);
	proc_handler_add(ph, "void query_current_zoom(out bool success)", falconm_query_current_zoom, d);
	proc_handler_add(ph, "void get_current_zoom(out int sequence, out int value)", falconm_get_current_zoom, d);
	proc_handler_add(
		ph,
		"void get_motor_angle(out int result, out float horizontal, out float vertical, out int horizontal_limit, out int vertical_limit)",
		falconm_get_angle, d);
	proc_handler_add(ph, "void query_supported_modes(int version, out bool success)",
			 falconm_query_supported_modes_proc, d);
	proc_handler_add(ph, "void get_supported_modes(out int sequence, out int current_mode, out int count)",
			 falconm_get_supported_modes, d);
	proc_handler_add(ph, "void get_supported_mode(int index, out int mode, out bool beta)",
			 falconm_get_supported_mode, d);
	proc_handler_add(ph, "void set_capture_mode(int mode, out bool success)", falconm_set_capture_mode, d);
	proc_handler_add(ph, "void get_capture_mode_result(out int sequence, out bool success)",
			 falconm_get_capture_mode_result, d);
	proc_handler_add(ph, "void query_capture_parameters(out bool success)", falconm_query_capture_parameters, d);
	proc_handler_add(
		ph,
		"void set_capture_parameters(bool watermark, bool mute, int resolution_id, string resolution, bool auto_zoom, bool auto_tracking, int angle_range, int accel_speed, int countdown, int flicker, out bool success)",
		falconm_set_capture_parameters, d);
	proc_handler_add(ph, "void set_capture_auto_tracking(bool auto_tracking, out bool success)",
			 falconm_set_capture_auto_tracking, d);
	proc_handler_add(ph, "void set_capture_auto_zoom(bool auto_zoom, out bool success)",
			 falconm_set_capture_auto_zoom, d);
	proc_handler_add(ph, "void set_capture_angle_range(int angle_range, out bool success)",
			 falconm_set_capture_angle_range, d);
	proc_handler_add(
		ph, "void set_capture_tracking_and_angle_range(bool auto_tracking, int angle_range, out bool success)",
		falconm_set_capture_tracking_and_angle_range, d);
	proc_handler_add(
		ph,
		"void set_capture_zoom_tracking_and_angle_range(bool auto_zoom, bool auto_tracking, int angle_range, out bool success)",
		falconm_set_capture_zoom_tracking_and_angle_range, d);
	proc_handler_add(
		ph,
		"void get_capture_parameters(out int sequence, out int mode, out bool watermark, out bool mute, out int resolution_id, out string resolution, out bool auto_zoom, out bool auto_tracking, out int angle_range, out int accel_speed, out bool has_countdown, out int countdown, out bool has_flicker, out int flicker, out int supported_resolution_count)",
		falconm_get_capture_parameters, d);
	proc_handler_add(
		ph, "void get_capture_supported_resolution(int index, out int resolution_id, out string resolution)",
		falconm_get_capture_supported_resolution, d);
	proc_handler_add(ph, "void query_default_capture_parameters(out bool success)",
			 falconm_query_default_capture_parameters, d);
	proc_handler_add(
		ph,
		"void get_default_capture_parameters(out int sequence, out int mode, out bool watermark, out bool mute, out int resolution_id, out string resolution, out bool auto_zoom, out bool auto_tracking, out int angle_range, out int accel_speed, out bool has_countdown, out int countdown, out bool has_flicker, out int flicker, out int supported_resolution_count)",
		falconm_get_default_capture_parameters, d);
	proc_handler_add(
		ph,
		"void get_default_capture_supported_resolution(int index, out int resolution_id, out string resolution)",
		falconm_get_default_capture_supported_resolution, d);
}

bool falconm_query_supported_modes(falconm_source *source, uint8_t max_version)
{
	return source && source->stream && source->stream->send(QuerySupportedModesRequest{max_version});
}

falconm_supported_modes falconm_get_supported_modes(const falconm_source *source)
{
	return source && source->stream ? source->stream->state().supported_modes : falconm_supported_modes{};
}

#ifdef XBOTGO_DEVICE_DISCOVERY
static bool falconm_search_device(obs_properties_t *, obs_property_t *, void *data)
{
	auto *d = static_cast<falconm_source *>(data);
	if (!d || !d->source) {
		return false;
	}

	XBotGo::DeviceSearchDialog dialog(QApplication::activeWindow(), XBotGo::DeviceSearchDialog::Mode::Select);
	if (dialog.exec() != QDialog::Accepted) {
		return false;
	}

	const std::optional<XBotGo::Device> device = dialog.selectedDevice();
	if (!device) {
		return false;
	}

	obs_data_t *settings = obs_source_get_settings(d->source);
	obs_data_set_string(settings, "broker_address", device->ip.toUtf8().constData());
	obs_data_set_string(settings, "device_id", device->id.toUtf8().constData());
	obs_data_set_int(settings, "broker_port", device->mqttPort);
	obs_data_set_string(settings, "device_version",
			   device->version.isEmpty() ? obs_module_text("Unknown") : device->version.toUtf8().constData());
	obs_data_set_string(settings, "device_serial_number",
			   device->serialNumber.isEmpty() ? obs_module_text("Unknown")
								 : device->serialNumber.toUtf8().constData());
	obs_source_update(d->source, settings);
	obs_data_release(settings);
	return true;
}
#endif

static obs_properties_t *falconm_properties(void *data)
{
	log_source_callback_thread("get_properties");
	auto *p = obs_properties_create();
#ifdef XBOTGO_DEVICE_DISCOVERY
	obs_properties_add_button2(p, "search_device", obs_module_text("SearchDevices"), falconm_search_device, data);
#else
	UNUSED_PARAMETER(data);
#endif
	obs_properties_add_text(p, "broker_address", obs_module_text("BrokerAddress"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(p, "device_id", obs_module_text("DeviceId"), OBS_TEXT_DEFAULT);
	obs_properties_add_int(p, "broker_port", obs_module_text("MqttPort"), 1, std::numeric_limits<uint16_t>::max(),
			       1);
	obs_property_t *resolution =
		obs_properties_add_list(p, STREAMING_RESOLUTION_SETTING, obs_module_text("StreamingResolution"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(resolution, obs_module_text("StreamingResolution1080P"),
				  static_cast<long long>(StreamingResolution::P1080));
	obs_property_list_add_int(resolution, obs_module_text("StreamingResolution1080P60"),
				  static_cast<long long>(StreamingResolution::P1080_60));
	obs_property_list_add_int(resolution, obs_module_text("StreamingResolution4K"),
				  static_cast<long long>(StreamingResolution::K4));
	obs_properties_add_text(p, "device_version", obs_module_text("FirmwareVersion"), OBS_TEXT_INFO);
	obs_properties_add_text(p, "device_serial_number", obs_module_text("SerialNumber"), OBS_TEXT_INFO);
	return p;
}
static void falconm_defaults(obs_data_t *s)
{
	log_source_callback_thread("get_defaults");
	obs_data_set_default_string(s, "broker_address", "");
	obs_data_set_default_string(s, "device_id", "");
	obs_data_set_default_int(s, "broker_port", DEFAULT_MQTT_PORT);
	obs_data_set_default_int(s, STREAMING_RESOLUTION_SETTING, static_cast<long long>(StreamingResolution::P1080));
	FALCONM_LOG_INFO("FalconM: falconm_defaults settings=%p broker_address='%s' device_id='%s' broker_port=%lld",
			 (void *)s, obs_data_get_string(s, "broker_address"), obs_data_get_string(s, "device_id"),
			 obs_data_get_int(s, "broker_port"));
	obs_data_set_default_string(s, "device_version", "");
	obs_data_set_default_string(s, "device_serial_number", "");
}

obs_source_info falconm_source_info = {.id = "xbotogo_falconm",
				       .type = OBS_SOURCE_TYPE_INPUT,
//				       .output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO|
//						       OBS_SOURCE_SCENE_UNIQUE,
					.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
				       .get_name = falconm_get_name,
				       .create = falconm_create,
				       .destroy = falconm_destroy,
				       .get_defaults = falconm_defaults,
				       .get_properties = falconm_properties,
				       .update = falconm_update};

} // namespace xbotgo
