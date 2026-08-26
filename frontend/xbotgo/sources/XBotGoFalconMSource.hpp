#pragma once

#include <obs.h>

namespace XBotGo {

/* Returns false when the Source is invalid, does not expose the FalconM
 * connection procedure, or reports that its control connection is unavailable. */
bool IsFalconMSourceConnected(obs_source_t *source);

} // namespace XBotGo
