#pragma once

#include <QDialog>
#include <obs.h>

class QLabel;
class QTimer;

class OBSBasicFalconMControl : public QDialog {
	Q_OBJECT
	obs_source_t *source = nullptr;
	QLabel *connection = nullptr;
	QLabel *angles = nullptr;
	QTimer *poller = nullptr;

public:
	explicit OBSBasicFalconMControl(obs_source_t *source, QWidget *parent = nullptr);
	~OBSBasicFalconMControl() override;

private:
	void Send(int direction, int operation);
	void Refresh();
};
