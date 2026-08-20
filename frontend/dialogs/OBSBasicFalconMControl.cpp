#include "OBSBasicFalconMControl.hpp"

#include <OBSApp.hpp>
#include "moc_OBSBasicFalconMControl.cpp"

#include <obs.h>
#include <callback/calldata.h>
#include <qt-wrappers.hpp>

#include "../../plugins/xbotogo-falconM/falconm-protocol.hpp"

#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int FalconMModeProtocolVersion = 3;

QString ModeLabel(uint16_t mode, bool beta)
{
	QString label;
	switch (mode) {
	case 1:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.1");
		break;
	case 2:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.2");
		break;
	case 3:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.3");
		break;
	case 4:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.4");
		break;
	case 5:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.5");
		break;
	case 6:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.6");
		break;
	case 7:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.7");
		break;
	case 8:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.8");
		break;
	case 11:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.11");
		break;
	case 12:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.12");
		break;
	case 13:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.13");
		break;
	case 14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.14");
		break;
	case 15:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.15");
		break;
	case 16:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.16");
		break;
	case 17:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.17");
		break;
	case 18:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.18");
		break;
	case 19:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.19");
		break;
	case 20:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.20");
		break;
	case 23:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.23");
		break;
	case 24:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.24");
		break;
	case 25:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.25");
		break;
	case 26:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.26");
		break;
	case 27:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.27");
		break;
	case 28:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.28");
		break;
	case 29:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.29");
		break;
	case 30:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.30");
		break;
	case 31:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.31");
		break;
	case 32:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.32");
		break;
	case 33:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.33");
		break;
	case 34:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.34");
		break;
	case 36:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.36");
		break;
	case 37:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.37");
		break;
	case 38:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.38");
		break;
	case 39:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.39");
		break;
	case 40:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.40");
		break;
	case 41:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.41");
		break;
	case 42:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.42");
		break;
	case 43:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.43");
		break;
	case 44:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.44");
		break;
	case 45:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.45");
		break;
	case 46:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.46");
		break;
	case 47:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.47");
		break;
	case 48:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.48");
		break;
	case 49:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.49");
		break;
	case 50:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.50");
		break;
	default:
		label = QStringLiteral("Mode %1").arg(mode);
		break;
	}
	return beta ? QStringLiteral("%1 (Beta)").arg(label) : label;
}
} // namespace

