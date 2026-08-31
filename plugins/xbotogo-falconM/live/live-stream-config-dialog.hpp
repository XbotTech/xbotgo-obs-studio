#pragma once

#include "live-stream-config.hpp"

#include <QDialog>

class QLineEdit;

namespace xbotgo {

class LiveStreamConfigDialog final : public QDialog {
	Q_OBJECT

public:
	explicit LiveStreamConfigDialog(const LiveStreamConfig &config, QWidget *parent = nullptr);
	LiveStreamConfig liveStreamConfig() const;

public slots:
	void accept() override;

private:
	QLineEdit *pushServerEdit_ = nullptr;
	QLineEdit *pushStreamKeyEdit_ = nullptr;
	QLineEdit *pullUrlEdit_ = nullptr;
	QString pullServer_;
	QString pullStreamKey_;
	QString taskId_;
};

} // namespace xbotgo
