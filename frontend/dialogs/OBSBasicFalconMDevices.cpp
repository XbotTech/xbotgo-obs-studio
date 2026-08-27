#include "OBSBasicFalconMDevices.hpp"
#include "OBSBasicFalconMControl.hpp"

#include <OBSApp.hpp>

#include "moc_OBSBasicFalconMDevices.cpp"

#include <obs.h>
#include <qt-wrappers.hpp>
#include <xbotgo/sources/XBotGoFalconMSource.hpp>

#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <cstring>

namespace {
constexpr const char *FalconMSourceId = "xbotogo_falconm";
}

OBSBasicFalconMDevices::OBSBasicFalconMDevices(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(QTStr("Basic.MainMenu.XBotGo.DeviceManagement"));
	setModal(false);
	resize(680, 320);

	devices = new QTableWidget(this);
	devices->setColumnCount(4);
	devices->setHorizontalHeaderLabels({QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Source"),
					    QTStr("Basic.MainMenu.XBotGo.DeviceManagement.DeviceId"),
					    QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Status"),
					    QTStr("Basic.MainMenu.XBotGo.DeviceManagement.ConnectionStatus")});
	devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
	devices->setSelectionBehavior(QAbstractItemView::SelectRows);
	devices->setSelectionMode(QAbstractItemView::SingleSelection);
	devices->setSortingEnabled(true);
	devices->horizontalHeader()->setStretchLastSection(false);
	devices->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	devices->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	devices->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	devices->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	devices->verticalHeader()->setVisible(false);

	auto *refresh = new QPushButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Refresh"), this);
	connect(refresh, &QPushButton::clicked, this, &OBSBasicFalconMDevices::ReloadDevices);
	connect(devices, &QTableWidget::cellDoubleClicked, this, &OBSBasicFalconMDevices::OpenControl);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(devices);
	layout->addWidget(refresh, 0, Qt::AlignRight);

	ReloadDevices();
}

OBSBasicFalconMDevices::~OBSBasicFalconMDevices()
{
	for (auto *source : sources) {
		obs_source_release(source);
	}
}

void OBSBasicFalconMDevices::ReloadDevices()
{
	for (auto *source : sources) {
		obs_source_release(source);
	}
	sources.clear();
	devices->setSortingEnabled(false);
	devices->setRowCount(0);

	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			auto *table = static_cast<QTableWidget *>(data);
			const char *id = obs_source_get_id(source);
			if (!id || strcmp(id, FalconMSourceId) != 0) {
				return true;
			}

			OBSDataAutoRelease settings = obs_source_get_settings(source);
			const char *deviceId = settings ? obs_data_get_string(settings, "device_id") : "";
			const int row = table->rowCount();
			auto *source_ref = obs_source_get_ref(source);
			auto *dialog = static_cast<OBSBasicFalconMDevices *>(table->parentWidget());
			const size_t sourceIndex = dialog->sources.size();
			dialog->sources.push_back(source_ref);
			table->insertRow(row);
			auto *sourceItem = new QTableWidgetItem(QT_UTF8(obs_source_get_name(source)));
			sourceItem->setData(Qt::UserRole, static_cast<qulonglong>(sourceIndex));
			table->setItem(row, 0, sourceItem);
			table->setItem(row, 1, new QTableWidgetItem(QT_UTF8(deviceId)));
			table->setItem(row, 2,
				       new QTableWidgetItem(
					       QTStr(obs_source_active(source)
							     ? "Basic.MainMenu.XBotGo.DeviceManagement.Active"
							     : "Basic.MainMenu.XBotGo.DeviceManagement.Inactive")));
			table->setItem(
				row, 3,
				new QTableWidgetItem(QTStr(xbotgo::IsFalconMSourceConnected(source)
							   ? "Basic.MainMenu.XBotGo.DeviceManagement.Connected"
							   : "Basic.MainMenu.XBotGo.DeviceManagement.Disconnected")));
			return true;
		},
		devices);

	if (devices->rowCount() == 0) {
		devices->setRowCount(1);
		devices->setItem(0, 0, new QTableWidgetItem(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Empty")));
		devices->setSpan(0, 0, 1, devices->columnCount());
		devices->item(0, 0)->setTextAlignment(Qt::AlignCenter);
	}

	devices->setSortingEnabled(true);
}

void OBSBasicFalconMDevices::OpenControl(int row, int)
{
	auto *sourceItem = row >= 0 ? devices->item(row, 0) : nullptr;
	if (!sourceItem) {
		return;
	}

	bool validIndex = false;
	const qulonglong sourceIndex = sourceItem->data(Qt::UserRole).toULongLong(&validIndex);
	if (!validIndex || sourceIndex >= sources.size()) {
		return;
	}
	auto *source = sources[static_cast<size_t>(sourceIndex)];
	if (controls.value(source)) {
		controls.value(source)->raise();
		controls.value(source)->activateWindow();
		return;
	}
	auto *control = new OBSBasicFalconMControl(source, this);
	controls.insert(source, control);
	connect(control, &QObject::destroyed, this, [this, source] { controls.remove(source); });
	control->show();
}
