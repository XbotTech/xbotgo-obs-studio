#pragma once

#include "curl-request-worker.hpp"
#include "live-stream-config.hpp"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

#include <functional>
#include <optional>

namespace xbotgo {

class LiveTaskClient final : public QObject {
	Q_OBJECT

public:
	using StartCallback = std::function<void(std::optional<LiveStreamConfig>, QString)>;
	using Completion = std::function<void(QString)>;

	explicit LiveTaskClient(QObject *parent = nullptr);
	~LiveTaskClient() override;
	void requestStart(QObject *context, const QString &liveTitle, StartCallback callback);
	void requestHeartbeat(QObject *context, const QString &taskId, Completion callback);
	void requestStop(QObject *context, QString taskId, Completion callback = {});
	void abortAll();

private:
	using ResultCallback = std::function<void(CurlPostResult)>;

	struct PendingRequest {
		QPointer<QObject> context;
		ResultCallback callback;
	};

	void post(const char *url, QByteArray body, QObject *context, ResultCallback callback);
	void complete(CurlPostResult result);

	CurlRequestWorker worker_;
	QHash<uint64_t, PendingRequest> pending_;
};

} // namespace xbotgo