OBSBasicFalconMControl::OBSBasicFalconMControl(obs_source_t *source_, QWidget *parent)
	: QDialog(parent),
	  source(obs_source_get_ref(source_))
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Control"));
	resize(420, 300);

	connection = new QLabel(this);
	modeStatus = new QLabel(this);
	modeSelector = new QComboBox(this);
	modeRefresh = new QPushButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Refresh"), this);
	parametersRefresh = new QPushButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Refresh"), this);
	parametersApply = new QPushButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Apply"), this);
	parametersApply->setEnabled(false);
	parametersStatus = new QLabel(this);
	parametersMode = new QLabel(this);
	parametersResolution = new QLabel(this);
	parametersResolutionId = new QLabel(this);
	parametersWatermark = new QLabel(this);
	parametersMute = new QLabel(this);
	parametersAutoZoom = new QLabel(this);
	parametersAutoTracking = new QCheckBox(this);
	parametersAngleRange = new QLabel(this);
	parametersAccelSpeed = new QLabel(this);
	parametersCountdown = new QLabel(this);
	parametersFlicker = new QLabel(this);
	parametersSupportedResolutions = new QLabel(this);
	parametersSupportedResolutions->setWordWrap(true);
	connection->setText(QTStr(obs_source_active(source) ? "Basic.MainMenu.XBotGo.DeviceManagement.Active"
							    : "Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));

	angles = new QLabel(this);
	auto *grid = new QGridLayout;
	const auto addButton = [this, grid](const QString &label, int row, int col, int direction) {
		auto *button = new QPushButton(label, this);
		grid->addWidget(button, row, col);
		connect(button, &QPushButton::pressed, this, [this, direction] { Send(direction, 1); });
		connect(button, &QPushButton::released, this, [this, direction] { Send(direction, 2); });
	};
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Up"), 0, 1, 0);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Left"), 1, 0, 2);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Center"), 1, 1, 4);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Right"), 1, 2, 3);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Down"), 2, 1, 1);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(connection);
	auto *modeLayout = new QHBoxLayout;
	modeLayout->addWidget(new QLabel(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureMode"), this));
	modeLayout->addWidget(modeSelector, 1);
	modeLayout->addWidget(modeRefresh);
	layout->addLayout(modeLayout);
	layout->addWidget(modeStatus);
	auto *parametersLayout = new QVBoxLayout;
	parametersLayout->addWidget(
		new QLabel(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureParameters"), this));
	parametersLayout->addWidget(parametersStatus);
	auto *parametersForm = new QFormLayout;
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterMode"), parametersMode);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterResolution"),
			       parametersResolution);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterResolutionId"),
			       parametersResolutionId);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterWatermark"), parametersWatermark);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterMute"), parametersMute);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAutoZoom"), parametersAutoZoom);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAutoTracking"),
			       parametersAutoTracking);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAngleRange"),
			       parametersAngleRange);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAccelSpeed"),
			       parametersAccelSpeed);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterCountdown"), parametersCountdown);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterFlicker"), parametersFlicker);
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterSupportedResolutions"),
			       parametersSupportedResolutions);
	parametersLayout->addLayout(parametersForm);
	auto *parametersButtons = new QHBoxLayout;
	parametersButtons->addWidget(parametersApply);
	parametersButtons->addWidget(parametersRefresh);
	parametersButtons->addStretch();
	parametersLayout->addLayout(parametersButtons);
	layout->addLayout(parametersLayout);
	layout->addWidget(angles);
	layout->addLayout(grid);
	connect(modeRefresh, &QPushButton::clicked, this, &OBSBasicFalconMControl::QueryModes);
	connect(parametersRefresh, &QPushButton::clicked, this, &OBSBasicFalconMControl::QueryCaptureParameters);
	connect(parametersApply, &QPushButton::clicked, this, &OBSBasicFalconMControl::ApplyCaptureParameters);
	connect(modeSelector, qOverload<int>(&QComboBox::currentIndexChanged), this,
		&OBSBasicFalconMControl::SelectMode);

	poller = new QTimer(this);
	connect(poller, &QTimer::timeout, this, &OBSBasicFalconMControl::Refresh);
	poller->start(500);
	modeTimeout = new QTimer(this);
	modeTimeout->setSingleShot(true);
	modeTimeout->setInterval(5000);
	connect(modeTimeout, &QTimer::timeout, this, [this] {
		if (waitingForModeResult) {
			RestoreConfirmedMode(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeTimeout"));
		}
	});
	parametersTimeout = new QTimer(this);
	parametersTimeout->setSingleShot(true);
	parametersTimeout->setInterval(5000);
	connect(parametersTimeout, &QTimer::timeout, this, [this] {
		parametersRefresh->setEnabled(obs_source_active(source));
		parametersApply->setEnabled(false);
		parametersStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureParametersQueryFailed"));
	});
	Refresh();
}

OBSBasicFalconMControl::~OBSBasicFalconMControl()
{
	if (source) {
		obs_source_release(source);
	}
}

void OBSBasicFalconMControl::Send(int direction, int operation)
{
	calldata_t cd;
	calldata_init(&cd);
	calldata_set_int(&cd, "direction", direction);
	calldata_set_int(&cd, "operation", operation);
	proc_handler_call(obs_source_get_proc_handler(source), "send_direction", &cd);
	calldata_free(&cd);
}

