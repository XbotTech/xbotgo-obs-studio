#include "live-task-client.hpp"

#include "live-stream-parser.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaObject>

#include <obs.h>

#include <algorithm>
#include <utility>

namespace xbotgo {

namespace {

constexpr auto LiveStartUrl = "https://test-cn-cloud.xbotgo.net/api/live/api/system/room/task/start";
constexpr auto HeartbeatUrl = "https://test-cn-cloud.xbotgo.net/api/live/api/system/room/task/heartbeat";
constexpr auto LiveStopUrl = "https://test-cn-cloud.xbotgo.net/api/live/api/system/room/task/stop";
constexpr int RequestTimeoutMs = 10'000;

QString CompletionError(const QByteArray &response)
{
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(response, &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
		return QStringLiteral("Invalid server response: %1").arg(parseError.errorString());
	}

	const QJsonObject root = document.object();
	const QJsonValue code = root.value(QStringLiteral("code"));
	if (code.isDouble() && code.toInt() == 200) {
		return {};
	}

	const QString message = root.value(QStringLiteral("msg")).toString();
	return message.isEmpty() ? QStringLiteral("The server returned an unsuccessful response.") : message;
}

QByteArray TaskBody(const QString &taskId)
{
	return QJsonDocument(QJsonObject{{QStringLiteral("taskId"), taskId}}).toJson(QJsonDocument::Compact);
}

} // namespace

LiveTaskClient::LiveTaskClient(QObject *parent)
	: QObject(parent),
	  worker_([this](CurlPostResult result) {
		  QMetaObject::invokeMethod(
			  this, [this, result = std::move(result)]() mutable { complete(std::move(result)); },
			  Qt::QueuedConnection);
	  })
{
}

LiveTaskClient::~LiveTaskClient()
{
	pending_.clear();
	worker_.shutdown();
}

void LiveTaskClient::requestStart(QObject *context, const QString &liveTitle, StartCallback callback)
{
	const QString normalizedTitle = liveTitle.trimmed();
	if (!context || !callback || normalizedTitle.isEmpty()) {
		return;
	}

	const QJsonObject requestBody{
		{QStringLiteral("videoModel"), QStringLiteral("1")},
		{QStringLiteral("liveType"), 8},
		{QStringLiteral("liveTitle"), normalizedTitle},
		{QStringLiteral("coverPath"), QString{}},
		{QStringLiteral("livePassword"), QString{}},
		{QStringLiteral("extra"), QJsonObject{}},
		{QStringLiteral("liveDesc"), QString{}},
		{QStringLiteral("livePrivacyStatus"), 1},
	};

	post(LiveStartUrl, QJsonDocument(requestBody).toJson(QJsonDocument::Compact), context,
	     [callback = std::move(callback)](CurlPostResult result) mutable {
		     if (result.transportCode != 0) {
			     blog(LOG_WARNING, "XBotGo live start request failed (%d)", result.transportCode);
			     callback(std::nullopt, std::move(result.error));
			     return;
		     }

		     QString error;
		     auto config = ParseLiveStreamResponse(result.body, error);
		     callback(std::move(config), std::move(error));
	     });
}

void LiveTaskClient::requestHeartbeat(QObject *context, const QString &taskId, Completion callback)
{
	if (!context || taskId.isEmpty()) {
		return;
	}

	post(HeartbeatUrl, TaskBody(taskId), context, [callback = std::move(callback)](CurlPostResult result) mutable {
		QString error;
		if (result.transportCode != 0) {
			blog(LOG_WARNING, "XBotGo heartbeat request failed (%d)", result.transportCode);
			error = std::move(result.error);
		} else {
			error = CompletionError(result.body);
			if (!error.isEmpty()) {
				blog(LOG_WARNING, "XBotGo heartbeat request was rejected");
			}
		}
		if (callback) {
			callback(std::move(error));
		}
	});
}

void LiveTaskClient::requestStop(QObject *context, QString taskId, Completion callback)
{
	if (!context || taskId.isEmpty()) {
		return;
	}

	post(LiveStopUrl, TaskBody(taskId), context, [callback = std::move(callback)](CurlPostResult result) mutable {
		QString error;
		if (result.transportCode != 0) {
			blog(LOG_WARNING, "XBotGo stop request failed (%d)", result.transportCode);
			error = std::move(result.error);
		} else {
			error = CompletionError(result.body);
			if (!error.isEmpty()) {
				blog(LOG_WARNING, "XBotGo stop request was rejected");
			}
		}
		if (callback) {
			callback(std::move(error));
		}
	});
}

void LiveTaskClient::abortAll()
{
	uint64_t cancelThrough = 0;
	for (auto it = pending_.cbegin(); it != pending_.cend(); ++it) {
		cancelThrough = std::max(cancelThrough, it.key());
	}
	pending_.clear();
	worker_.cancelThrough(cancelThrough);
}

void LiveTaskClient::post(const char *url, QByteArray body, QObject *context, ResultCallback callback)
{
	const uint64_t requestId = worker_.post(QByteArray(url), std::move(body), RequestTimeoutMs);
	if (requestId != 0) {
		pending_.insert(requestId, {context, std::move(callback)});
	}
}

void LiveTaskClient::complete(CurlPostResult result)
{
	auto it = pending_.find(result.requestId);
	if (it == pending_.end()) {
		return;
	}
	PendingRequest request = std::move(it.value());
	pending_.erase(it);
	if (request.context && request.callback && !result.cancelled) {
		request.callback(std::move(result));
	}
}

} // namespace xbotgo
