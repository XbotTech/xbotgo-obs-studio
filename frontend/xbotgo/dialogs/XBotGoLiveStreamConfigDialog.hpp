#pragma once

#include "../models/XBotGoLiveStreamConfig.hpp"

#include <QDialog>

class QLineEdit;

namespace XBotGo {

class LiveStreamConfigDialog final : public QDialog {
	Q_OBJECT

public:
	explicit LiveStreamConfigDialog(const LiveStreamConfig &config, QWidget *parent = nullptr);

	LiveStreamConfig liveStreamConfig() const;

public slots:
	void accept() override;

private:
	QLineEdit *pushServerEdit = nullptr;
	QLineEdit *pushStreamKeyEdit = nullptr;
	QLineEdit *pullUrlEdit = nullptr;
	QString pullServer;
	QString pullStreamKey;
	QString taskId;
};

} // namespace XBotGo
