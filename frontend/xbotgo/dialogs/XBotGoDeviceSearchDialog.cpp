#include "XBotGoDeviceSearchDialog.hpp"
#include "../models/XBotGoSsdpParser.hpp"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHideEvent>
#include <QNetworkDatagram>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

namespace XBotGo {

#define SSDP_QUERY_IP "239.255.255.250"

static const uint16_t SSDP_PORT = 1900;

DeviceSearchDialog::DeviceSearchDialog(QWidget *parent)
	: QDialog(parent),
	  groupAddress4(QStringLiteral(SSDP_QUERY_IP))
{
	deviceTable = new QTableView(this);
	deviceModel = new QStandardItemModel(0, 4, this);
	deviceModel->setHorizontalHeaderLabels(
		{tr("Device ID"), tr("IP Address"), tr("MQTT Port"), tr("Protocol Version")});
	deviceTable->setModel(deviceModel);
	deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
	deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	deviceTable->horizontalHeader()->setStretchLastSection(true);

	auto refreshButton = new QPushButton(tr("&Refresh"));
	auto buttonBox = new QDialogButtonBox;
	buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);

	connect(refreshButton, &QPushButton::clicked, this, &DeviceSearchDialog::refreshSearch);
	connect(&timer, &QTimer::timeout, this, &DeviceSearchDialog::sendDatagram);

	auto mainLayout = new QVBoxLayout;
	mainLayout->addWidget(deviceTable);
	mainLayout->addWidget(buttonBox);
	setLayout(mainLayout);

	setMinimumSize(600, 400);
	resize(800, 500);

	socket4 = new QUdpSocket(this);
	if (!socket4->bind(QHostAddress::AnyIPv4, SSDP_PORT,
			   QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
		qCritical() << "Failed to bind socket:" << socket4->errorString();
		return;
	}

	const auto interfaces = QNetworkInterface::allInterfaces();
	for (const QNetworkInterface &networkInterface : interfaces) {
		const auto flags = networkInterface.flags();
		if (!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning) ||
		    !flags.testFlag(QNetworkInterface::CanMulticast) ||
		    !flags.testFlag(QNetworkInterface::CanBroadcast) || flags.testFlag(QNetworkInterface::IsLoopBack)) {
			continue;
		}

		bool hasIPv4Address = false;
		for (const QNetworkAddressEntry &entry : networkInterface.addressEntries()) {
			if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
				hasIPv4Address = true;
				break;
			}
		}
		if (!hasIPv4Address)
			continue;

		if (socket4->joinMulticastGroup(groupAddress4, networkInterface)) {
			multicastInterfaces.append(networkInterface);
			qDebug() << "Using multicast interface" << networkInterface.humanReadableName();
		} else {
			qWarning() << "Failed to join multicast group on" << networkInterface.humanReadableName() << ":"
				   << socket4->errorString();
		}
	}

	if (multicastInterfaces.isEmpty()) {
		qCritical() << "No active IPv4 multicast interface is available";
		socket4->close();
		return;
	}

	connect(socket4, &QUdpSocket::readyRead, this, &DeviceSearchDialog::readPendingDatagrams);
}

DeviceSearchDialog::~DeviceSearchDialog()
{
	stopSearch();
	if (socket4) {
		for (const QNetworkInterface &networkInterface : multicastInterfaces)
			socket4->leaveMulticastGroup(groupAddress4, networkInterface);
		qDebug() << "Leaving Multicast group\n";
		socket4->close();
	}
}

void DeviceSearchDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	startSearch();
}

void DeviceSearchDialog::hideEvent(QHideEvent *event)
{
	stopSearch();
	QDialog::hideEvent(event);
}

void DeviceSearchDialog::startSearch()
{
	clearDevices();
	if (!socket4 || socket4->state() != QAbstractSocket::BoundState || multicastInterfaces.isEmpty())
		return;

	timer.start(3000);
	sendDatagram();
}

void DeviceSearchDialog::stopSearch()
{
	timer.stop();
}

void DeviceSearchDialog::refreshSearch()
{
	startSearch();
}

void DeviceSearchDialog::sendDatagram()
{
	std::string datagramStr =
	    "M-SEARCH * HTTP/1.1\r\n"
	    "HOST: " +
	    std::string(SSDP_QUERY_IP) + ":" + std::to_string(SSDP_PORT) +
	    "\r\n"
	    "MAN: \"ssdp:discover\"\r\n"
	    "MX: 2\r\n"
	    "ST: ssdp:all\r\n\r\n";
	QByteArray datagramData(datagramStr.c_str());
	qint64 bytesSent = socket4->writeDatagram(datagramData.constData(),
						  groupAddress4, SSDP_PORT);
	if (bytesSent == -1) {
		qCritical() << "Failed to send datagram:" << socket4->errorString();
	} else {
		qDebug() << datagramStr;
		qDebug() << bytesSent << "bytes to " << groupAddress4.toString() << ":"
			 << SSDP_PORT;
	}
}

void DeviceSearchDialog::updateDevice(const Device &device)
{
	const QStringList values{device.id, device.ip, QString::number(device.mqttPort),
				 QString::number(device.protocolVersion)};

	const auto existing = deviceRows.constFind(device.id);
	if (existing != deviceRows.cend()) {
		const int row = existing.value();
		for (int column = 0; column < values.size(); ++column)
			deviceModel->item(row, column)->setText(values.at(column));
		return;
	}

	QList<QStandardItem *> items;
	items.reserve(values.size());
	for (const QString &value : values)
		items.append(new QStandardItem(value));

	const int row = deviceModel->rowCount();
	deviceModel->appendRow(items);
	deviceRows.insert(device.id, row);
}

void DeviceSearchDialog::clearDevices()
{
	deviceModel->removeRows(0, deviceModel->rowCount());
	deviceRows.clear();
}

void DeviceSearchDialog::readPendingDatagrams()
{
	while (socket4->hasPendingDatagrams()) {
		const QNetworkDatagram datagram = socket4->receiveDatagram();
		const std::optional<Device> device = parseSsdpDevice(datagram.data(), datagram.senderAddress());
		if (!device) {
			qDebug() << "Ignoring invalid SSDP response from" << datagram.senderAddress();
			continue;
		}

		updateDevice(*device);
	}
}

} // namespace XBotGo
