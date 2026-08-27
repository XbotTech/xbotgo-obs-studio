#include "XBotGoAutoDirectorPolicy.hpp"

#include <cassert>
#include <chrono>

using namespace std::chrono_literals;

int main()
{
	using XBotGo::CameraRole;
	using XBotGo::AutoDirectorPolicy::CameraRoleForHorizontalAngle;
	using XBotGo::AutoDirectorPolicy::CanStartProgramSwitch;
	using XBotGo::AutoDirectorPolicy::IsSwitchCoolingDown;
	using XBotGo::AutoDirectorPolicy::ProgramSwitchPath;
	using XBotGo::AutoDirectorPolicy::ProgramSwitchPathForMode;

	assert(CameraRoleForHorizontalAngle(-30.01) == CameraRole::Left);
	assert(CameraRoleForHorizontalAngle(-30.0) == CameraRole::Center);
	assert(CameraRoleForHorizontalAngle(0.0) == CameraRole::Center);
	assert(CameraRoleForHorizontalAngle(30.0) == CameraRole::Center);
	assert(CameraRoleForHorizontalAngle(30.01) == CameraRole::Right);

	const auto switchedAt = std::chrono::steady_clock::time_point{10s};
	assert(!IsSwitchCoolingDown(std::nullopt, switchedAt));
	assert(IsSwitchCoolingDown(switchedAt, switchedAt - 1ms));
	assert(IsSwitchCoolingDown(switchedAt, switchedAt + 2999ms));
	assert(!IsSwitchCoolingDown(switchedAt, switchedAt + 3s));
	assert(!IsSwitchCoolingDown(switchedAt, switchedAt + 4s));

	assert(ProgramSwitchPathForMode(false) == ProgramSwitchPath::SetCurrentScene);
	assert(ProgramSwitchPathForMode(true) == ProgramSwitchPath::TransitionToScene);
	assert(!CanStartProgramSwitch(false, false));
	assert(!CanStartProgramSwitch(true, true));
	assert(CanStartProgramSwitch(true, false));

	return 0;
}