void OBSBasicFalconMControl::QueryModes()
{
	if (!source || !obs_source_active(source)) {
		modeSelector->setEnabled(false);
		modeRefresh->setEnabled(false);
		modeStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));
		return;
	}
	calldata_t state;
	calldata_init(&state);
	proc_handler_call(obs_source_get_proc_handler(source), "get_supported_modes", &state);
	long long sequence = 0;
	calldata_get_int(&state, "sequence", &sequence);
	calldata_free(&state);

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_int(&cd, "version", FalconMModeProtocolVersion);
	proc_handler_call(obs_source_get_proc_handler(source), "query_supported_modes", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (!success) {
		modeStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeQueryFailed"));
		return;
	}
	modeQuerySequence = static_cast<uint64_t>(sequence);
	waitingForModes = true;
	modeSelector->setEnabled(false);
	modeRefresh->setEnabled(false);
	modeStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeLoading"));
}

void OBSBasicFalconMControl::QueryCaptureParameters()
{
	if (!source || !obs_source_active(source)) {
		parametersRefresh->setEnabled(false);
		parametersApply->setEnabled(false);
		parametersStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));
		return;
	}
	calldata_t state;
	calldata_init(&state);
	proc_handler_call(obs_source_get_proc_handler(source), "get_capture_parameters", &state);
	long long sequence = 0;
	calldata_get_int(&state, "sequence", &sequence);
	calldata_free(&state);
	parametersQuerySequence = static_cast<uint64_t>(sequence);

	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "query_capture_parameters", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (!success) {
		parametersTimeout->stop();
		parametersRefresh->setEnabled(obs_source_active(source));
		parametersStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureParametersQueryFailed"));
		return;
	}
	parametersRefresh->setEnabled(false);
	parametersApply->setEnabled(false);
	parametersTimeout->start();
	parametersStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureParametersLoading"));
}

void OBSBasicFalconMControl::ApplyCaptureParameters()
{
	if (!source || !obs_source_active(source)) {
		parametersApply->setEnabled(false);
		parametersStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));
		return;
	}
	calldata_t cd;
	calldata_init(&cd);
	calldata_set_bool(&cd, "auto_tracking", parametersAutoTracking->isChecked());
	proc_handler_call(obs_source_get_proc_handler(source), "set_capture_auto_tracking", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	parametersStatus->setText(
		QTStr(success ? "Basic.MainMenu.XBotGo.DeviceManagement.CaptureParametersApplied"
			      : "Basic.MainMenu.XBotGo.DeviceManagement.CaptureParametersApplyFailed"));
}

