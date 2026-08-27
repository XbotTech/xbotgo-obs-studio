#include "OBSBasicFalconMControl.hpp"

#include <OBSApp.hpp>
#include <widgets/OBSBasic.hpp>
#include "moc_OBSBasicFalconMControl.cpp"

#include <obs.h>
#include <callback/calldata.h>
#include <qt-wrappers.hpp>
#include <xbotgo/components/XBotGoComboBoxControl.hpp>
#include <xbotgo/components/XBotGoSliderControl.hpp>
#include <xbotgo/scenes/XBotGoCameraRoleScenes.hpp>
#include <xbotgo/sources/XBotGoFalconMSource.hpp>

#include "../../plugins/xbotogo-falconM/falconm-protocol.hpp"

#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int FalconMModeProtocolVersion = 3;

int CameraRoleControlIndex(const std::optional<xbotgo::CameraRole> &role)
{
	if (!role) {
		return -1;
	}

	switch (*role) {
	case xbotgo::CameraRole::Center:
		return 0;
	case xbotgo::CameraRole::Left:
		return 1;
	case xbotgo::CameraRole::Right:
		return 2;
	}
	return -1;
}

QString ModeLabel(uint16_t mode, bool beta)
{
	QString label;
	switch (static_cast<xbotgo::ModeType>(mode)) {
	case xbotgo::ModeType::Soccer5v5Over14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.1");
		break;
	case xbotgo::ModeType::Soccer5v5Under14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.2");
		break;
	case xbotgo::ModeType::Soccer7v7Over14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.3");
		break;
	case xbotgo::ModeType::Soccer7v7Under14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.4");
		break;
	case xbotgo::ModeType::BasketballWholeOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.5");
		break;
	case xbotgo::ModeType::BasketballWholeUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.6");
		break;
	case xbotgo::ModeType::BasketballHalfOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.7");
		break;
	case xbotgo::ModeType::BasketballHalfUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.8");
		break;
	case xbotgo::ModeType::Soccer11v11Over14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.11");
		break;
	case xbotgo::ModeType::Soccer11v11Under14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.12");
		break;
	case xbotgo::ModeType::RugbyWholeOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.13");
		break;
	case xbotgo::ModeType::RugbyWholeUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.14");
		break;
	case xbotgo::ModeType::LacrosseWholeOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.15");
		break;
	case xbotgo::ModeType::LacrosseWholeUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.16");
		break;
	case xbotgo::ModeType::IceHockeyWholeOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.17");
		break;
	case xbotgo::ModeType::IceHockeyWholeUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.18");
		break;
	case xbotgo::ModeType::WheelchairSoccer:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.19");
		break;
	case xbotgo::ModeType::FollowMe:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.20");
		break;
	case xbotgo::ModeType::TennisDouble:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.23");
		break;
	case xbotgo::ModeType::TennisSingle:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.24");
		break;
	case xbotgo::ModeType::HandballWholeOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.25");
		break;
	case xbotgo::ModeType::HandballWholeUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.26");
		break;
	case xbotgo::ModeType::HandballHalfOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.27");
		break;
	case xbotgo::ModeType::HandballHalfUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.28");
		break;
	case xbotgo::ModeType::BroomballWholeOver14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.29");
		break;
	case xbotgo::ModeType::BroomballWholeUnder14:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.30");
		break;
	case xbotgo::ModeType::PickleballDouble:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.31");
		break;
	case xbotgo::ModeType::PickleballSingle:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.32");
		break;
	case xbotgo::ModeType::BadmintonDouble:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.33");
		break;
	case xbotgo::ModeType::BadmintonSingle:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.34");
		break;
	case xbotgo::ModeType::BasketballWholeOver14High:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.36");
		break;
	case xbotgo::ModeType::BasketballWholeUnder14High:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.37");
		break;
	case xbotgo::ModeType::BasketballHalfOver14High:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.38");
		break;
	case xbotgo::ModeType::BasketballHalfUnder14High:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.39");
		break;
	case xbotgo::ModeType::Volleyball:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.40");
		break;
	case xbotgo::ModeType::KeyPlayerHalf:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.41");
		break;
	case xbotgo::ModeType::KeyPlayerFull:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.42");
		break;
	case xbotgo::ModeType::KeyPlayerHalfHigh:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.43");
		break;
	case xbotgo::ModeType::KeyPlayerFullHigh:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.44");
		break;
	case xbotgo::ModeType::AmericanFootballCloseHigh:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.45");
		break;
	case xbotgo::ModeType::AmericanFootballMediumHigh:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.46");
		break;
	case xbotgo::ModeType::AmericanFootballFarHigh:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.47");
		break;
	case xbotgo::ModeType::RugbyHigh:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.48");
		break;
	case xbotgo::ModeType::FlagFootballHigh:
		label = QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Mode.49");
		break;
	case xbotgo::ModeType::Baseball:
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
	resize(500, 300);

	OBSBasic *main = OBSBasic::Get();
	cameraRoleControl = new xbotgo::ComboBoxControl(this);
	cameraRoleControl->setTitle(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CameraRole"));
	for (const xbotgo::CameraRole role :
	     {xbotgo::CameraRole::Center, xbotgo::CameraRole::Left, xbotgo::CameraRole::Right}) {
		cameraRoleControl->addItem(QString::fromLatin1(xbotgo::CameraRoleSceneName(role)),
					   static_cast<int>(role));
	}
	cameraRoleControl->setCurrentIndex(
	CameraRoleControlIndex(main ? xbotgo::GetSourceCameraRole(*main, source) : std::nullopt));
	connect(cameraRoleControl, &xbotgo::ComboBoxControl::currentIndexChanged, this, [this](int index) {
		if (index < 0) {
			return;
		}

		OBSBasic *main = OBSBasic::Get();
		const int previousIndex = CameraRoleControlIndex(
			main ? xbotgo::GetSourceCameraRole(*main, source) : std::nullopt);
		const auto role = static_cast<xbotgo::CameraRole>(cameraRoleControl->currentData().toInt());
		if (!main || !xbotgo::AssignSourceToCameraRoleScene(*main, source, role)) {
			const QSignalBlocker blocker(cameraRoleControl);
			cameraRoleControl->setCurrentIndex(previousIndex);
		}
	});


	modeSelector = new QComboBox(this);
	parametersAutoZoom = new QCheckBox(this);
	parametersAutoZoom->setEnabled(false);
	parametersAutoTracking = new QCheckBox(this);
	parametersAutoTracking->setEnabled(false);
	parametersAngleRange = new xbotgo::SliderControl(this);
	parametersAngleRange->setTitle(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAngleRange"));
	parametersAngleRange->setRange(60, 150);
	parametersAngleRange->setSingleStep(1);
	parametersAngleRange->setValueFormatter([](int value) { return QStringLiteral("%1°").arg(value); });
	parametersAngleRange->setEnabled(false);
	manualZoomSlider = new xbotgo::SliderControl(this);
	manualZoomSlider->setTitle(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ManualZoom"));
	manualZoomSlider->setRange(10, 30);
	manualZoomSlider->setSingleStep(1);
	manualZoomSlider->setValueFormatter(
		[](int value) { return QStringLiteral("%1x").arg(value / 10.0, 0, 'f', 1); });
	manualZoomSlider->setEnabled(false);

	angles = new QLabel(this);
	buzzerLongButton = new QPushButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.LongBeep"), this);
	hallCalibrationStart =
		new QPushButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.HallCalibrationStart"), this);
	hallCalibrationStart->setEnabled(false);
	auto *grid = new QGridLayout;
	const auto addButton = [this, grid](const QString &label, int row, int col, int direction) {
		auto *button = new QPushButton(label, this);
		directionButtons.emplace_back(button);
		grid->addWidget(button, row, col);
		connect(button, &QPushButton::pressed, this, [this, direction] { Send(direction, 1); });
		connect(button, &QPushButton::released, this, [this, direction] { Send(direction, 2); });
	};
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Up"), 0, 1, 0);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Left"), 1, 0, 2);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Center"), 1, 1, 4);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Right"), 1, 2, 3);
	addButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Down"), 2, 1, 1);

	auto *rootLayout = new QHBoxLayout(this);
	auto *leftLayout = new QVBoxLayout;
	auto *rightLayout = new QVBoxLayout;
	rootLayout->addLayout(leftLayout, 1);
	rootLayout->addLayout(rightLayout, 1);
	rightLayout->addWidget(cameraRoleControl);
	auto *zoomLayout = new QFormLayout;
	zoomLayout->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAutoZoom"), parametersAutoZoom);
	zoomLayout->addRow(manualZoomSlider);
	rightLayout->addLayout(zoomLayout);
	rightLayout->addWidget(buzzerLongButton);
	rightLayout->addWidget(hallCalibrationStart);
	rightLayout->addStretch();

	auto *modeLayout = new QHBoxLayout;
	modeLayout->addWidget(new QLabel(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.CaptureMode"), this));
	modeLayout->addWidget(modeSelector, 1);
	leftLayout->addLayout(modeLayout);
	auto *parametersForm = new QFormLayout;
	parametersForm->addRow(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ParameterAutoTracking"),
			       parametersAutoTracking);
	parametersForm->addRow(parametersAngleRange);
	leftLayout->addLayout(parametersForm);
	leftLayout->addWidget(angles);
	leftLayout->addLayout(grid);
	connect(parametersAutoZoom, &QCheckBox::toggled, this, &OBSBasicFalconMControl::ApplyAutoZoom);
	connect(parametersAutoTracking, &QCheckBox::toggled, this, &OBSBasicFalconMControl::ApplyAutoTracking);
	connect(parametersAngleRange, &xbotgo::SliderControl::sliderReleased, this,
		&OBSBasicFalconMControl::ApplyAngleRange);
	connect(modeSelector, qOverload<int>(&QComboBox::currentIndexChanged), this,
		&OBSBasicFalconMControl::SelectMode);
	connect(buzzerLongButton, &QPushButton::clicked, this,
		[this] { SendBuzzerMode(xbotgo::BuzzerMode::Beep3000Ms); });
	connect(hallCalibrationStart, &QPushButton::clicked, this, &OBSBasicFalconMControl::StartHallCalibration);
	connect(manualZoomSlider, &xbotgo::SliderControl::valueChanged, this,
		&OBSBasicFalconMControl::ManualZoomValueChanged);
	connect(manualZoomSlider, &xbotgo::SliderControl::sliderPressed, this, [this] {
		manualZoomDragging = true;
		manualZoomCommandValue = manualZoomSlider->value();
	});
	connect(manualZoomSlider, &xbotgo::SliderControl::sliderReleased, this, [this] {
		manualZoomDragging = false;
		manualZoomQueryDebounce->stop();
		QueryCurrentZoom();
	});

	auto *poller = new QTimer(this);
	connect(poller, &QTimer::timeout, this, &OBSBasicFalconMControl::Refresh);
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
		const bool connected = xbotgo::IsFalconMSourceConnected(source);
		const bool calibrating = currentHallCalibrationStatus ==
			static_cast<int>(xbotgo::falconm_hall_calibration_status::calibrating);
		hallCalibrationStart->setEnabled(connected && !calibrating);
	});
	manualZoomQueryDebounce = new QTimer(this);
	manualZoomQueryDebounce->setSingleShot(true);
	manualZoomQueryDebounce->setInterval(250);
	connect(manualZoomQueryDebounce, &QTimer::timeout, this, &OBSBasicFalconMControl::QueryCurrentZoom);
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
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		return;
	}

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_int(&cd, "direction", direction);
	calldata_set_int(&cd, "operation", operation);
	proc_handler_call(obs_source_get_proc_handler(source), "send_direction", &cd);
	calldata_free(&cd);
}

