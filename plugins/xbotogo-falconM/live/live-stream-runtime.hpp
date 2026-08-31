#pragma once

#include "live-stream-session.hpp"
#include "live-task-client.hpp"

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <obs-frontend-api.h>

class QAction;
class QWidget;

namespace xbotgo {

class LiveStreamRuntime final : public QObject {
	Q_OBJECT

public:
	LiveStreamRuntime(QAction &startAction, QWidget &dialogParent, QObject *parent = nullptr);
	void start();
	void handleFrontendEvent(obs_frontend_event event);
	void shutdown();

private:
	void setActionForPhase();
	void sendHeartbeat();
	void stopTaskAndReset();
	bool installService(const LiveStreamConfig &config);

	QPointer<QAction> startAction_;
	QPointer<QWidget> dialogParent_;
	LiveTaskClient client_;
	LiveStreamSession session_;
	QTimer heartbeatTimer_;
	QString heartbeatTaskId_;
	bool heartbeatInFlight_ = false;
	bool shutdown_ = false;
};

} // namespace xbotgo