void OBSBasicFalconMControl::UpdateCaptureParameters()
{
	if (!source || !obs_source_active(source)) {
		return;
	}
	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "get_capture_parameters", &cd);
	long long sequence = 0, mode = 0, resolutionId = 0, angleRange = 0, accelSpeed = 0;
	long long countdown = 0, flicker = 0, resolutionCount = 0;
	bool watermark = false, mute = false, autoZoom = false, autoTracking = false;
	bool hasCountdown = false, hasFlicker = false;
	const char *resolution = nullptr;
	calldata_get_int(&cd, "sequence", &sequence);
	calldata_get_int(&cd, "mode", &mode);
	calldata_get_bool(&cd, "watermark", &watermark);
	calldata_get_bool(&cd, "mute", &mute);
	calldata_get_int(&cd, "resolution_id", &resolutionId);
	calldata_get_string(&cd, "resolution", &resolution);
	calldata_get_bool(&cd, "auto_zoom", &autoZoom);
	calldata_get_bool(&cd, "auto_tracking", &autoTracking);
	calldata_get_int(&cd, "angle_range", &angleRange);
	calldata_get_int(&cd, "accel_speed", &accelSpeed);
	calldata_get_bool(&cd, "has_countdown", &hasCountdown);
	calldata_get_int(&cd, "countdown", &countdown);
	calldata_get_bool(&cd, "has_flicker", &hasFlicker);
	calldata_get_int(&cd, "flicker", &flicker);
	calldata_get_int(&cd, "supported_resolution_count", &resolutionCount);
	if (sequence <= 0 || static_cast<uint64_t>(sequence) <= parametersQuerySequence ||
	    static_cast<uint64_t>(sequence) == displayedParametersSequence) {
		calldata_free(&cd);
		return;
	}

	const QSignalBlocker auto_tracking_blocker(parametersAutoTracking);
	QStringList supported;
	for (long long index = 0; index < resolutionCount; ++index) {
		calldata_t item;
		calldata_init(&item);
		calldata_set_int(&item, "index", index);
		proc_handler_call(obs_source_get_proc_handler(source), "get_capture_supported_resolution", &item);
		long long itemId = 0;
		const char *itemValue = nullptr;
		if (calldata_get_int(&item, "resolution_id", &itemId) &&
		    calldata_get_string(&item, "resolution", &itemValue)) {
			const QString value = QString::fromUtf8(itemValue);
			supported << QStringLiteral("%1: %2").arg(itemId).arg(value);
		}
		calldata_free(&item);
	}
	parametersMode->setText(QString::number(mode));
	parametersResolution->setText(QString::fromUtf8(resolution ? resolution : ""));
	parametersResolutionId->setText(QString::number(resolutionId));
	parametersWatermark->setText(watermark ? QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Yes")
					       : QTStr("Basic.MainMenu.XBotGo.DeviceManagement.No"));
	parametersMute->setText(mute ? QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Yes")
				     : QTStr("Basic.MainMenu.XBotGo.DeviceManagement.No"));
	parametersAutoZoom->setText(autoZoom ? QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Yes")
					     : QTStr("Basic.MainMenu.XBotGo.DeviceManagement.No"));
	parametersAutoTracking->setChecked(autoTracking);
	parametersAngleRange->setText(QString::number(angleRange));
	parametersAccelSpeed->setText(QString::number(accelSpeed));
	parametersCountdown->setText(hasCountdown ? QString::number(countdown) : QStringLiteral("N/A"));
	parametersFlicker->setText(hasFlicker ? QString::number(flicker) : QStringLiteral("N/A"));
	parametersSupportedResolutions->setText(supported.isEmpty() ? QStringLiteral("N/A")
								    : supported.join(QStringLiteral(", ")));
	parametersStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureParametersReady"));
	parametersRefresh->setEnabled(true);
	parametersApply->setEnabled(true);
	parametersTimeout->stop();
	displayedParametersSequence = static_cast<uint64_t>(sequence);
	calldata_free(&cd);
}

void OBSBasicFalconMControl::UpdateModes()
{
	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "get_supported_modes", &cd);
	long long sequence = 0, current = 0, count = 0;
	calldata_get_int(&cd, "sequence", &sequence);
	calldata_get_int(&cd, "current_mode", &current);
	calldata_get_int(&cd, "count", &count);
	calldata_free(&cd);
	if (sequence <= 0 || (waitingForModes && static_cast<uint64_t>(sequence) <= modeQuerySequence) ||
	    static_cast<uint64_t>(sequence) == displayedModesSequence) {
		return;
	}

	const QSignalBlocker blocker(modeSelector);
	modeSelector->clear();
	for (long long index = 0; index < count; ++index) {
		calldata_t item;
		calldata_init(&item);
		calldata_set_int(&item, "index", index);
		proc_handler_call(obs_source_get_proc_handler(source), "get_supported_mode", &item);
		long long mode = 0;
		bool beta = false;
		if (calldata_get_int(&item, "mode", &mode) &&
		    xbotgo::falconm_is_basketball_mode(static_cast<uint16_t>(mode))) {
			calldata_get_bool(&item, "beta", &beta);
			modeSelector->addItem(ModeLabel(static_cast<uint16_t>(mode), beta), mode);
		}
		calldata_free(&item);
	}
	const int currentIndex = modeSelector->findData(current);
	modeSelector->setCurrentIndex(currentIndex);
	confirmedMode = currentIndex >= 0 ? static_cast<int>(current) : -1;
	displayedModesSequence = static_cast<uint64_t>(sequence);
	waitingForModes = false;
	modeSelector->setEnabled(!waitingForModeResult && modeSelector->count() > 0);
	modeRefresh->setEnabled(!waitingForModeResult);
	modeStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeReady"));
}

