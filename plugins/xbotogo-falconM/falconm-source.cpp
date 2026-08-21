#include "falconm.hpp"

#ifdef XBOTGO_DEVICE_DISCOVERY
#include <XBotGoDeviceSearchDialog.hpp>
#include <QApplication>
#endif

#include <util/base.h>

#include <limits>
#include <cstring>
#include <callback/calldata.h>

namespace xbotgo {

static constexpr uint16_t DEFAULT_MQTT_PORT = 1883;

static uint16_t get_broker_port(obs_data_t *settings)
{
	const long long port = obs_data_get_int(settings, "broker_port");
	return port > 0 && port <= std::numeric_limits<uint16_t>::max() ? static_cast<uint16_t>(port)
									: DEFAULT_MQTT_PORT;
}

static const char *falconm_get_name(void *)
{
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

static void falconm_stop(falconm_source *d)
{
	if (!d->active.exchange(false)) {
		return;
	}
	d->stopping = true;
	if (d->stream) {
		d->stream->send(SetMotorAngleReportingRequest{false});
		d->stream->stopStreaming();
		d->stream->disconnect();
	}
	obs_source_output_video(d->source, nullptr);
}

static void *falconm_create(obs_data_t *s, obs_source_t *source)
{
	auto *d = new falconm_source;
	d->source = source;
	d->stream = falconm_stream_create();
	d->broker_address = obs_data_get_string(s, "broker_address");
	d->device_id = obs_data_get_string(s, "device_id");
	d->broker_port = get_broker_port(s);
	falconm_register_proc_handler(d);
	return d;
}
static void falconm_destroy(void *p)
{
	auto *d = (falconm_source *)p;
	falconm_stop(d);
	delete d;
}
static void falconm_update(void *p, obs_data_t *s)
{
	auto *d = (falconm_source *)p;
	const std::string broker_address = obs_data_get_string(s, "broker_address");
	const std::string device_id = obs_data_get_string(s, "device_id");
	const uint16_t broker_port = get_broker_port(s);
	const bool connection_changed = d->broker_address != broker_address || d->device_id != device_id ||
					d->broker_port != broker_port;

	d->broker_address = broker_address;
	d->device_id = device_id;
	d->broker_port = broker_port;

	if (connection_changed && d->active) {
		d->stream->disconnect();
		if (!d->stream->connect(d->device_id, d->broker_address, d->broker_port)) {
			blog(LOG_ERROR, "FalconM: reconnect failed after connection settings changed");
		}
	}
}
static void falconm_activate(void *p)
{
	auto *d = (falconm_source *)p;
	if (d->active.exchange(true)) {
		return;
	}
	d->stopping = false;
	d->stream->setDecodedFrameCallback([d](const obs_source_frame &f) { output_video(d, f); });
	d->stream->setAudioCallback([d](const obs_source_audio &f) { output_audio(d, f); });
	if (!d->stream->connect(d->device_id, d->broker_address, d->broker_port)) {
		blog(LOG_ERROR, "FalconM: connect/startStreaming failed");
		d->active = false;
		return;
	}
}
static void falconm_deactivate(void *p)
{
	falconm_stop((falconm_source *)p);
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
	if (!calldata_get_int(cd, "mode", &mode) || mode < 0 || mode > 4 || !d->stream) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	calldata_set_bool(cd, "success", d->stream->send(SetBuzzerModeRequest{static_cast<uint8_t>(mode)}));
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

	falconm_capture_parameters parameters = d->stream->state().capture_parameters;
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
	calldata_set_bool(cd, "success", d->stream->send(SetCaptureParametersRequest{std::move(parameters)}));
}

static void falconm_set_capture_auto_tracking(void *data, calldata_t *cd)
{
	auto *d = static_cast<falconm_source *>(data);
	bool auto_tracking = false;
	if (!d->stream || !calldata_get_bool(cd, "auto_tracking", &auto_tracking)) {
		calldata_set_bool(cd, "success", false);
		return;
	}
	falconm_capture_parameters parameters = d->stream->state().capture_parameters;
	parameters.auto_tracking = auto_tracking;
	calldata_set_bool(cd, "success", d->stream->send(SetCaptureParametersRequest{std::move(parameters)}));
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
	proc_handler_add(ph, "void send_direction(int direction, int operation, out bool success)",
			 falconm_send_direction, d);
	proc_handler_add(ph, "void query_motor_angle(out bool success)", falconm_query_angle, d);
	proc_handler_add(ph, "void set_motor_angle_reporting(bool enabled, out bool success)",
			 falconm_set_angle_reporting, d);
	proc_handler_add(ph, "void set_buzzer_mode(int mode, out bool success)", falconm_set_buzzer_mode, d);
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
	obs_properties_add_text(p, "device_version", obs_module_text("FirmwareVersion"), OBS_TEXT_INFO);
	obs_properties_add_text(p, "device_serial_number", obs_module_text("SerialNumber"), OBS_TEXT_INFO);
	return p;
}
static void falconm_defaults(obs_data_t *s)
{
	obs_data_set_default_string(s, "broker_address", "");
	obs_data_set_default_string(s, "device_id", "");
	obs_data_set_default_int(s, "broker_port", DEFAULT_MQTT_PORT);
	obs_data_set_default_string(s, "device_version", "");
	obs_data_set_default_string(s, "device_serial_number", "");
}

obs_source_info falconm_source_info = {.id = "xbotogo_falconm",
				       .type = OBS_SOURCE_TYPE_INPUT,
				       .output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO |
						       OBS_SOURCE_DO_NOT_DUPLICATE,
				       .get_name = falconm_get_name,
				       .create = falconm_create,
				       .destroy = falconm_destroy,
				       .get_defaults = falconm_defaults,
				       .get_properties = falconm_properties,
				       .update = falconm_update,
				       .activate = falconm_activate,
				       .deactivate = falconm_deactivate};

} // namespace xbotgo
