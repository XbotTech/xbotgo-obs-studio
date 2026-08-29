#pragma once

#include <xbotgo/sources/XBotGoSourceObserver.hpp>

#include <QWidget>
#include <obs.h>

#include <cstdint>
#include <vector>

class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QTimer;

namespace xbotgo {
class ComboBoxControl;
class SliderControl;
} // namespace xbotgo

namespace xbotgo {
enum class BuzzerMode : uint8_t;
}

class OBSBasicFalconMControl : public QWidget {
	Q_OBJECT
	xbotgo::SourceObserver sourceObserver;
	xbotgo::ComboBoxControl *cameraRoleControl = nullptr;
	QLabel *angles = nullptr;
	std::vector<QPushButton *> directionButtons;
	QPushButton *buzzerLongButton = nullptr;
	QPushButton *hallCalibrationStart = nullptr;
	QComboBox *modeSelector = nullptr;
	QCheckBox *parametersAutoZoom = nullptr;
	QCheckBox *parametersAutoTracking = nullptr;
	xbotgo::SliderControl *parametersAngleRange = nullptr;
	xbotgo::SliderControl *manualZoomSlider = nullptr;
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

public:
	explicit OBSBasicFalconMControl(obs_source_t *source, QWidget *parent = nullptr);

signals:
	void connectionStateChanged(bool connected);

private:
	void Send(int direction, int operation);
	void SendBuzzerMode(xbotgo::BuzzerMode mode);
	void QueryHallCalibration();
	void StartHallCalibration();
	void UpdateHallCalibration();
	void QueryCurrentZoom();
	void UpdateCurrentZoom();
	void ManualZoomValueChanged(int value);
	bool DisableAutoZoomForManualControl();
	void UpdateManualZoomEnabled();
	void QueryModes();
	void QueryCaptureParameters();
	void ApplyAutoZoom(bool checked);
	void ApplyAutoTracking(bool checked);
	void ApplyAngleRange();
	void UpdateModes();
	void UpdateCaptureParameters();
	void SelectMode(int index);
	void HandleModeResult();
	void RestoreConfirmedMode();
	void Refresh();
};