void OBSBasicFalconMControl::SelectMode(int index)
{
	if (index < 0 || waitingForModes || waitingForModeResult || !source || !obs_source_active(source)) {
		return;
	}
	const int mode = modeSelector->itemData(index).toInt();
	if (mode == confirmedMode) {
		return;
	}
	calldata_t state;
	calldata_init(&state);
	proc_handler_call(obs_source_get_proc_handler(source), "get_capture_mode_result", &state);
	long long sequence = 0;
	calldata_get_int(&state, "sequence", &sequence);
	calldata_free(&state);

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_int(&cd, "mode", mode);
	proc_handler_call(obs_source_get_proc_handler(source), "set_capture_mode", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (!success) {
		RestoreConfirmedMode(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeSetFailed"));
		return;
	}
	modeResultSequence = static_cast<uint64_t>(sequence);
	waitingForModeResult = true;
	modeSelector->setEnabled(false);
	modeRefresh->setEnabled(false);
	modeStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeApplying"));
	modeTimeout->start();
}

void OBSBasicFalconMControl::HandleModeResult()
{
	if (!waitingForModeResult) {
		return;
	}
	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "get_capture_mode_result", &cd);
	long long sequence = 0;
	bool success = false;
	calldata_get_int(&cd, "sequence", &sequence);
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (static_cast<uint64_t>(sequence) <= modeResultSequence) {
		return;
	}
	waitingForModeResult = false;
	modeTimeout->stop();
	if (success) {
		modeStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeApplied"));
		QueryModes();
	} else {
		RestoreConfirmedMode(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ModeSetFailed"));
	}
}

void OBSBasicFalconMControl::RestoreConfirmedMode(const QString &status)
{
	waitingForModeResult = false;
	modeTimeout->stop();
	const QSignalBlocker blocker(modeSelector);
	modeSelector->setCurrentIndex(modeSelector->findData(confirmedMode));
	modeSelector->setEnabled(obs_source_active(source) && modeSelector->count() > 0);
	modeRefresh->setEnabled(obs_source_active(source));
	modeStatus->setText(status);
}

void OBSBasicFalconMControl::Refresh()
{
	if (!source) {
		return;
	}
	const bool active = obs_source_active(source);
	connection->setText(QTStr(active ? "Basic.MainMenu.XBotGo.DeviceManagement.Active"
					 : "Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));
	if (active) {
		if (!sourceWasActive) {
			sourceWasActive = true;
			QueryModes();
			QueryCaptureParameters();
		}
		UpdateModes();
		HandleModeResult();
		UpdateCaptureParameters();
	} else {
		sourceWasActive = false;
		if (!waitingForModeResult) {
			modeSelector->setEnabled(false);
			modeRefresh->setEnabled(false);
		}
		parametersRefresh->setEnabled(false);
		parametersApply->setEnabled(false);
		parametersTimeout->stop();
		parametersStatus->setText(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));
	}
	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "get_motor_angle", &cd);
	long long result = 0, hlimit = 0, vlimit = 0;
	double horizontal = 0, vertical = 0;
	calldata_get_int(&cd, "result", &result);
	calldata_get_float(&cd, "horizontal", &horizontal);
	calldata_get_float(&cd, "vertical", &vertical);
	calldata_get_int(&cd, "horizontal_limit", &hlimit);
	calldata_get_int(&cd, "vertical_limit", &vlimit);
	angles->setText(QStringLiteral("H: %1  V: %2  (%3/%4)")
				.arg(horizontal, 0, 'f', 2)
				.arg(vertical, 0, 'f', 2)
				.arg(hlimit)
				.arg(vlimit));
	calldata_free(&cd);
}
