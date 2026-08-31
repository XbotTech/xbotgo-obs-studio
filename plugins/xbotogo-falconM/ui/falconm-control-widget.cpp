#include "falconm-control-widget.hpp"

#include "xbotgo-combo-box-control.hpp"
#include "xbotgo-slider-control.hpp"
#include "../runtime/xbotgo-translation.hpp"
#include "../protocol/falcon-calibration.hpp"
#include "../protocol/falcon-camera-requests.hpp"
#include "../protocol/falcon-mode-requests.hpp"
#include "../scenes/camera-role-scenes.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

namespace xbotgo {
namespace {
constexpr int FalconMModeProtocolVersion = 3;

int CameraRoleControlIndex(const std::optional<CameraRole> &role)
{
	if (!role) {
		return -1;
	}
	return static_cast<int>(*role);
}

QString ModeLabel(uint16_t mode, bool beta)
{
	QString label;
	switch (static_cast<xbotgo::ModeType>(mode)) {
	case xbotgo::ModeType::Soccer5v5Over14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.1");
		break;
	case xbotgo::ModeType::Soccer5v5Under14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.2");
		break;
	case xbotgo::ModeType::Soccer7v7Over14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.3");
		break;
	case xbotgo::ModeType::Soccer7v7Under14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.4");
		break;
	case xbotgo::ModeType::BasketballWholeOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.5");
		break;
	case xbotgo::ModeType::BasketballWholeUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.6");
		break;
	case xbotgo::ModeType::BasketballHalfOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.7");
		break;
	case xbotgo::ModeType::BasketballHalfUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.8");
		break;
	case xbotgo::ModeType::Soccer11v11Over14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.11");
		break;
	case xbotgo::ModeType::Soccer11v11Under14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.12");
		break;
	case xbotgo::ModeType::RugbyWholeOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.13");
		break;
	case xbotgo::ModeType::RugbyWholeUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.14");
		break;
	case xbotgo::ModeType::LacrosseWholeOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.15");
		break;
	case xbotgo::ModeType::LacrosseWholeUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.16");
		break;
	case xbotgo::ModeType::IceHockeyWholeOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.17");
		break;
	case xbotgo::ModeType::IceHockeyWholeUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.18");
		break;
	case xbotgo::ModeType::WheelchairSoccer:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.19");
		break;
	case xbotgo::ModeType::FollowMe:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.20");
		break;
	case xbotgo::ModeType::TennisDouble:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.23");
		break;
	case xbotgo::ModeType::TennisSingle:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.24");
		break;
	case xbotgo::ModeType::HandballWholeOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.25");
		break;
	case xbotgo::ModeType::HandballWholeUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.26");
		break;
	case xbotgo::ModeType::HandballHalfOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.27");
		break;
	case xbotgo::ModeType::HandballHalfUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.28");
		break;
	case xbotgo::ModeType::BroomballWholeOver14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.29");
		break;
	case xbotgo::ModeType::BroomballWholeUnder14:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.30");
		break;
	case xbotgo::ModeType::PickleballDouble:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.31");
		break;
	case xbotgo::ModeType::PickleballSingle:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.32");
		break;
	case xbotgo::ModeType::BadmintonDouble:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.33");
		break;
	case xbotgo::ModeType::BadmintonSingle:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.34");
		break;
	case xbotgo::ModeType::BasketballWholeOver14High:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.36");
		break;
	case xbotgo::ModeType::BasketballWholeUnder14High:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.37");
		break;
	case xbotgo::ModeType::BasketballHalfOver14High:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.38");
		break;
	case xbotgo::ModeType::BasketballHalfUnder14High:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.39");
		break;
	case xbotgo::ModeType::Volleyball:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.40");
		break;
	case xbotgo::ModeType::KeyPlayerHalf:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.41");
		break;
	case xbotgo::ModeType::KeyPlayerFull:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.42");
		break;
	case xbotgo::ModeType::KeyPlayerHalfHigh:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.43");
		break;
	case xbotgo::ModeType::KeyPlayerFullHigh:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.44");
		break;
	case xbotgo::ModeType::AmericanFootballCloseHigh:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.45");
		break;
	case xbotgo::ModeType::AmericanFootballMediumHigh:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.46");
		break;
	case xbotgo::ModeType::AmericanFootballFarHigh:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.47");
		break;
	case xbotgo::ModeType::RugbyHigh:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.48");
		break;
	case xbotgo::ModeType::FlagFootballHigh:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.49");
		break;
	case xbotgo::ModeType::Baseball:
		label = Tr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.50");
		break;
	default:
		label = QStringLiteral("Mode %1").arg(mode);
		break;
	}
	return beta ? QStringLiteral("%1 (Beta)").arg(label) : label;
}
} // namespace

FalconMControlWidget::FalconMControlWidget(obs_source_t *source, QWidget *parent) : QWidget(parent), source_(source)
{
	cameraRoleControl = new ComboBoxControl(this);
	cameraRoleControl->setTitle(Tr("Basic.MainMenu.XBotGo.DeviceManagement.CameraRole"));
	for (const CameraRole role : {CameraRole::Center, CameraRole::Left, CameraRole::Right}) {
		cameraRoleControl->addItem(QString::fromLatin1(CameraRoleSceneName(role)), static_cast<int>(role));
	}
	OBSSource sourceRef = source_.lock();
	cameraRoleControl->setCurrentIndex(CameraRoleControlIndex(GetSourceCameraRole(sourceRef)));
	connect(cameraRoleControl, &ComboBoxControl::currentIndexChanged, this, [this](int index) {
		if (index < 0) {
			return;
		}
		OBSSource sourceRef = source_.lock();
		const int previousIndex = CameraRoleControlIndex(GetSourceCameraRole(sourceRef));
		const auto role = static_cast<CameraRole>(cameraRoleControl->currentData().toInt());
		if (!sourceRef || !AssignSourceToCameraRoleScene(sourceRef, role)) {
			const QSignalBlocker blocker(cameraRoleControl);
			cameraRoleControl->setCurrentIndex(previousIndex);
		}
	});

	modeSelector = new QComboBox(this);
	parametersAutoZoom = new QCheckBox(this);
	parametersAutoZoom->setEnabled(false);
	parametersAutoTracking = new QCheckBox(this);
	parametersAutoTracking->setEnabled(false);
	parametersAngleRange = new SliderControl(this);
	parametersAngleRange->setTitle(Tr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAngleRange"));
	parametersAngleRange->setRange(60, 150);
	parametersAngleRange->setSingleStep(1);
	parametersAngleRange->setValueFormatter([](int value) { return QStringLiteral("%1°").arg(value); });
	parametersAngleRange->setEnabled(false);
	manualZoomSlider = new SliderControl(this);
	manualZoomSlider->setTitle(Tr("Basic.MainMenu.XBotGo.DeviceManagement.ManualZoom"));
	manualZoomSlider->setRange(10, 30);
	manualZoomSlider->setSingleStep(1);
	manualZoomSlider->setValueFormatter(
		[](int value) { return QStringLiteral("%1x").arg(value / 10.0, 0, 'f', 1); });
	manualZoomSlider->setEnabled(false);

	angles = new QLabel(this);
	buzzerLongButton = new QPushButton(Tr("Basic.MainMenu.XBotGo.DeviceManagement.LongBeep"), this);
	hallCalibrationStart = new QPushButton(Tr("Basic.MainMenu.XBotGo.DeviceManagement.HallCalibrationStart"), this);
	hallCalibrationStart->setEnabled(false);
	auto *grid = new QGridLayout;
	const auto addButton = [this, grid](const QString &label, int row, int col, int direction) {
		auto *button = new QPushButton(label, this);
		directionButtons.emplace_back(button);
		grid->addWidget(button, row, col);
		connect(button, &QPushButton::pressed, this, [this, direction] { Send(direction, 1); });
		connect(button, &QPushButton::released, this, [this, direction] { Send(direction, 2); });
	};
	addButton(Tr("Basic.MainMenu.XBotGo.DeviceManagement.Up"), 0, 1, 0);
	addButton(Tr("Basic.MainMenu.XBotGo.DeviceManagement.Left"), 1, 0, 2);
	addButton(Tr("Basic.MainMenu.XBotGo.DeviceManagement.Center"), 1, 1, 4);
	addButton(Tr("Basic.MainMenu.XBotGo.DeviceManagement.Right"), 1, 2, 3);
	addButton(Tr("Basic.MainMenu.XBotGo.DeviceManagement.Down"), 2, 1, 1);

	auto *rootLayout = new QHBoxLayout(this);
	auto *leftLayout = new QVBoxLayout;
	auto *rightLayout = new QVBoxLayout;
	rootLayout->addLayout(leftLayout, 1);
	rootLayout->addLayout(rightLayout, 1);
	rightLayout->addWidget(cameraRoleControl);
	auto *zoomLayout = new QFormLayout;
	zoomLayout->addRow(Tr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAutoZoom"), parametersAutoZoom);
	zoomLayout->addRow(manualZoomSlider);
	rightLayout->addLayout(zoomLayout);
	rightLayout->addWidget(buzzerLongButton);
	rightLayout->addWidget(hallCalibrationStart);
	rightLayout->addStretch();

	auto *modeLayout = new QHBoxLayout;
	modeLayout->addWidget(new QLabel(Tr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureMode"), this));
	modeLayout->addWidget(modeSelector, 1);
	leftLayout->addLayout(modeLayout);
	auto *parametersForm = new QFormLayout;
	parametersForm->addRow(Tr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAutoTracking"),
			       parametersAutoTracking);
	parametersForm->addRow(parametersAngleRange);
	leftLayout->addLayout(parametersForm);
	leftLayout->addWidget(angles);
	leftLayout->addLayout(grid);
	connect(parametersAutoZoom, &QCheckBox::toggled, this, &FalconMControlWidget::ApplyAutoZoom);
	connect(parametersAutoTracking, &QCheckBox::toggled, this, &FalconMControlWidget::ApplyAutoTracking);
	connect(parametersAngleRange, &SliderControl::sliderReleased, this, &FalconMControlWidget::ApplyAngleRange);
	connect(modeSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, &FalconMControlWidget::SelectMode);
	connect(buzzerLongButton, &QPushButton::clicked, this, [this] { SendBuzzerMode(BuzzerMode::Beep3000Ms); });
	connect(hallCalibrationStart, &QPushButton::clicked, this, &FalconMControlWidget::StartHallCalibration);
	connect(manualZoomSlider, &SliderControl::valueChanged, this, &FalconMControlWidget::ManualZoomValueChanged);
	connect(manualZoomSlider, &SliderControl::sliderPressed, this, [this] {
		manualZoomDragging = true;
		manualZoomCommandValue = manualZoomSlider->value();
	});
	connect(manualZoomSlider, &SliderControl::sliderReleased, this, [this] {
		manualZoomDragging = false;
		manualZoomQueryDebounce->stop();
		QueryCurrentZoom();
	});

	auto *poller = new QTimer(this);
	connect(poller, &QTimer::timeout, this, &FalconMControlWidget::Refresh);
	poller->start(500);
	modeTimeout = new QTimer(this);
	modeTimeout->setSingleShot(true);
	modeTimeout->setInterval(5000);
	connect(modeTimeout, &QTimer::timeout, this, [this] {
		if (waitingForModeResult) {
			RestoreConfirmedMode();
		}
	});
	hallCalibrationTimeout = new QTimer(this);
	hallCalibrationTimeout->setSingleShot(true);
	hallCalibrationTimeout->setInterval(5000);
	connect(hallCalibrationTimeout, &QTimer::timeout, this, [this] {
		const bool calibrating = currentHallCalibrationStatus ==
					 static_cast<int>(falconm_hall_calibration_status::calibrating);
		hallCalibrationStart->setEnabled(source_.connected() && !calibrating);
	});
	manualZoomQueryDebounce = new QTimer(this);
	manualZoomQueryDebounce->setSingleShot(true);
	manualZoomQueryDebounce->setInterval(250);
	connect(manualZoomQueryDebounce, &QTimer::timeout, this, &FalconMControlWidget::QueryCurrentZoom);
	Refresh();
}

void FalconMControlWidget::Send(int direction, int operation)
{
	if (direction < 0 || direction > 4 || operation < 0 || operation > 2) {
		return;
	}
	source_.send(SendDirectionRequest{static_cast<falconm_direction>(direction),
					  static_cast<falconm_operation>(operation)});
}

void FalconMControlWidget::SendBuzzerMode(BuzzerMode mode)
{
	source_.send(SetBuzzerModeRequest{mode});
}

void FalconMControlWidget::QueryHallCalibration()
{
	if (!source_.connected()) {
		hallCalibrationStart->setEnabled(false);
		return;
	}
	hallCalibrationQuerySequence = source_.state().hall_calibration_sequence;
	if (!source_.send(QueryHallCalibrationRequest{})) {
		hallCalibrationStart->setEnabled(true);
		return;
	}
	hallCalibrationStart->setEnabled(false);
	hallCalibrationTimeout->start();
}

void FalconMControlWidget::StartHallCalibration()
{
	if (!source_.connected()) {
		return;
	}
	hallCalibrationQuerySequence = source_.state().hall_calibration_sequence;
	if (!source_.send(StartHallCalibrationRequest{})) {
		return;
	}
	hallCalibrationStart->setEnabled(false);
	hallCalibrationTimeout->start();
}

void FalconMControlWidget::UpdateHallCalibration(const falconm_device_state &state)
{
	if (state.hall_calibration_sequence <= hallCalibrationQuerySequence ||
	    state.hall_calibration_sequence == displayedHallCalibrationSequence) {
		return;
	}
	currentHallCalibrationStatus = static_cast<int>(state.hall_calibration_status);
	displayedHallCalibrationSequence = state.hall_calibration_sequence;
	hallCalibrationTimeout->stop();
	hallCalibrationStart->setEnabled(state.hall_calibration_status != falconm_hall_calibration_status::calibrating);
}

void FalconMControlWidget::QueryCurrentZoom()
{
	if (!source_.connected()) {
		manualZoomSlider->setEnabled(false);
		return;
	}
	manualZoomQuerySequence = source_.state().current_zoom_sequence;
	source_.send(QueryCurrentZoomRequest{});
	UpdateManualZoomEnabled();
}

void FalconMControlWidget::UpdateCurrentZoom(const falconm_device_state &state)
{
	if (state.current_zoom < 10 || state.current_zoom > 30 ||
	    state.current_zoom_sequence <= manualZoomQuerySequence ||
	    state.current_zoom_sequence == displayedManualZoomSequence) {
		return;
	}
	hasCurrentManualZoom = true;
	displayedManualZoomSequence = state.current_zoom_sequence;
	if (!manualZoomDragging) {
		const QSignalBlocker blocker(manualZoomSlider);
		manualZoomSlider->setValue(state.current_zoom);
		manualZoomCommandValue = state.current_zoom;
	}
	UpdateManualZoomEnabled();
}

bool FalconMControlWidget::SendCaptureParameters(bool autoZoom, bool autoTracking, int angleRange)
{
	if (!source_.connected() || !hasConfirmedCaptureParameters || angleRange < 60 || angleRange > 150) {
		return false;
	}
	auto parameters = source_.state().capture_parameters;
	parameters.auto_zoom = autoZoom;
	parameters.auto_tracking = autoTracking;
	parameters.angle_range = static_cast<uint16_t>(angleRange);
	return source_.send(SetCaptureParametersRequest{std::move(parameters)});
}

bool FalconMControlWidget::DisableAutoZoomForManualControl()
{
	if (!parametersAutoZoom->isChecked()) {
		return true;
	}
	if (!SendCaptureParameters(false, parametersAutoTracking->isChecked(), parametersAngleRange->value())) {
		return false;
	}
	const QSignalBlocker blocker(parametersAutoZoom);
	parametersAutoZoom->setChecked(false);
	confirmedAutoZoom = false;
	confirmedAutoTracking = parametersAutoTracking->isChecked();
	confirmedAngleRange = parametersAngleRange->value();
	UpdateManualZoomEnabled();
	return true;
}

void FalconMControlWidget::ManualZoomValueChanged(int value)
{
	if (!source_.connected() || !hasCurrentManualZoom || !hasConfirmedCaptureParameters || value < 10 ||
	    value > 30 || value == manualZoomCommandValue) {
		return;
	}
	if (!DisableAutoZoomForManualControl()) {
		const QSignalBlocker blocker(manualZoomSlider);
		manualZoomSlider->setValue(manualZoomCommandValue);
		return;
	}

	const int direction = value > manualZoomCommandValue ? 1 : -1;
	while (manualZoomCommandValue != value) {
		if (!source_.send(ManualZoomRequest{falconm_zoom_type::relative, static_cast<int8_t>(direction)})) {
			const QSignalBlocker blocker(manualZoomSlider);
			manualZoomSlider->setValue(manualZoomCommandValue);
			QueryCurrentZoom();
			return;
		}
		manualZoomCommandValue += direction;
	}
	if (!manualZoomDragging) {
		manualZoomQueryDebounce->start();
	}
}

void FalconMControlWidget::UpdateManualZoomEnabled()
{
	manualZoomSlider->setEnabled(source_.connected() && hasCurrentManualZoom && hasConfirmedCaptureParameters &&
				     !parametersAutoZoom->isChecked());
}

void FalconMControlWidget::QueryModes()
{
	if (!source_.connected()) {
		modeSelector->setEnabled(false);
		return;
	}
	modeQuerySequence = source_.state().supported_modes_sequence;
	if (!source_.send(QuerySupportedModesRequest{FalconMModeProtocolVersion})) {
		return;
	}
	waitingForModes = true;
	modeSelector->setEnabled(false);
}

void FalconMControlWidget::QueryCaptureParameters()
{
	if (!source_.connected()) {
		parametersAutoZoom->setEnabled(false);
		parametersAutoTracking->setEnabled(false);
		parametersAngleRange->setEnabled(false);
		return;
	}
	parametersQuerySequence = source_.state().capture_parameters_sequence;
	if (!source_.send(QueryCaptureParametersRequest{})) {
		return;
	}
	parametersAutoZoom->setEnabled(false);
	parametersAutoTracking->setEnabled(false);
	parametersAngleRange->setEnabled(false);
}

void FalconMControlWidget::ApplyAutoZoom(bool checked)
{
	if (!source_.connected() || !hasConfirmedCaptureParameters || checked == confirmedAutoZoom) {
		return;
	}
	if (SendCaptureParameters(checked, parametersAutoTracking->isChecked(), parametersAngleRange->value())) {
		confirmedAutoZoom = checked;
	} else {
		const QSignalBlocker blocker(parametersAutoZoom);
		parametersAutoZoom->setChecked(confirmedAutoZoom);
	}
	UpdateManualZoomEnabled();
}

void FalconMControlWidget::ApplyAutoTracking(bool checked)
{
	if (!source_.connected() || !hasConfirmedCaptureParameters || checked == confirmedAutoTracking) {
		return;
	}
	if (SendCaptureParameters(parametersAutoZoom->isChecked(), checked, parametersAngleRange->value())) {
		confirmedAutoTracking = checked;
	} else {
		const QSignalBlocker blocker(parametersAutoTracking);
		parametersAutoTracking->setChecked(confirmedAutoTracking);
	}
}

void FalconMControlWidget::ApplyAngleRange()
{
	const int angleRange = parametersAngleRange->value();
	if (!source_.connected() || !hasConfirmedCaptureParameters || angleRange == confirmedAngleRange) {
		return;
	}
	if (SendCaptureParameters(parametersAutoZoom->isChecked(), parametersAutoTracking->isChecked(), angleRange)) {
		confirmedAngleRange = angleRange;
	} else {
		const QSignalBlocker blocker(parametersAngleRange);
		parametersAngleRange->setValue(confirmedAngleRange);
	}
}

void FalconMControlWidget::UpdateCaptureParameters(const falconm_device_state &state)
{
	if (state.capture_parameters_sequence <= parametersQuerySequence ||
	    state.capture_parameters_sequence == displayedParametersSequence) {
		return;
	}
	const auto &parameters = state.capture_parameters;
	const QSignalBlocker autoZoomBlocker(parametersAutoZoom);
	const QSignalBlocker autoTrackingBlocker(parametersAutoTracking);
	const QSignalBlocker angleRangeBlocker(parametersAngleRange);
	parametersAutoZoom->setChecked(parameters.auto_zoom);
	parametersAutoTracking->setChecked(parameters.auto_tracking);
	parametersAngleRange->setValue(parameters.angle_range);
	confirmedAutoZoom = parameters.auto_zoom;
	confirmedAutoTracking = parameters.auto_tracking;
	confirmedAngleRange = parametersAngleRange->value();
	hasConfirmedCaptureParameters = true;
	UpdateManualZoomEnabled();
	parametersAutoZoom->setEnabled(true);
	parametersAutoTracking->setEnabled(true);
	parametersAngleRange->setEnabled(true);
	displayedParametersSequence = state.capture_parameters_sequence;
}

void FalconMControlWidget::UpdateModes(const falconm_device_state &state)
{
	if (state.supported_modes_sequence == 0 ||
	    (waitingForModes && state.supported_modes_sequence <= modeQuerySequence) ||
	    state.supported_modes_sequence == displayedModesSequence) {
		return;
	}
	const QSignalBlocker blocker(modeSelector);
	modeSelector->clear();
	for (const auto &mode : state.supported_modes.modes) {
		if (falconm_is_basketball_mode(static_cast<ModeType>(mode.mode))) {
			modeSelector->addItem(ModeLabel(mode.mode, mode.beta), mode.mode);
		}
	}
	const int currentIndex = modeSelector->findData(state.supported_modes.current_mode);
	modeSelector->setCurrentIndex(currentIndex);
	confirmedMode = currentIndex >= 0 ? state.supported_modes.current_mode : -1;
	displayedModesSequence = state.supported_modes_sequence;
	waitingForModes = false;
	modeSelector->setEnabled(!waitingForModeResult && modeSelector->count() > 0);
}

void FalconMControlWidget::SelectMode(int index)
{
	if (index < 0 || waitingForModes || waitingForModeResult || !source_.connected()) {
		return;
	}
	const int mode = modeSelector->itemData(index).toInt();
	if (mode == confirmedMode) {
		return;
	}
	modeResultSequence = source_.state().capture_mode_result.sequence;
	if (!source_.send(SetCaptureModeRequest{static_cast<ModeType>(static_cast<uint16_t>(mode))})) {
		RestoreConfirmedMode();
		return;
	}
	waitingForModeResult = true;
	modeSelector->setEnabled(false);
	modeTimeout->start();
}

void FalconMControlWidget::HandleModeResult(const falconm_device_state &state)
{
	if (!waitingForModeResult || state.capture_mode_result.sequence <= modeResultSequence) {
		return;
	}
	waitingForModeResult = false;
	modeTimeout->stop();
	if (state.capture_mode_result.success) {
		QueryModes();
	} else {
		RestoreConfirmedMode();
	}
}

void FalconMControlWidget::RestoreConfirmedMode()
{
	waitingForModeResult = false;
	modeTimeout->stop();
	const QSignalBlocker blocker(modeSelector);
	modeSelector->setCurrentIndex(modeSelector->findData(confirmedMode));
	modeSelector->setEnabled(source_.connected() && modeSelector->count() > 0);
}

void FalconMControlWidget::Refresh()
{
	if (!source_.valid()) {
		if (sourceWasConnected) {
			sourceWasConnected = false;
			emit connectionStateChanged(false);
		}
		return;
	}
	const bool connected = source_.connected();
	if (connected != sourceWasConnected) {
		emit connectionStateChanged(connected);
	}
	for (QPushButton *button : directionButtons) {
		button->setEnabled(connected);
	}
	buzzerLongButton->setEnabled(connected);
	if (connected) {
		if (!sourceWasConnected) {
			sourceWasConnected = true;
			QueryModes();
			QueryCaptureParameters();
			QueryHallCalibration();
			QueryCurrentZoom();
		}
		const falconm_device_state state = source_.state();
		UpdateModes(state);
		HandleModeResult(state);
		UpdateCaptureParameters(state);
		UpdateHallCalibration(state);
		UpdateCurrentZoom(state);
	} else {
		sourceWasConnected = false;
		if (!waitingForModeResult) {
			modeSelector->setEnabled(false);
		}
		parametersAutoZoom->setEnabled(false);
		parametersAutoTracking->setEnabled(false);
		parametersAngleRange->setEnabled(false);
		hallCalibrationStart->setEnabled(false);
		hallCalibrationTimeout->stop();
		hasCurrentManualZoom = false;
		manualZoomDragging = false;
		manualZoomSlider->setEnabled(false);
		manualZoomQueryDebounce->stop();
	}

	const auto angle = source_.state().motor_angle;
	angles->setText(QStringLiteral("H: %1  V: %2  (%3/%4)")
				.arg(angle.horizontal / 100.0, 0, 'f', 2)
				.arg(angle.vertical / 100.0, 0, 'f', 2)
				.arg(angle.horizontal_limit)
				.arg(angle.vertical_limit));
}

} // namespace xbotgo
