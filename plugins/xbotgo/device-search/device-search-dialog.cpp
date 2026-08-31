#include "device-search-dialog.hpp"
#include "util/base.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QNetworkDatagram>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

namespace xbotgo {

#define SSDP_ADDR "239.255.255.250"
#define SSDP_PORT 1900

#define COLUMN_INDEX_IP 2

DeviceSearchDialog::DeviceSearchDialog(QWidget *parent) : QDialog(parent), group_address4(SSDP_ADDR)
{
	// 列表
	deviceTable = new QTableView(this);
	deviceModel = new QStandardItemModel(0, 9, this);
	deviceModel->setHorizontalHeaderLabels({tr("Device ID"), tr("Serial Number"), tr("IP Address"),
						tr("Subnet Mask"), tr("MAC Address"), tr("Device Version"),
						tr("MQTT Port"), tr("Role"), tr("Time Synchronized")});
	deviceTable->setModel(deviceModel);
	deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
	deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	deviceTable->setColumnWidth(COLUMN_INDEX_IP, 120);
	deviceTable->horizontalHeader()->setStretchLastSection(true);
	connect(deviceTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
		&DeviceSearchDialog::updateSelection);

	auto buttonBox = new QDialogButtonBox;

	// 刷新 - btn
	auto refreshButton = new QPushButton(tr("&Refresh"));
	connect(refreshButton, &QPushButton::clicked, this, &DeviceSearchDialog::refreshSearch);

	// 确认 -btn
	selectButton = buttonBox->addButton(QDialogButtonBox::Ok);
	selectButton->setEnabled(false);

	// btnbox
	buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(deviceTable, &QTableView::doubleClicked, this, [this] {
		if (selectedDevice()) {
			accept();
		}
	});

	auto mainLayout = new QVBoxLayout;
	mainLayout->addWidget(deviceTable);
	mainLayout->addWidget(buttonBox);
	setLayout(mainLayout);

	setMinimumSize(600, 400);
	resize(800, 500);

	initSocket();
}

DeviceSearchDialog::~DeviceSearchDialog()
{
	uninitSocket();
}

std::optional<DeviceInfo> DeviceSearchDialog::selectedDevice() const
{
	const QModelIndex current = deviceTable->currentIndex();
	if (!current.isValid()) {
		return std::nullopt;
	}

	const QStandardItem *idItem = deviceModel->item(current.row(), 0);
	if (!idItem) {
		return std::nullopt;
	}

	const auto device = devices.constFind(idItem->text());
	if (device == devices.cend()) {
		return std::nullopt;
	}

	return *device.value();
}

void DeviceSearchDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	clearDevices();
}

void DeviceSearchDialog::hideEvent(QHideEvent *event)
{
	QDialog::hideEvent(event);
}

void DeviceSearchDialog::initSocket()
{
	if (!socket4.bind(QHostAddress::AnyIPv4, SSDP_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
		const std::string error = socket4.errorString().toStdString();
		blog(LOG_ERROR, "Failed to bind socket: %s", error.c_str());
		return;
	}

	const auto interfaces = QNetworkInterface::allInterfaces();
	for (const QNetworkInterface &networkInterface : interfaces) {
		if (shouldJoinInterface(networkInterface)) {
			const std::string human_readable_name = networkInterface.humanReadableName().toStdString();
			if (socket4.joinMulticastGroup(group_address4, networkInterface)) {
				join_multicast_interfaces.append(networkInterface);
				qDebug() << "Using multicast interface" << human_readable_name.c_str();
				blog(LOG_DEBUG, "Using multicast interface %s", human_readable_name.c_str());
			} else {
				const std::string error = socket4.errorString().toStdString();
				blog(LOG_WARNING, "Failed to join multicast group on %s: %s",
				     human_readable_name.c_str(), error.c_str());
			}
		}
	}

	connect(&socket4, &QUdpSocket::readyRead, this, &DeviceSearchDialog::readPendingDatagrams);
}

void DeviceSearchDialog::uninitSocket()
{
	for (const QNetworkInterface &networkInterface : join_multicast_interfaces) {
		const std::string human_readable_name = networkInterface.humanReadableName().toStdString();
		if (socket4.leaveMulticastGroup(group_address4, networkInterface)) {
			blog(LOG_DEBUG, "Leaved multicast interface %s", human_readable_name.c_str());
		} else {
			const std::string error = socket4.errorString().toStdString();
			blog(LOG_WARNING, "Failed to leave multicast group on %s: %s", human_readable_name.c_str(),
			     error.c_str());
		}
	}
	socket4.close();
}

bool DeviceSearchDialog::shouldJoinInterface(const QNetworkInterface &interface)
{
	const auto flags = interface.flags();
	if (!flags.testFlag(QNetworkInterface::IsUp)            // ifconfig up
	    || !flags.testFlag(QNetworkInterface::IsRunning)    // running
	    || !flags.testFlag(QNetworkInterface::CanMulticast) // 支持多播
	    || !flags.testFlag(QNetworkInterface::CanBroadcast) // 支持广播
	    || flags.testFlag(QNetworkInterface::IsLoopBack)    // 回环接口
	) {
		return false;
	}

	for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
		if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
			return true;
		}
	}

	return false;
}

void DeviceSearchDialog::refreshSearch()
{
	clearDevices();
}

void DeviceSearchDialog::updateDevice(const DeviceInfo &device)
{
	const QString unknown = tr("Unknown");
	const QString timeSynchronized = device.timeSynchronized.has_value()
						 ? (device.timeSynchronized.value() ? tr("Yes") : tr("No"))
						 : unknown;
	const QStringList values{device.id,
				 device.serialNumber.isEmpty() ? unknown : device.serialNumber,
				 device.ip,
				 device.mask.isEmpty() ? unknown : device.mask,
				 device.mac.isEmpty() ? unknown : device.mac,
				 device.version.isEmpty() ? unknown : device.version,
				 QString::number(device.mqttPort),
				 device.role.isEmpty() ? unknown : device.role,
				 timeSynchronized};
	devices.insert(device.id, std::make_shared<const DeviceInfo>(device));

	const auto existing = deviceRows.constFind(device.id);
	if (existing != deviceRows.cend()) {
		const int row = existing.value();
		for (int column = 0; column < values.size(); ++column) {
			deviceModel->item(row, column)->setText(values.at(column));
		}
		return;
	}

	QList<QStandardItem *> items;
	items.reserve(values.size());
	for (const QString &value : values) {
		items.append(new QStandardItem(value));
	}

	const int row = deviceModel->rowCount();
	deviceModel->appendRow(items);
	deviceRows.insert(device.id, row);
}

void DeviceSearchDialog::clearDevices()
{
	deviceModel->removeRows(0, deviceModel->rowCount());
	deviceRows.clear();
	devices.clear();
	updateSelection();
}

void DeviceSearchDialog::updateSelection()
{
	if (selectButton) {
		selectButton->setEnabled(selectedDevice().has_value());
	}
}

void DeviceSearchDialog::readPendingDatagrams()
{
	while (socket4.hasPendingDatagrams()) {
		const QNetworkDatagram datagram = socket4.receiveDatagram();
		const std::optional<DeviceInfo> device = DeviceInfo::parseSsdpDevice(datagram.data());
		if (device && device->isValid()) {
			updateDevice(*device);
		}
	}
}

} // namespace xbotgo
