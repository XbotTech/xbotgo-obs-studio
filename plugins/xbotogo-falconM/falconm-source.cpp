#include "falconm.hpp"

#ifdef XBOTGO_DEVICE_DISCOVERY
#include <XBotGoDeviceSearchDialog.hpp>
#include <QApplication>
#endif

#include <util/base.h>

#include <limits>
#include <pthread.h>

namespace xbotgo {

static constexpr uint16_t DEFAULT_MQTT_PORT = 1883;

static void log_source_callback_thread(const char *callback_name)
{
	uint64_t thread_id = 0;
	const bool is_main_thread = pthread_main_np() != 0;
	if (pthread_threadid_np(nullptr, &thread_id) != 0) {
		blog(LOG_WARNING, "FalconM: callback=%s thread_id=unavailable is_main_thread=%s", callback_name,
		     is_main_thread ? "true" : "false");
		return;
	}

	blog(LOG_INFO, "FalconM: callback=%s thread_id=%llu is_main_thread=%s", callback_name,
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
		bool requested_active = false;
		uint64_t request_serial = 0;

		{
			std::unique_lock<std::mutex> lock(d->control_mutex);
			d->control_cv.wait(lock, [d, handled_serial] {
				return d->worker_stop || d->request_serial != handled_serial;
			});

			if (d->worker_stop) {
				break;
			}

			requested_active = d->requested_active;
			broker_address = d->broker_address;
			device_id = d->device_id;
			broker_port = d->broker_port;
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

		if (!requested_active) {
			continue;
		}

		d->stopping = false;
		if (!d->stream->connect(device_id, broker_address, broker_port)) {
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

static void falconm_request_state(falconm_source *d, bool active)
{
	{
		std::lock_guard<std::mutex> lock(d->control_mutex);
		d->requested_active = active;
		++d->request_serial;
	}
	d->control_cv.notify_one();
}

static void falconm_stop(falconm_source *d)
{
	if (!d->active.exchange(false)) {
		return;
	}
	d->stopping = true;
	falconm_request_state(d, false);
	obs_source_output_video(d->source, nullptr);
}

static void *falconm_create(obs_data_t *s, obs_source_t *source)
{
	log_source_callback_thread("create");
	blog(LOG_INFO,
	     "FalconM: falconm_create settings=%p source=%p broker_address='%s' device_id='%s' broker_port=%lld",
	     (void *)s, (void *)source, obs_data_get_string(s, "broker_address"), obs_data_get_string(s, "device_id"),
	     obs_data_get_int(s, "broker_port"));
	auto *d = new falconm_source;
	d->source = source;
	d->stream = falconm_stream_create();
	d->broker_address = obs_data_get_string(s, "broker_address");
	d->device_id = obs_data_get_string(s, "device_id");
	d->broker_port = get_broker_port(s);
	d->stream->setDecodedFrameCallback([d](const obs_source_frame &f) {
		output_video(d, f);
	});
	d->stream->setAudioCallback([d](const obs_source_audio &f) {
		output_audio(d, f);
	});
	d->control_thread = std::thread(falconm_control_worker, d);
	blog(LOG_INFO, "FalconM: falconm_create result=%p broker_address='%s' device_id='%s' broker_port=%u", (void *)d,
	     d->broker_address.c_str(), d->device_id.c_str(), d->broker_port);
	return d;
}
static void falconm_destroy(void *p)
{
	log_source_callback_thread("destroy");
	auto *d = (falconm_source *)p;
	d->active = false;
	d->stopping = true;
	obs_source_output_video(d->source, nullptr);
	{
		std::lock_guard<std::mutex> lock(d->control_mutex);
		d->requested_active = false;
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
		const bool connection_changed = d->broker_address != broker_address || d->device_id != device_id ||
					 d->broker_port != broker_port;

		blog(LOG_INFO,
		     "FalconM: falconm_update source_data=%p settings=%p settings.broker_address='%s' "
		     "settings.device_id='%s' settings.broker_port=%lld current.broker_address='%s' "
		     "current.device_id='%s' current.broker_port=%u",
		     p, (void *)s, broker_address.c_str(), device_id.c_str(), obs_data_get_int(s, "broker_port"),
		     d->broker_address.c_str(), d->device_id.c_str(), d->broker_port);

		d->broker_address = broker_address;
		d->device_id = device_id;
		d->broker_port = broker_port;
		if (connection_changed && d->active) {
			d->stopping = true;
			d->requested_active = true;
			++d->request_serial;
			notify_worker = true;
		}
	}
	if (notify_worker) {
		d->control_cv.notify_one();
	}
}
static void falconm_activate(void *p)
{
	log_source_callback_thread("activate");
	auto *d = (falconm_source *)p;
	if (d->active.exchange(true)) {
		return;
	}
	falconm_request_state(d, true);
}
static void falconm_deactivate(void *p)
{
	log_source_callback_thread("deactivate");
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
	return p;
}
static void falconm_defaults(obs_data_t *s)
{
	log_source_callback_thread("get_defaults");
	obs_data_set_default_string(s, "broker_address", "");
	obs_data_set_default_string(s, "device_id", "");
	obs_data_set_default_int(s, "broker_port", DEFAULT_MQTT_PORT);
	blog(LOG_INFO, "FalconM: falconm_defaults settings=%p broker_address='%s' device_id='%s' broker_port=%lld",
	     (void *)s, obs_data_get_string(s, "broker_address"), obs_data_get_string(s, "device_id"),
	     obs_data_get_int(s, "broker_port"));
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
				       .update = falconm_update,
				       .activate = falconm_activate,
					       .deactivate = falconm_deactivate};

} // namespace xbotgo
