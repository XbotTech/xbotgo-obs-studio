#pragma once

#include <QDialog>
#include <obs.h>

class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QTimer;

namespace XBotGo {
class SliderControl;
}

class OBSBasicFalconMControl : public QDialog {
	Q_OBJECT
	obs_source_t *source = nullptr;
	QLabel *connection = nullptr;
	QLabel *angles = nullptr;
	QPushButton *buzzerLongButton = nullptr;
	QLabel *buzzerStatus = nullptr;
	QLabel *hallCalibrationStatus = nullptr;
	QPushButton *hallCalibrationRefresh = nullptr;
	QPushButton *hallCalibrationStart = nullptr;
	QLabel *modeStatus = nullptr;
	QComboBox *modeSelector = nullptr;
	QPushButton *modeRefresh = nullptr;
	QPushButton *parametersRefresh = nullptr;
	QPushButton *parametersApply = nullptr;
	QLabel *parametersStatus = nullptr;
	QLabel *parametersMode = nullptr;
	QLabel *parametersResolution = nullptr;
	QLabel *parametersResolutionId = nullptr;
	QLabel *parametersWatermark = nullptr;
	QLabel *parametersMute = nullptr;
	QCheckBox *parametersAutoZoom = nullptr;
	QCheckBox *parametersAutoTracking = nullptr;
	XBotGo::SliderControl *parametersAngleRange = nullptr;
	QLabel *parametersAccelSpeed = nullptr;
	QLabel *parametersCountdown = nullptr;
	QLabel *parametersFlicker = nullptr;
	QLabel *parametersSupportedResolutions = nullptr;
	XBotGo::SliderControl *manualZoomSlider = nullptr;
	QLabel *manualZoomStatus = nullptr;
	QTimer *poller = nullptr;
	QTimer *modeTimeout = nullptr;
	QTimer *parametersTimeout = nullptr;
	QTimer *hallCalibrationTimeout = nullptr;
	QTimer *manualZoomTimeout = nullptr;
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
	int currentManualZoom = 10;
	int manualZoomCommandValue = 10;
	int confirmedMode = -1;
	int confirmedAngleRange = 0;
	bool confirmedAutoZoom = false;
	bool confirmedAutoTracking = false;
	bool hasConfirmedCaptureParameters = false;
	bool applyingCaptureParameters = false;
	bool waitingForModes = false;
	bool waitingForModeResult = false;
	bool sourceWasActive = false;
	bool hasCurrentManualZoom = false;
	bool manualZoomDragging = false;

public:
	explicit OBSBasicFalconMControl(obs_source_t *source, QWidget *parent = nullptr);
	~OBSBasicFalconMControl() override;

private:
	void Send(int direction, int operation);
	void SendBuzzerMode(int mode);
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
	void ApplyCaptureParameters();
	void UpdateParametersApplyEnabled();
	void UpdateModes();
	void UpdateCaptureParameters();
	void SelectMode(int index);
	void HandleModeResult();
	void RestoreConfirmedMode(const QString &status);
	void Refresh();
};
