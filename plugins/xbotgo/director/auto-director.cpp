#include "auto-director.hpp"

#include "auto-director-policy.hpp"

#include <callback/calldata.h>
#include <util/base.h>
#include "../scenes/camera-role-scenes.hpp"
#include "../falconm-source-bridge.hpp"
#include <obs-frontend-api.h>

#include <QMetaObject>
#include <QThread>

#include <cmath>
#include <stdexcept>

namespace xbotgo {
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

AutoDirector::AutoDirector(QObject *parent) : QObject(parent) {}

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
	angleEventGeneration_.fetch_add(1, std::memory_order_relaxed);
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
	angleEventGeneration_.fetch_add(1, std::memory_order_relaxed);
	globalSignals_.clear();
	obs_enum_sources(DisconnectExistingSource, this);
	lastSwitch_.reset();
	centerConfigurationState_ = CenterConfigurationState::Unknown;
	blog(LOG_INFO, "XBotGo auto director stopped");
}

void AutoDirector::setSwitchCooldownSeconds(int seconds)
{
	if (QThread::currentThread() != thread()) {
		throw std::logic_error("XBotGo auto director cooldown changed outside the Qt UI thread");
	}
	if (seconds < MinimumSwitchCooldownSeconds || seconds > MaximumSwitchCooldownSeconds) {
		throw std::out_of_range("XBotGo auto director cooldown is outside the supported range");
	}

	switchCooldownSeconds_ = seconds;
}

void AutoDirector::SourceCreated(void *context, calldata_t *params)
{
	auto *director = static_cast<AutoDirector *>(context);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	OBSSource sourceRef(source);
	const uint64_t generation = director->angleEventGeneration_.load(std::memory_order_relaxed);
	QMetaObject::invokeMethod(
		director,
		[director, sourceRef, generation] {
			if (director->started_ && sourceRef && !obs_source_removed(sourceRef) &&
			    director->angleEventGeneration_.load(std::memory_order_relaxed) == generation) {
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
	const uint64_t generation = director->angleEventGeneration_.load(std::memory_order_relaxed);
	QMetaObject::invokeMethod(
		director,
		[director, sourceRef, horizontal, reportedAt, generation] {
			if (director->started_ &&
			    director->angleEventGeneration_.load(std::memory_order_relaxed) == generation) {
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
	if (started_ && FalconMSourceBridge::IsFalconM(source)) {
		FalconMSourceBridge(source).connectMotorAngleReport(MotorAngleReported, this);
	}
}

void AutoDirector::detachSource(obs_source_t *source)
{
	FalconMSourceBridge(source).disconnectMotorAngleReport(MotorAngleReported, this);
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
			if (!FalconMSourceBridge::IsFalconM(source)) {
				return true;
			}

			const auto role = GetSourceCameraRole(source);
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
			if (FalconMSourceBridge::IsFalconM(source) && GetSourceCameraRole(source) == data.role) {
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
	OBSSourceAutoRelease source = obs_frontend_get_current_scene();
	return source ? OBSSource(source) : OBSSource{};
}

bool AutoDirector::switchProgram(OBSSource targetScene)
{
	OBSSourceAutoRelease transition = obs_get_output_source(0);
	const bool transitionAvailable = transition != nullptr;
	const bool transitionActive = transitionAvailable && obs_transition_is_active(transition);
	if (!AutoDirectorPolicy::CanStartProgramSwitch(transitionAvailable, transitionActive)) {
		return false;
	}

	obs_frontend_set_current_scene(targetScene);
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

void AutoDirector::processMotorAngle(obs_source_t *source, double horizontal,
				     std::chrono::steady_clock::time_point reportedAt)
{
	if (QThread::currentThread() != thread()) {
		throw std::logic_error("XBotGo auto director processed motor angle outside the Qt UI thread");
	}
	if (!started_) {
		return;
	}

	// 1. 获取中间机位
	obs_source_t *centerSource = uniqueCenterSource();
	if (!centerSource || centerSource != source || !FalconMSourceBridge(source).connected()) {
		return;
	}

	// 2. 冷却中跳过本次检测
	if (AutoDirectorPolicy::IsSwitchCoolingDown(lastSwitch_, reportedAt,
						    std::chrono::seconds{switchCooldownSeconds_})) {
		return;
	}

	// 3. 根据角度，判断目标机位
	const CameraRole targetRole = AutoDirectorPolicy::CameraRoleForHorizontalAngle(horizontal);

	// 4. 获取目标场景对应的源
	OBSSource targetScene = GetCameraRoleScene(targetRole);

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

} // namespace xbotgo
