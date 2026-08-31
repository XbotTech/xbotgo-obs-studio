#pragma once

#include <QObject>
#include <obs.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace xbotgo {

enum class CameraRole;

class AutoDirector final : public QObject {
public:
	static constexpr int MinimumSwitchCooldownSeconds = 1;
	static constexpr int MaximumSwitchCooldownSeconds = 10;
	static constexpr int DefaultSwitchCooldownSeconds = 3;

	explicit AutoDirector(QObject *parent = nullptr);
	~AutoDirector() override;

	void start();
	void stop();
	bool isStarted() const noexcept { return started_; }
	int switchCooldownSeconds() const noexcept { return switchCooldownSeconds_; }
	void setSwitchCooldownSeconds(int seconds);

private:
	enum class CenterConfigurationState {
		Unknown,
		Missing,
		Valid,
		Multiple,
	};

	static void SourceCreated(void *context, calldata_t *params);
	static void SourceRemoved(void *context, calldata_t *params);
	static void MotorAngleReported(void *context, calldata_t *params);
	static bool AttachExistingSource(void *context, obs_source_t *source);
	static bool DisconnectExistingSource(void *context, obs_source_t *source);

	void attachSource(obs_source_t *source);
	void detachSource(obs_source_t *source);
	void processMotorAngle(obs_source_t *source, double horizontal,
			       std::chrono::steady_clock::time_point reportedAt);
	obs_source_t *uniqueCenterSource();
	bool hasFalconMSourceForRole(CameraRole role);
	OBSSource actualProgramSource() const;
	bool switchProgram(OBSSource targetScene);
	void setCenterConfigurationState(CenterConfigurationState state);

	bool started_ = false;
	int switchCooldownSeconds_ = DefaultSwitchCooldownSeconds;
	std::atomic<uint64_t> angleEventGeneration_{0};
	std::vector<OBSSignal> globalSignals_;
	std::optional<std::chrono::steady_clock::time_point> lastSwitch_;
	CenterConfigurationState centerConfigurationState_ = CenterConfigurationState::Unknown;
};

} // namespace xbotgo
