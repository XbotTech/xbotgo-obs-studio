#include "OBSBasicFalconMControl.hpp"

#include <OBSApp.hpp>
#include "moc_OBSBasicFalconMControl.cpp"

#include <obs.h>
#include <callback/calldata.h>
#include <qt-wrappers.hpp>

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

OBSBasicFalconMControl::OBSBasicFalconMControl(obs_source_t *source_, QWidget *parent)
	: QDialog(parent),
	  source(obs_source_get_ref(source_))
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Control"));
	resize(360, 240);

	connection = new QLabel(this);
	angles = new QLabel(this);
	connection->setText(QTStr(obs_source_active(source) ? "Basic.MainMenu.XBotGo.DeviceManagement.Active"
							    : "Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));

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
	layout->addWidget(angles);
	layout->addLayout(grid);

	poller = new QTimer(this);
	connect(poller, &QTimer::timeout, this, &OBSBasicFalconMControl::Refresh);
	poller->start(500);
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

void OBSBasicFalconMControl::Refresh()
{
	if (!source) {
		return;
	}
	connection->setText(QTStr(obs_source_active(source) ? "Basic.MainMenu.XBotGo.DeviceManagement.Active"
							    : "Basic.MainMenu.XBotGo.DeviceManagement.Inactive"));
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
