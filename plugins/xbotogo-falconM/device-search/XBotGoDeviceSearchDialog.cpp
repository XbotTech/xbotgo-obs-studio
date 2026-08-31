#include "XBotGoDeviceSearchDialog.hpp"
#include "XBotGoSsdpParser.hpp"

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

#define SSDP_QUERY_IP "239.255.255.250"

static const uint16_t SSDP_PORT = 1900;

DeviceSearchDialog::DeviceSearchDialog(QWidget *parent, Mode mode_)
	: QDialog(parent),
	  mode(mode_),
	  groupAddress4(QStringLiteral(SSDP_QUERY_IP))
{
	deviceTable = new QTableView(this);
	deviceModel = new QStandardItemModel(0, 9, this);
	deviceModel->setHorizontalHeaderLabels({tr("Device ID"), tr("Serial Number"), tr("IP Address"),
						tr("Subnet Mask"), tr("MAC Address"), tr("Device Version"),
						tr("MQTT Port"), tr("Role"), tr("Time Synchronized")});
	deviceTable->setModel(deviceModel);
	deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
	deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	deviceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	deviceTable->horizontalHeader()->setStretchLastSection(true);

	auto refreshButton = new QPushButton(tr("&Refresh"));
	auto buttonBox = new QDialogButtonBox;
	buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);
	if (mode == Mode::Select) {
		selectButton = buttonBox->addButton(QDialogButtonBox::Ok);
		selectButton->setEnabled(false);
		buttonBox->addButton(QDialogButtonBox::Cancel);
		connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
		connect(deviceTable, &QTableView::doubleClicked, this, [this] {
			if (selectedDevice()) {
				accept();
			}
		});
	} else {
		buttonBox->addButton(QDialogButtonBox::Close);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	}

	connect(refreshButton, &QPushButton::clicked, this, &DeviceSearchDialog::refreshSearch);
	connect(&timer, &QTimer::timeout, this, &DeviceSearchDialog::sendDatagram);
	connect(deviceTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
		&DeviceSearchDialog::updateSelection);

	auto mainLayout = new QVBoxLayout;
	mainLayout->addWidget(deviceTable);
	mainLayout->addWidget(buttonBox);
	setLayout(mainLayout);

	setMinimumSize(600, 400);
	resize(800, 500);

	socket4 = new QUdpSocket(this);
	if (!socket4->bind(QHostAddress::AnyIPv4, SSDP_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
		qCritical() << "Failed to bind socket:" << socket4->errorString();
		return;
	}

	const auto interfaces = QNetworkInterface::allInterfaces();
	for (const QNetworkInterface &networkInterface : interfaces) {
		const auto flags = networkInterface.flags();
		if (!flags.testFlag(QNetworkInterface::IsUp)            // ifconfig up
		    || !flags.testFlag(QNetworkInterface::IsRunning)    // running
		    || !flags.testFlag(QNetworkInterface::CanMulticast) // 支持多播
		    || !flags.testFlag(QNetworkInterface::CanBroadcast) // 支持广播
		    || flags.testFlag(QNetworkInterface::IsLoopBack)    // 回环接口
		) {
			continue;
		}

		bool hasIPv4Address = false;
		for (const QNetworkAddressEntry &entry : networkInterface.addressEntries()) {
			if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
				hasIPv4Address = true;
				break;
			}
		}
		if (!hasIPv4Address) {
			continue;
		}

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
		for (const QNetworkInterface &networkInterface : multicastInterfaces) {
			socket4->leaveMulticastGroup(groupAddress4, networkInterface);
		}
		socket4->close();
	}
}

std::optional<Device> DeviceSearchDialog::selectedDevice() const
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

	return device.value();
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
	if (!socket4 || socket4->state() != QAbstractSocket::BoundState || multicastInterfaces.isEmpty()) {
		return;
	}

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
	const QByteArray datagramData = QByteArrayLiteral("M-SEARCH * HTTP/1.1\r\n") +
					QByteArrayLiteral("HOST: " SSDP_QUERY_IP ":1900\r\n") +
					QByteArrayLiteral("MAN: \"ssdp:discover\"\r\n") +
					QByteArrayLiteral("MX: 2\r\n") + QByteArrayLiteral("ST: ssdp:all\r\n\r\n");
	const qint64 bytesSent = socket4->writeDatagram(datagramData, groupAddress4, SSDP_PORT);
	if (bytesSent == -1) {
		qCritical() << "Failed to send datagram:" << socket4->errorString();
	} else {
		qDebug() << datagramData;
		qDebug() << bytesSent << "bytes to" << groupAddress4.toString() << ":" << SSDP_PORT;
	}
}

void DeviceSearchDialog::updateDevice(const Device &device)
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
	devices.insert(device.id, device);

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

} // namespace xbotgo
