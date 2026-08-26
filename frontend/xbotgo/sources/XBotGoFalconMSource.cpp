#include "XBotGoFalconMSource.hpp"

#include <callback/calldata.h>

namespace XBotGo {

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

} // namespace XBotGo
