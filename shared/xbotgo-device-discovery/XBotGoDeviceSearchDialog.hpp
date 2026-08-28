#pragma once

#include "XBotGoDevice.hpp"

#include <QDialog>
#include <QHash>
#include <QNetworkInterface>
#include <QTimer>
#include <QUdpSocket>

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
	enum class Mode {
		Browse,
		Select,
	};

	explicit DeviceSearchDialog(QWidget *parent, Mode mode = Mode::Browse);
	~DeviceSearchDialog() override;

	std::optional<Device> selectedDevice() const;

private slots:
	void readPendingDatagrams();
	void refreshSearch();
	void sendDatagram();
	void updateSelection();

protected:
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

private:
	void startSearch();
	void stopSearch();
	void updateDevice(const Device &device);
	void clearDevices();

	Mode mode;
	QTableView *deviceTable = nullptr;
	QStandardItemModel *deviceModel = nullptr;
	QPushButton *selectButton = nullptr;
	QUdpSocket *socket4 = nullptr;
	QTimer timer;
	QHostAddress groupAddress4;
	QHash<QString, int> deviceRows;
	QHash<QString, Device> devices;
	QList<QNetworkInterface> multicastInterfaces;
};

} // namespace xbotgo
