#pragma once

#include <QDialog>
#include <obs.h>

class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QTimer;

class OBSBasicFalconMControl : public QDialog {
	Q_OBJECT
	obs_source_t *source = nullptr;
	QLabel *connection = nullptr;
	QLabel *angles = nullptr;
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
	QLabel *parametersAutoZoom = nullptr;
	QCheckBox *parametersAutoTracking = nullptr;
	QLabel *parametersAngleRange = nullptr;
	QLabel *parametersAccelSpeed = nullptr;
	QLabel *parametersCountdown = nullptr;
	QLabel *parametersFlicker = nullptr;
	QLabel *parametersSupportedResolutions = nullptr;
	QTimer *poller = nullptr;
	QTimer *modeTimeout = nullptr;
	QTimer *parametersTimeout = nullptr;
	uint64_t modeQuerySequence = 0;
	uint64_t modeResultSequence = 0;
	uint64_t displayedModesSequence = 0;
	uint64_t parametersQuerySequence = 0;
	uint64_t displayedParametersSequence = 0;
	int confirmedMode = -1;
	bool waitingForModes = false;
	bool waitingForModeResult = false;
	bool sourceWasActive = false;

public:
	explicit OBSBasicFalconMControl(obs_source_t *source, QWidget *parent = nullptr);
	~OBSBasicFalconMControl() override;

private:
	void Send(int direction, int operation);
	void QueryModes();
	void QueryCaptureParameters();
	void ApplyCaptureParameters();
	void UpdateModes();
	void UpdateCaptureParameters();
	void SelectMode(int index);
	void HandleModeResult();
	void RestoreConfirmedMode(const QString &status);
	void Refresh();
};