void OBSBasicFalconMControl::SendBuzzerMode(xbotgo::BuzzerMode mode)
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		return;
	}

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_int(&cd, "mode", static_cast<long long>(mode));
	proc_handler_call(obs_source_get_proc_handler(source), "set_buzzer_mode", &cd);
	calldata_free(&cd);
}

void OBSBasicFalconMControl::QueryHallCalibration()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		hallCalibrationStart->setEnabled(false);
		return;
	}

	calldata_t state;
	calldata_init(&state);
	proc_handler_call(obs_source_get_proc_handler(source), "get_hall_calibration", &state);
	long long sequence = 0;
	calldata_get_int(&state, "sequence", &sequence);
	calldata_free(&state);

	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "query_hall_calibration", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (!success) {
		hallCalibrationStart->setEnabled(true);
		return;
	}

	hallCalibrationQuerySequence = static_cast<uint64_t>(sequence);
	hallCalibrationStart->setEnabled(false);
	hallCalibrationTimeout->start();
}

void OBSBasicFalconMControl::StartHallCalibration()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		return;
	}

	calldata_t state;
	calldata_init(&state);
	proc_handler_call(obs_source_get_proc_handler(source), "get_hall_calibration", &state);
	long long sequence = 0;
	calldata_get_int(&state, "sequence", &sequence);
	calldata_free(&state);

	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "start_hall_calibration", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (!success) {
		return;
	}

	hallCalibrationQuerySequence = static_cast<uint64_t>(sequence);
	hallCalibrationStart->setEnabled(false);
	hallCalibrationTimeout->start();
}

