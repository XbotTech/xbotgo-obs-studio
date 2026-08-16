#pragma once

#include "../models/XBotGoDevice.hpp"

#include <QDialog>
#include <QHash>
#include <QNetworkInterface>
#include <QTimer>
#include <QUdpSocket>

class QHideEvent;
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
	void refreshSearch();
	void sendDatagram();

protected:
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

private:
	void startSearch();
	void stopSearch();
	void updateDevice(const Device &device);
	void clearDevices();

	QTableView *deviceTable = nullptr;
	QStandardItemModel *deviceModel = nullptr;
	QUdpSocket *socket4 = nullptr;
	QTimer timer;
	QHostAddress groupAddress4;
	QHash<QString, int> deviceRows;
	QList<QNetworkInterface> multicastInterfaces;
};

} // namespace XBotGo
