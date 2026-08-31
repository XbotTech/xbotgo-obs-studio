#pragma once

#include "device-info.hpp"

#include <QDialog>
#include <QHash>
#include <QNetworkInterface>
#include <QUdpSocket>

#include <memory>
#include <optional>

class QHideEvent;
class QPushButton;
class QShowEvent;
class QStandardItemModel;
class QTableView;

namespace xbotgo {

class DeviceSearchDialog : public QDialog {
	Q_OBJECT

public:
	explicit DeviceSearchDialog(QWidget *parent);
	~DeviceSearchDialog() override;

	std::optional<DeviceInfo> selectedDevice() const;

private slots:
	void readPendingDatagrams();
	void refreshSearch();
	void updateSelection();

protected:
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

private:
	void initSocket();
	void uninitSocket();
	bool shouldJoinInterface(const QNetworkInterface &interface);

	void updateDevice(const DeviceInfo &device);
	void clearDevices();

	QTableView *deviceTable = nullptr;
	QStandardItemModel *deviceModel = nullptr;
	QPushButton *selectButton = nullptr;
	QUdpSocket socket4;
	QHostAddress group_address4;
	QHash<QString, int> deviceRows;
	QHash<QString, std::shared_ptr<const DeviceInfo>> devices;
	QList<QNetworkInterface> join_multicast_interfaces;
};

} // namespace xbotgo
