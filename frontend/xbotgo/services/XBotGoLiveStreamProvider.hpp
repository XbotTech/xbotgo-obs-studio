#pragma once

#include "../models/XBotGoLiveStreamConfig.hpp"

#include <functional>
#include <optional>

#include <QPointer>

class QObject;
class QTimer;

namespace xbotgo {

using LiveStreamConfigCallback = std::function<void(std::optional<LiveStreamConfig> config, const QString &error)>;

class LiveStreamProvider {
public:
	virtual ~LiveStreamProvider() = default;

	virtual void requestLiveStreamConfig(QObject *context, LiveStreamConfigCallback callback) = 0;
	virtual void startHeartbeat(QObject *context, const QString &taskId) = 0;
	virtual void stopHeartbeat() = 0;
	virtual void stopLiveTask(QObject *context) = 0;
};

class HttpLiveStreamProvider final : public LiveStreamProvider {
public:
	~HttpLiveStreamProvider() override;

	void requestLiveStreamConfig(QObject *context, LiveStreamConfigCallback callback) override;
	void startHeartbeat(QObject *context, const QString &taskId) override;
	void stopHeartbeat() override;
	void stopLiveTask(QObject *context) override;

private:
	void sendHeartbeat();

	QPointer<QObject> heartbeatContext;
	QPointer<QTimer> heartbeatTimer;
	QString activeTaskId;
	bool heartbeatInFlight = false;
};

} // namespace xbotgo