void OBSBasicFalconMControl::UpdateHallCalibration()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		return;
	}

	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "get_hall_calibration", &cd);
	long long sequence = 0, status = -1;
	calldata_get_int(&cd, "sequence", &sequence);
	calldata_get_int(&cd, "status", &status);
	calldata_free(&cd);
	if (sequence <= 0 || static_cast<uint64_t>(sequence) <= hallCalibrationQuerySequence ||
	    static_cast<uint64_t>(sequence) == displayedHallCalibrationSequence) {
		return;
	}

	switch (static_cast<xbotgo::falconm_hall_calibration_status>(status)) {
	case xbotgo::falconm_hall_calibration_status::uncalibrated:
	case xbotgo::falconm_hall_calibration_status::calibrating:
	case xbotgo::falconm_hall_calibration_status::succeeded:
	case xbotgo::falconm_hall_calibration_status::failed:
		break;
	default:
		return;
	}

	currentHallCalibrationStatus = static_cast<int>(status);
	displayedHallCalibrationSequence = static_cast<uint64_t>(sequence);
	hallCalibrationTimeout->stop();
	hallCalibrationStart->setEnabled(
		status != static_cast<int>(xbotgo::falconm_hall_calibration_status::calibrating));
}

void OBSBasicFalconMControl::QueryCurrentZoom()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		manualZoomSlider->setEnabled(false);
		return;
	}

	calldata_t state;
	calldata_init(&state);
	proc_handler_call(obs_source_get_proc_handler(source), "get_current_zoom", &state);
	long long sequence = 0;
	calldata_get_int(&state, "sequence", &sequence);
	calldata_free(&state);

	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "query_current_zoom", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (!success) {
		UpdateManualZoomEnabled();
		return;
	}

	manualZoomQuerySequence = static_cast<uint64_t>(sequence);
	UpdateManualZoomEnabled();
}

