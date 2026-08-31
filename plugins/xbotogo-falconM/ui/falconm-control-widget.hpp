#pragma once

#include "../falconm-source-bridge.hpp"

#include <QWidget>

#include <cstdint>
#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTimer;

namespace xbotgo {

class SliderControl;
class ComboBoxControl;
enum class BuzzerMode : uint8_t;

class FalconMControlWidget final : public QWidget {
	Q_OBJECT

public:
	explicit FalconMControlWidget(obs_source_t *source, QWidget *parent = nullptr);

signals:
	void connectionStateChanged(bool connected);

private:
	void Send(int direction, int operation);
	void SendBuzzerMode(BuzzerMode mode);
	void QueryHallCalibration();
	void StartHallCalibration();
	void UpdateHallCalibration(const falconm_device_state &state);
	void QueryCurrentZoom();
	void UpdateCurrentZoom(const falconm_device_state &state);
	void ManualZoomValueChanged(int value);
	bool DisableAutoZoomForManualControl();
	void UpdateManualZoomEnabled();
	void QueryModes();
	void QueryCaptureParameters();
	void ApplyAutoZoom(bool checked);
	void ApplyAutoTracking(bool checked);
	void ApplyAngleRange();
	bool SendCaptureParameters(bool autoZoom, bool autoTracking, int angleRange);
	void UpdateModes(const falconm_device_state &state);
	void UpdateCaptureParameters(const falconm_device_state &state);
	void SelectMode(int index);
	void HandleModeResult(const falconm_device_state &state);
	void RestoreConfirmedMode();
	void Refresh();

	FalconMSourceBridge source_;
	ComboBoxControl *cameraRoleControl = nullptr;
	QLabel *angles = nullptr;
	std::vector<QPushButton *> directionButtons;
	QPushButton *buzzerLongButton = nullptr;
	QPushButton *hallCalibrationStart = nullptr;
	QComboBox *modeSelector = nullptr;
	QCheckBox *parametersAutoZoom = nullptr;
	QCheckBox *parametersAutoTracking = nullptr;
	SliderControl *parametersAngleRange = nullptr;
	SliderControl *manualZoomSlider = nullptr;
	QTimer *modeTimeout = nullptr;
	QTimer *hallCalibrationTimeout = nullptr;
	QTimer *manualZoomQueryDebounce = nullptr;
	uint64_t modeQuerySequence = 0;
	uint64_t modeResultSequence = 0;
	uint64_t displayedModesSequence = 0;
	uint64_t parametersQuerySequence = 0;
	uint64_t displayedParametersSequence = 0;
	uint64_t hallCalibrationQuerySequence = 0;
	uint64_t displayedHallCalibrationSequence = 0;
	uint64_t manualZoomQuerySequence = 0;
	uint64_t displayedManualZoomSequence = 0;
	int currentHallCalibrationStatus = -1;
	int manualZoomCommandValue = 10;
	int confirmedMode = -1;
	int confirmedAngleRange = 0;
	bool confirmedAutoZoom = false;
	bool confirmedAutoTracking = false;
	bool hasConfirmedCaptureParameters = false;
	bool waitingForModes = false;
	bool waitingForModeResult = false;
	bool sourceWasConnected = false;
	bool hasCurrentManualZoom = false;
	bool manualZoomDragging = false;
};

} // namespace xbotgo
