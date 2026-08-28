#pragma once

#include <obs.h>
#include <callback/signal.h>

namespace xbotgo {

bool IsFalconMSource(obs_source_t *source);

/* Returns false when the Source is invalid, does not expose the FalconM
 * connection procedure, or reports that its control connection is unavailable. */
bool IsFalconMSourceConnected(obs_source_t *source);

bool ConnectMotorAngleReport(obs_source_t *source, signal_callback_t callback, void *context);
void DisconnectMotorAngleReport(obs_source_t *source, signal_callback_t callback, void *context);

} // namespace xbotgo
