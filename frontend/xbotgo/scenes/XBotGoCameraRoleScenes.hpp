#pragma once

#include <obs.hpp>

#include <optional>

class OBSBasic;

namespace XBotGo {

enum class CameraRole {
	Center,
	Left,
	Right,
};

const char *CameraRoleSceneName(CameraRole role);

// Returns an existing role scene and never creates one. Must be called on the Qt UI thread.
OBSSource GetCameraRoleScene(OBSBasic &main, CameraRole role);

// Must be called on the Qt UI thread. source is borrowed and remains owned by OBS.
std::optional<CameraRole> GetSourceCameraRole(OBSBasic &main, obs_source_t *source);

// Adds source to the target before removing other scene items. Returns false without
// removing existing items when the target scene cannot be prepared.
bool AssignSourceToCameraRoleScene(OBSBasic &main, obs_source_t *source, CameraRole role);

} // namespace XBotGo