void OBSBasicFalconMControl::UpdateCurrentZoom()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		return;
	}

	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "get_current_zoom", &cd);
	long long sequence = 0, value = 0;
	calldata_get_int(&cd, "sequence", &sequence);
	calldata_get_int(&cd, "value", &value);
	calldata_free(&cd);
	if (sequence <= 0 || value < 10 || value > 30 ||
	    static_cast<uint64_t>(sequence) <= manualZoomQuerySequence ||
	    static_cast<uint64_t>(sequence) == displayedManualZoomSequence) {
		return;
	}

	hasCurrentManualZoom = true;
	displayedManualZoomSequence = static_cast<uint64_t>(sequence);
	if (!manualZoomDragging) {
		const QSignalBlocker blocker(manualZoomSlider);
		manualZoomSlider->setValue(static_cast<int>(value));
		manualZoomCommandValue = static_cast<int>(value);
	}
	UpdateManualZoomEnabled();
}

bool OBSBasicFalconMControl::DisableAutoZoomForManualControl()
{
	if (!parametersAutoZoom->isChecked()) {
		return true;
	}
	if (!hasConfirmedCaptureParameters) {
		return false;
	}

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_bool(&cd, "auto_zoom", false);
	calldata_set_bool(&cd, "auto_tracking", parametersAutoTracking->isChecked());
	calldata_set_int(&cd, "angle_range", parametersAngleRange->value());
	proc_handler_call(obs_source_get_proc_handler(source), "set_capture_zoom_tracking_and_angle_range", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (!success) {
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

void OBSBasicFalconMControl::ManualZoomValueChanged(int value)
{
	if (!xbotgo::IsFalconMSourceConnected(source) || !hasCurrentManualZoom || !hasConfirmedCaptureParameters ||
	    value < 10 || value > 30 || value == manualZoomCommandValue) {
		return;
	}
	if (!DisableAutoZoomForManualControl()) {
		const QSignalBlocker blocker(manualZoomSlider);
		manualZoomSlider->setValue(manualZoomCommandValue);
		return;
	}

	const int direction = value > manualZoomCommandValue ? 1 : -1;
	bool success = true;
	while (manualZoomCommandValue != value) {
		calldata_t cd;
		calldata_init(&cd);
		calldata_set_int(&cd, "type", static_cast<int>(xbotgo::falconm_zoom_type::relative));
		calldata_set_int(&cd, "value", direction);
		proc_handler_call(obs_source_get_proc_handler(source), "send_manual_zoom", &cd);
		calldata_get_bool(&cd, "success", &success);
		calldata_free(&cd);
		if (!success) {
			break;
		}
		manualZoomCommandValue += direction;
	}

	if (!success) {
		const QSignalBlocker blocker(manualZoomSlider);
		manualZoomSlider->setValue(manualZoomCommandValue);
		QueryCurrentZoom();
		return;
	}

	if (!manualZoomDragging) {
		manualZoomQueryDebounce->start();
	}
}

void OBSBasicFalconMControl::UpdateManualZoomEnabled()
{
	const bool connected = xbotgo::IsFalconMSourceConnected(source);
	manualZoomSlider->setEnabled(connected && hasCurrentManualZoom && hasConfirmedCaptureParameters &&
				     !parametersAutoZoom->isChecked());
}

void OBSBasicFalconMControl::QueryModes()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		modeSelector->setEnabled(false);
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
		return;
	}
	modeQuerySequence = static_cast<uint64_t>(sequence);
	waitingForModes = true;
	modeSelector->setEnabled(false);
}

void OBSBasicFalconMControl::QueryCaptureParameters()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		parametersAutoZoom->setEnabled(false);
		parametersAutoTracking->setEnabled(false);
		parametersAngleRange->setEnabled(false);
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
		return;
	}
	parametersAutoZoom->setEnabled(false);
	parametersAutoTracking->setEnabled(false);
	parametersAngleRange->setEnabled(false);
}

