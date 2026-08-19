#include "falconm.hpp"
#include <util/base.h>

namespace xbotgo {

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
	const bool connection_changed = d->broker_address != broker_address || d->device_id != device_id;

	d->broker_address = broker_address;
	d->device_id = device_id;

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
		d->active = false;
	}
}
static void falconm_deactivate(void *p)
{
	falconm_stop((falconm_source *)p);
}
static obs_properties_t *falconm_properties(void *)
{
	auto *p = obs_properties_create();
	obs_properties_add_text(p, "broker_address", "MQTT Broker IP", OBS_TEXT_DEFAULT);
	obs_properties_add_text(p, "device_id", "Device ID", OBS_TEXT_DEFAULT);
	return p;
}
static void falconm_defaults(obs_data_t *s)
{
	obs_data_set_default_string(s, "broker_address", "169.254.184.18");
	obs_data_set_default_string(s, "device_id", "Xbt-F-6c092e");
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
