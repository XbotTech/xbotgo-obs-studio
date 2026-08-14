#pragma once

#include "../models/XBotGoDevice.hpp"

#include <QDialog>
#include <QHash>
#include <QNetworkInterface>
#include <QTimer>
#include <QUdpSocket>

class QPushButton;
class QShowEvent;
class QStandardItemModel;
class QTableView;

namespace XBotGo {

class DeviceSearchDialog : public QDialog {
	Q_OBJECT

public:
	explicit DeviceSearchDialog(QWidget *parent);
	~DeviceSearchDialog() override;

private slots:
	void readPendingDatagrams();
	void startSending();
	void stopSending();
	void sendDatagram();

protected:
	void showEvent(QShowEvent *event) override;

private:
	void updateDevice(const Device &device);
	void clearDevices();

	QTableView *deviceTable = nullptr;
	QStandardItemModel *deviceModel = nullptr;
	QPushButton *startButton = nullptr;
	QUdpSocket *socket4 = nullptr;
	QTimer timer;
	QHostAddress groupAddress4;
	QHash<QString, int> deviceRows;
	QList<QNetworkInterface> multicastInterfaces;
};

} // namespace XBotGo