void OBSBasicFalconMControl::ApplyAutoZoom(bool checked)
{
	if (!xbotgo::IsFalconMSourceConnected(source) || !hasConfirmedCaptureParameters ||
	    checked == confirmedAutoZoom) {
		return;
	}

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_bool(&cd, "auto_zoom", checked);
	proc_handler_call(obs_source_get_proc_handler(source), "set_capture_auto_zoom", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (success) {
		confirmedAutoZoom = checked;
	} else {
		const QSignalBlocker blocker(parametersAutoZoom);
		parametersAutoZoom->setChecked(confirmedAutoZoom);
	}
	UpdateManualZoomEnabled();
}

void OBSBasicFalconMControl::ApplyAutoTracking(bool checked)
{
	if (!xbotgo::IsFalconMSourceConnected(source) || !hasConfirmedCaptureParameters ||
	    checked == confirmedAutoTracking) {
		return;
	}

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_bool(&cd, "auto_tracking", checked);
	proc_handler_call(obs_source_get_proc_handler(source), "set_capture_auto_tracking", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (success) {
		confirmedAutoTracking = checked;
	} else {
		const QSignalBlocker blocker(parametersAutoTracking);
		parametersAutoTracking->setChecked(confirmedAutoTracking);
	}
}

void OBSBasicFalconMControl::ApplyAngleRange()
{
	if (!xbotgo::IsFalconMSourceConnected(source) || !hasConfirmedCaptureParameters ||
	    parametersAngleRange->value() == confirmedAngleRange) {
		return;
	}

	calldata_t cd;
	calldata_init(&cd);
	calldata_set_int(&cd, "angle_range", parametersAngleRange->value());
	proc_handler_call(obs_source_get_proc_handler(source), "set_capture_angle_range", &cd);
	bool success = false;
	calldata_get_bool(&cd, "success", &success);
	calldata_free(&cd);
	if (success) {
		confirmedAngleRange = parametersAngleRange->value();
	} else {
		const QSignalBlocker blocker(parametersAngleRange);
		parametersAngleRange->setValue(confirmedAngleRange);
	}
}

void OBSBasicFalconMControl::UpdateCaptureParameters()
{
	if (!xbotgo::IsFalconMSourceConnected(source)) {
		return;
	}
	calldata_t cd;
	calldata_init(&cd);
	proc_handler_call(obs_source_get_proc_handler(source), "get_capture_parameters", &cd);
	long long sequence = 0, angleRange = 0;
	bool autoZoom = false, autoTracking = false;
	calldata_get_int(&cd, "sequence", &sequence);
	calldata_get_bool(&cd, "auto_zoom", &autoZoom);
	calldata_get_bool(&cd, "auto_tracking", &autoTracking);
	calldata_get_int(&cd, "angle_range", &angleRange);
	if (sequence <= 0 || static_cast<uint64_t>(sequence) <= parametersQuerySequence ||
	    static_cast<uint64_t>(sequence) == displayedParametersSequence) {
		calldata_free(&cd);
		return;
	}

	const QSignalBlocker auto_zoom_blocker(parametersAutoZoom);
	const QSignalBlocker auto_tracking_blocker(parametersAutoTracking);
	const QSignalBlocker angle_range_blocker(parametersAngleRange);
	parametersAutoZoom->setChecked(autoZoom);
	parametersAutoTracking->setChecked(autoTracking);
	parametersAngleRange->setValue(static_cast<int>(angleRange));
	confirmedAutoZoom = autoZoom;
	confirmedAutoTracking = autoTracking;
	confirmedAngleRange = parametersAngleRange->value();
	hasConfirmedCaptureParameters = true;
	UpdateManualZoomEnabled();
	parametersAutoZoom->setEnabled(true);
	parametersAutoTracking->setEnabled(true);
	parametersAngleRange->setEnabled(true);
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
		    xbotgo::falconm_is_basketball_mode(
			    static_cast<xbotgo::ModeType>(static_cast<uint16_t>(mode)))) {
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
}

void OBSBasicFalconMControl::SelectMode(int index)
{
	if (index < 0 || waitingForModes || waitingForModeResult ||
	    !xbotgo::IsFalconMSourceConnected(source)) {
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
		RestoreConfirmedMode();
		return;
	}
	modeResultSequence = static_cast<uint64_t>(sequence);
	waitingForModeResult = true;
	modeSelector->setEnabled(false);
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
		QueryModes();
	} else {
		RestoreConfirmedMode();
	}
}

void OBSBasicFalconMControl::RestoreConfirmedMode()
{
	waitingForModeResult = false;
	modeTimeout->stop();
	const QSignalBlocker blocker(modeSelector);
	modeSelector->setCurrentIndex(modeSelector->findData(confirmedMode));
	modeSelector->setEnabled(xbotgo::IsFalconMSourceConnected(source) && modeSelector->count() > 0);
}

void OBSBasicFalconMControl::Refresh()
{
	if (!source) {
		return;
	}
	const bool connected = xbotgo::IsFalconMSourceConnected(source);
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
		UpdateModes();
		HandleModeResult();
		UpdateCaptureParameters();
		UpdateHallCalibration();
		UpdateCurrentZoom();
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
