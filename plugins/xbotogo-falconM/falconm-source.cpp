#include "falconm.hpp"

#ifdef XBOTGO_DEVICE_DISCOVERY
#include <XBotGoDeviceSearchDialog.hpp>
#include <QApplication>
#endif

#include <util/base.h>

#include <limits>

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
	d->stream->setDecodedFrameCallback([d](const obs_source_frame &f) {
		output_video(d, f);
	});
	d->stream->setAudioCallback([d](const obs_source_audio &f) {
		output_audio(d, f);
	});
	if (!d->stream->connect(d->device_id, d->broker_address, d->broker_port)) {
		blog(LOG_ERROR, "FalconM: connect/startStreaming failed");
	}
}
static void falconm_deactivate(void *p)
{
	falconm_stop((falconm_source *)p);
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
	return p;
}
static void falconm_defaults(obs_data_t *s)
{
	obs_data_set_default_string(s, "broker_address", "");
	obs_data_set_default_string(s, "device_id", "");
	obs_data_set_default_int(s, "broker_port", DEFAULT_MQTT_PORT);
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
