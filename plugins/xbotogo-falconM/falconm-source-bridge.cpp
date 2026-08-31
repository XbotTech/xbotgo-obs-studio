#include "falconm-source-bridge.hpp"

#include "falconm.hpp"

#include <cstring>

namespace xbotgo {
namespace {

constexpr char FalconMSourceId[] = "xbotogo_falconm";
constexpr char MotorAngleReportSignal[] = "motor_angle_report";

falconm_source *SourceData(obs_source_t *source)
{
	if (!FalconMSourceBridge::IsFalconM(source) || obs_source_removed(source)) {
		return nullptr;
	}

	auto *data = static_cast<falconm_source *>(obs_obj_get_data(source));
	return data && data->stream ? data : nullptr;
}

} // namespace

FalconMSourceBridge::FalconMSourceBridge(obs_source_t *source) : source_(OBSGetWeakRef(source))
{
	const char *uuid = source ? obs_source_get_uuid(source) : nullptr;
	if (uuid) {
		uuid_ = uuid;
	}
}

OBSSource FalconMSourceBridge::lock() const
{
	OBSSource source = OBSGetStrongRef(source_);
	return SourceData(source) ? source : nullptr;
}

std::string FalconMSourceBridge::uuid() const
{
	return uuid_;
}

bool FalconMSourceBridge::valid() const
{
	return static_cast<bool>(lock());
}

bool FalconMSourceBridge::connected() const
{
	OBSSource source = lock();
	auto *data = SourceData(source);
	return data && data->stream->isConnected();
}

bool FalconMSourceBridge::send(const FalconRequest &request) const
{
	OBSSource source = lock();
	auto *data = SourceData(source);
	return data && data->stream->isConnected() && data->stream->send(request);
}

falconm_device_state FalconMSourceBridge::state() const
{
	OBSSource source = lock();
	auto *data = SourceData(source);
	return data ? data->stream->state() : falconm_device_state{};
}

bool FalconMSourceBridge::connectMotorAngleReport(signal_callback_t callback, void *context) const
{
	OBSSource source = lock();
	if (!source || !callback) {
		return false;
	}

	signal_handler_t *handler = obs_source_get_signal_handler(source);
	if (!handler) {
		return false;
	}

	signal_handler_disconnect(handler, MotorAngleReportSignal, callback, context);
	signal_handler_connect_ref(handler, MotorAngleReportSignal, callback, context);
	return true;
}

void FalconMSourceBridge::disconnectMotorAngleReport(signal_callback_t callback, void *context) const
{
	OBSSource source = lock();
	if (!source || !callback) {
		return;
	}

	signal_handler_t *handler = obs_source_get_signal_handler(source);
	if (handler) {
		signal_handler_disconnect(handler, MotorAngleReportSignal, callback, context);
	}
}

bool FalconMSourceBridge::IsFalconM(obs_source_t *source)
{
	const char *sourceId = source ? obs_source_get_id(source) : nullptr;
	return sourceId && std::strcmp(sourceId, FalconMSourceId) == 0;
}

} // namespace xbotgo
