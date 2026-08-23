#pragma once

#include <QDialog>
#include <obs.h>
#include <QHash>
#include <QPointer>
#include <vector>

class QTableWidget;
class OBSBasicFalconMControl;

class OBSBasicFalconMDevices : public QDialog {
	Q_OBJECT

	QTableWidget *devices = nullptr;
	std::vector<obs_source_t *> sources;
	QHash<obs_source_t *, QPointer<OBSBasicFalconMControl>> controls;

public:
	explicit OBSBasicFalconMDevices(QWidget *parent = nullptr);
	~OBSBasicFalconMDevices() override;

private:
	void ReloadDevices();
	void OpenControl(int row, int column);
};
