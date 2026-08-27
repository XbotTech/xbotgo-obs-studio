#pragma once

#include <QObject>

#include <obs.hpp>

#include <chrono>
#include <optional>
#include <vector>

class OBSBasic;

namespace XBotGo {

enum class CameraRole;

class AutoDirector final : public QObject {
public:
	explicit AutoDirector(OBSBasic &main);
	~AutoDirector() override;

	void start();
	void stop();

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

	OBSBasic &main_;
	bool started_ = false;
	std::vector<OBSSignal> globalSignals_;
	std::optional<std::chrono::steady_clock::time_point> lastSwitch_;
	CenterConfigurationState centerConfigurationState_ = CenterConfigurationState::Unknown;

};

} // namespace XBotGo
