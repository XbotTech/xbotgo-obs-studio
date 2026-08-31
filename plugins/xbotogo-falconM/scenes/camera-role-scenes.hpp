#pragma once

#include "camera-role.hpp"

#include <obs.hpp>

#include <optional>

namespace xbotgo {

const char *CameraRoleSceneName(CameraRole role);

// Must be called on the Qt UI thread.
OBSSource GetCameraRoleScene(CameraRole role);
std::optional<CameraRole> GetSourceCameraRole(obs_source_t *source);
bool AssignSourceToCameraRoleScene(obs_source_t *source, CameraRole role);

} // namespace xbotgo
