#pragma once

#include <QDialog>

class QTableWidget;

class OBSBasicFalconMDevices : public QDialog {
	Q_OBJECT

	QTableWidget *devices = nullptr;

public:
	explicit OBSBasicFalconMDevices(QWidget *parent = nullptr);

private:
	void ReloadDevices();
};
