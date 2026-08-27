#include "XBotGoAutoDirector.hpp"

#include "XBotGoAutoDirectorPolicy.hpp"

#include <widgets/OBSBasic.hpp>

#include <callback/calldata.h>
#include <util/base.h>
#include <xbotgo/scenes/XBotGoCameraRoleScenes.hpp>
#include <xbotgo/sources/XBotGoFalconMSource.hpp>

#include <QMetaObject>
#include <QThread>

#include <cmath>

namespace XBotGo {
namespace {

const char *CameraRoleName(CameraRole role)
{
	switch (role) {
	case CameraRole::Center:
		return "center";
	case CameraRole::Left:
		return "left";
	case CameraRole::Right:
		return "right";
	}
	return "unknown";
}

} // namespace

AutoDirector::AutoDirector(OBSBasic &main) : QObject(&main), main_(main) {}

AutoDirector::~AutoDirector()
{
	stop();
}

void AutoDirector::start()
{
	if (started_) {
		return;
	}

	started_ = true;
	globalSignals_.reserve(3);
	globalSignals_.emplace_back(obs_get_signal_handler(), "source_create", SourceCreated, this);
	globalSignals_.emplace_back(obs_get_signal_handler(), "source_remove", SourceRemoved, this);
	globalSignals_.emplace_back(obs_get_signal_handler(), "source_destroy", SourceRemoved, this);
	obs_enum_sources(AttachExistingSource, this);
	blog(LOG_INFO, "XBotGo auto director started");
}

void AutoDirector::stop()
{
	if (!started_) {
		return;
	}

	started_ = false;
	globalSignals_.clear();
	obs_enum_sources(DisconnectExistingSource, this);
	lastSwitch_.reset();
	centerConfigurationState_ = CenterConfigurationState::Unknown;
	blog(LOG_INFO, "XBotGo auto director stopped");
}

void AutoDirector::SourceCreated(void *context, calldata_t *params)
{
	auto *director = static_cast<AutoDirector *>(context);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	OBSSource sourceRef(source);
	QMetaObject::invokeMethod(
		director,
		[director, sourceRef] {
			if (director->started_ && sourceRef && !obs_source_removed(sourceRef)) {
				director->attachSource(sourceRef);
			}
		},
		Qt::QueuedConnection);
}

void AutoDirector::SourceRemoved(void *context, calldata_t *params)
{
	auto *director = static_cast<AutoDirector *>(context);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	director->detachSource(source);
}

void AutoDirector::MotorAngleReported(void *context, calldata_t *params)
{
	auto *director = static_cast<AutoDirector *>(context);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	double horizontal = 0.0;
	if (!source || !calldata_get_float(params, "horizontal", &horizontal) || !std::isfinite(horizontal)) {
		return;
	}

	const auto reportedAt = std::chrono::steady_clock::now();
	OBSSource sourceRef(source);
	QMetaObject::invokeMethod(
		director,
		[director, sourceRef, horizontal, reportedAt] {
			if (director->started_) {
				director->processMotorAngle(sourceRef, horizontal, reportedAt);
			}
		},
		Qt::QueuedConnection);
}

bool AutoDirector::AttachExistingSource(void *context, obs_source_t *source)
{
	static_cast<AutoDirector *>(context)->attachSource(source);
	return true;
}

bool AutoDirector::DisconnectExistingSource(void *context, obs_source_t *source)
{
	static_cast<AutoDirector *>(context)->detachSource(source);
	return true;
}

void AutoDirector::attachSource(obs_source_t *source)
{
	if (started_ && IsFalconMSource(source)) {
		ConnectMotorAngleReport(source, MotorAngleReported, this);
	}
}

void AutoDirector::detachSource(obs_source_t *source)
{
	DisconnectMotorAngleReport(source, MotorAngleReported, this);
}

obs_source_t *AutoDirector::uniqueCenterSource()
{
	struct CenterSourceData {
		AutoDirector &director;
		obs_source_t *source = nullptr;
		size_t count = 0;
	} data{*this};

	obs_enum_sources(
		[](void *context, obs_source_t *source) {
			auto &data = *static_cast<CenterSourceData *>(context);
			if (!IsFalconMSource(source)) {
				return true;
			}

			const auto role = GetSourceCameraRole(data.director.main_, source);
			if (role == CameraRole::Center) {
				data.source = source;
				++data.count;
			}
			return true;
		},
		&data);

	if (data.count == 0) {
		setCenterConfigurationState(CenterConfigurationState::Missing);
		return nullptr;
	}
	if (data.count > 1) {
		setCenterConfigurationState(CenterConfigurationState::Multiple);
		return nullptr;
	}

	setCenterConfigurationState(CenterConfigurationState::Valid);
	return data.source;
}

bool AutoDirector::hasFalconMSourceForRole(CameraRole role)
{
	struct RoleSourceData {
		AutoDirector &director;
		CameraRole role;
		bool found = false;
	} data{*this, role};

	obs_enum_sources(
		[](void *context, obs_source_t *source) {
			auto &data = *static_cast<RoleSourceData *>(context);
			if (IsFalconMSource(source) && GetSourceCameraRole(data.director.main_, source) == data.role) {
				data.found = true;
				return false;
			}
			return true;
		},
		&data);
	return data.found;
}

OBSSource AutoDirector::actualProgramSource() const
{
	OBSSourceAutoRelease transition = obs_get_output_source(0);
	OBSSourceAutoRelease activeSource = transition ? obs_transition_get_active_source(transition) : nullptr;
	if (activeSource) {
		return OBSSource(activeSource);
	}

	return main_.IsPreviewProgramMode() ? main_.GetProgramSource() : main_.GetCurrentSceneSource();
}

bool AutoDirector::switchProgram(OBSSource targetScene)
{
	OBSSourceAutoRelease transition = obs_get_output_source(0);
	const bool transitionAvailable = transition != nullptr;
	const bool transitionActive = transitionAvailable && obs_transition_is_active(transition);
	if (!AutoDirectorPolicy::CanStartProgramSwitch(transitionAvailable, transitionActive)) {
		return false;
	}

	switch (AutoDirectorPolicy::ProgramSwitchPathForMode(main_.IsPreviewProgramMode())) {
	case AutoDirectorPolicy::ProgramSwitchPath::SetCurrentScene:
		main_.SetCurrentScene(targetScene);
		break;
	case AutoDirectorPolicy::ProgramSwitchPath::TransitionToScene:
		main_.TransitionToScene(targetScene);
		break;
	}

	return actualProgramSource() == targetScene;
}

void AutoDirector::setCenterConfigurationState(CenterConfigurationState state)
{
	if (centerConfigurationState_ == state) {
		return;
	}

	if (state == CenterConfigurationState::Missing) {
		blog(LOG_WARNING, "XBotGo auto director paused: center camera is not configured");
	} else if (state == CenterConfigurationState::Multiple) {
		blog(LOG_WARNING, "XBotGo auto director paused: multiple center cameras are configured");
	} else if (state == CenterConfigurationState::Valid &&
		   centerConfigurationState_ != CenterConfigurationState::Unknown) {
		blog(LOG_INFO, "XBotGo auto director resumed: center camera configuration is valid");
	}
	centerConfigurationState_ = state;
}

void AutoDirector::processMotorAngle(
	obs_source_t *source,
	double horizontal,
	std::chrono::steady_clock::time_point reportedAt
	)
{
	if (QThread::currentThread() != thread()) {
		blog(LOG_ERROR, "XBotGo auto director rejected motor angle outside the Qt UI thread");
		return;
	}

	// 1. 获取中间机位
	obs_source_t *centerSource = uniqueCenterSource();
	if (!centerSource || centerSource != source || !IsFalconMSourceConnected(source)) {
		return;
	}

	// 2. 冷却中跳过本次检测
	if (AutoDirectorPolicy::IsSwitchCoolingDown(lastSwitch_, reportedAt)) {
		return;
	}

	// 3. 根据角度，判断目标机位
	const CameraRole targetRole = AutoDirectorPolicy::CameraRoleForHorizontalAngle(horizontal);

	// 4. 获取目标场景对应的源
	OBSSource targetScene = GetCameraRoleScene(main_, targetRole);

	// 5. 目标场景不存在或目标场景下无源，不进行切换
	if (!targetScene || !hasFalconMSourceForRole(targetRole)) {
		return;
	}

	OBSSource currentProgram = actualProgramSource();
	if (currentProgram == targetScene) {
		return;
	}

	// 执行切换
	if (!switchProgram(targetScene)) {
		return;
	}
	lastSwitch_ = std::chrono::steady_clock::now();
	blog(LOG_INFO, "XBotGo auto director switched program to %s camera at horizontal angle %.2f",
	     CameraRoleName(targetRole), horizontal);
}

} // namespace XBotGo
