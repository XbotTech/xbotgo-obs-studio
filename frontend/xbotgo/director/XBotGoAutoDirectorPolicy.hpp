#pragma once

#include <xbotgo/scenes/XBotGoCameraRoleScenes.hpp>

#include <chrono>
#include <optional>

namespace xbotgo::AutoDirectorPolicy {

constexpr double LeftAngleBoundary = -30.0;
constexpr double RightAngleBoundary = 30.0;
constexpr std::chrono::seconds SwitchCooldown{3};

enum class ProgramSwitchPath {
	SetCurrentScene,
	TransitionToScene,
};

constexpr ProgramSwitchPath ProgramSwitchPathForMode(bool previewProgramMode)
{
	return previewProgramMode ? ProgramSwitchPath::TransitionToScene : ProgramSwitchPath::SetCurrentScene;
}

constexpr bool CanStartProgramSwitch(bool transitionAvailable, bool transitionActive)
{
	return transitionAvailable && !transitionActive;
}

constexpr CameraRole CameraRoleForHorizontalAngle(double angle)
{
	if (angle < LeftAngleBoundary) {
		return CameraRole::Left;
	}
	if (angle > RightAngleBoundary) {
		return CameraRole::Right;
	}
	return CameraRole::Center;
}

inline bool IsSwitchCoolingDown(std::optional<std::chrono::steady_clock::time_point> lastSwitch,
				std::chrono::steady_clock::time_point now)
{
	return lastSwitch && now - *lastSwitch < SwitchCooldown;
}

} // namespace xbotgo::AutoDirectorPolicy
