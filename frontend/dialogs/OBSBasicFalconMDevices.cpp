#include "OBSBasicFalconMDevices.hpp"

#include <OBSApp.hpp>

#include "moc_OBSBasicFalconMDevices.cpp"

#include <obs.h>
#include <qt-wrappers.hpp>

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
	resize(560, 320);

	devices = new QTableWidget(this);
	devices->setColumnCount(3);
	devices->setHorizontalHeaderLabels({QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Source"),
					    QTStr("Basic.MainMenu.XBotGo.DeviceManagement.DeviceId"),
					    QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Status")});
	devices->setEditTriggers(QAbstractItemView::NoEditTriggers);
	devices->setSelectionBehavior(QAbstractItemView::SelectRows);
	devices->setSelectionMode(QAbstractItemView::SingleSelection);
	devices->setSortingEnabled(true);
	devices->horizontalHeader()->setStretchLastSection(true);
	devices->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	devices->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	devices->verticalHeader()->setVisible(false);

	auto *refresh = new QPushButton(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Refresh"), this);
	connect(refresh, &QPushButton::clicked, this, &OBSBasicFalconMDevices::ReloadDevices);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(devices);
	layout->addWidget(refresh, 0, Qt::AlignRight);

	ReloadDevices();
}

void OBSBasicFalconMDevices::ReloadDevices()
{
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
			table->insertRow(row);
			table->setItem(row, 0, new QTableWidgetItem(QT_UTF8(obs_source_get_name(source))));
			table->setItem(row, 1, new QTableWidgetItem(QT_UTF8(deviceId)));
			table->setItem(row, 2,
				       new QTableWidgetItem(
					       QTStr(obs_source_active(source)
							     ? "Basic.MainMenu.XBotGo.DeviceManagement.Active"
							     : "Basic.MainMenu.XBotGo.DeviceManagement.Inactive")));
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
