#include "XBotGoFalconMSource.hpp"

#include <callback/calldata.h>

#include <cstring>

namespace XBotGo {
namespace {

constexpr char FalconMSourceId[] = "xbotogo_falconm";
constexpr char MotorAngleReportSignal[] = "motor_angle_report";

} // namespace

bool IsFalconMSource(obs_source_t *source)
{
	const char *sourceId = source ? obs_source_get_id(source) : nullptr;
	return sourceId && strcmp(sourceId, FalconMSourceId) == 0;
}

bool IsFalconMSourceConnected(obs_source_t *source)
{
	if (!source) {
		return false;
	}

	proc_handler_t *handler = obs_source_get_proc_handler(source);
	if (!handler) {
		return false;
	}

	calldata_t cd;
	calldata_init(&cd);
	const bool called = proc_handler_call(handler, "get_connection_state", &cd);
	bool connected = false;
	const bool hasConnectionState = calldata_get_bool(&cd, "connected", &connected);
	calldata_free(&cd);
	return called && hasConnectionState && connected;
}

bool ConnectMotorAngleReport(obs_source_t *source, signal_callback_t callback, void *context)
{
	if (!IsFalconMSource(source) || !callback) {
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

void DisconnectMotorAngleReport(obs_source_t *source, signal_callback_t callback, void *context)
{
	if (!IsFalconMSource(source) || !callback) {
		return;
	}

	signal_handler_t *handler = obs_source_get_signal_handler(source);
	if (handler) {
		signal_handler_disconnect(handler, MotorAngleReportSignal, callback, context);
	}
}

} // namespace XBotGo
