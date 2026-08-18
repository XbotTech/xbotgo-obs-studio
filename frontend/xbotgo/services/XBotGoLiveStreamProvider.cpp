#include "XBotGoLiveStreamProvider.hpp"

#include <utility/RemoteTextThread.hpp>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTimer>
#include <QUrl>

#include <obs.h>

#include <utility>
#include <vector>

namespace XBotGo {

namespace {

constexpr auto liveStartUrl = "https://test-cn-cloud.xbotgo.net/api/live/api/system/room/task/start";
constexpr auto heartbeatUrl = "https://test-cn-cloud.xbotgo.net/api/live/api/system/room/task/heartbeat";
constexpr auto liveStopUrl = "https://test-cn-cloud.xbotgo.net/api/live/api/system/room/task/stop";
constexpr int heartbeatIntervalMs = 10'000;

std::optional<LiveStreamConfig> parseLiveStreamUrls(const QString &pushUrl, const QString &pullUrl, QString &error)
{
	auto splitUrl = [&error](const QString &value, QString &server, QString &streamKey) {
		const QUrl url(value, QUrl::StrictMode);
		const QString scheme = url.scheme().toLower();
		const QString path = url.path(QUrl::FullyEncoded);
		const qsizetype lastSlash = path.lastIndexOf(QLatin1Char('/'));

		if (!url.isValid() || (scheme != QStringLiteral("rtmp") && scheme != QStringLiteral("rtmps")) ||
		    url.host().isEmpty() || lastSlash < 0 || lastSlash == path.size() - 1) {
			error = QStringLiteral("The server returned an invalid RTMP URL: %1").arg(value);
			return false;
		}

		QUrl serverUrl = url;
		serverUrl.setPath(path.left(lastSlash), QUrl::StrictMode);
		serverUrl.setQuery({});
		serverUrl.setFragment({});
		server = serverUrl.toString(QUrl::FullyEncoded);
		streamKey = path.mid(lastSlash + 1);
		if (url.hasQuery()) {
			streamKey += QLatin1Char('?') + url.query(QUrl::FullyEncoded);
		}
		return true;
	};

	LiveStreamConfig config;
	if (!splitUrl(pushUrl, config.pushServer, config.pushStreamKey) ||
	    !splitUrl(pullUrl, config.pullServer, config.pullStreamKey)) {
		return std::nullopt;
	}

	return config;
}

std::optional<LiveStreamConfig> parseLiveStreamResponse(const std::string &response, QString &error)
{
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(response), &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
		error = QStringLiteral("Invalid server response: %1").arg(parseError.errorString());
		return std::nullopt;
	}

	const QJsonObject root = document.object();
	const QJsonValue code = root.value(QStringLiteral("code"));
	if (!code.isDouble() || code.toInt() != 200) {
		error = root.value(QStringLiteral("msg")).toString();
		if (error.isEmpty()) {
			error = QStringLiteral("The server returned an unsuccessful response.");
		}
		return std::nullopt;
	}

	const QJsonValue dataValue = root.value(QStringLiteral("data"));
	if (!dataValue.isObject()) {
		error = QStringLiteral("The server response does not contain live stream data.");
		return std::nullopt;
	}

	const QJsonObject data = dataValue.toObject();
	const QString pushUrl = data.value(QStringLiteral("livePushUrl")).toString();
	const QString playUrlJson = data.value(QStringLiteral("livePlayUrl")).toString();
	if (pushUrl.isEmpty() || playUrlJson.isEmpty()) {
		error = QStringLiteral("The server response is missing the push or play URL.");
		return std::nullopt;
	}

	QJsonParseError playUrlParseError;
	const QJsonDocument playUrlDocument = QJsonDocument::fromJson(playUrlJson.toUtf8(), &playUrlParseError);
	if (playUrlParseError.error != QJsonParseError::NoError || !playUrlDocument.isObject()) {
		error = QStringLiteral("Invalid livePlayUrl: %1").arg(playUrlParseError.errorString());
		return std::nullopt;
	}

	const QString pullUrl = playUrlDocument.object().value(QStringLiteral("rtmpPlayUrl")).toString();
	if (pullUrl.isEmpty()) {
		error = QStringLiteral("The server response is missing the RTMP play URL.");
		return std::nullopt;
	}

	auto config = parseLiveStreamUrls(pushUrl, pullUrl, error);
	if (!config) {
		return std::nullopt;
	}

	config->taskId = data.value(QStringLiteral("taskId")).toString();
	if (config->taskId.isEmpty()) {
		error = QStringLiteral("The server response is missing the task ID.");
		return std::nullopt;
	}

	return config;
}

} // namespace

HttpLiveStreamProvider::~HttpLiveStreamProvider()
{
	stopHeartbeat();
}

void HttpLiveStreamProvider::requestLiveStreamConfig(QObject *context, LiveStreamConfigCallback callback)
{
	const QJsonObject requestBody{
		{QStringLiteral("videoModel"), QStringLiteral("1")},
		{QStringLiteral("liveType"), 8},
		{QStringLiteral("liveTitle"), QStringLiteral("多机位直播内测-陆伟")},
		{QStringLiteral("coverPath"), QString{}},
		{QStringLiteral("livePassword"), QString{}},
		{QStringLiteral("extra"), QJsonObject{}},
		{QStringLiteral("liveDesc"), QString{}},
		{QStringLiteral("livePrivacyStatus"), 1},
	};
	const std::string postData = QJsonDocument(requestBody).toJson(QJsonDocument::Compact).toStdString();
	std::vector<std::string> headers{
		"DATA-REGION: CN",
		"BLINK-APP-LANG: en_US",
		"Accept: */*",
	};

	auto *thread = new RemoteTextThread(liveStartUrl, std::move(headers), "application/json", postData, 10);
	QObject::connect(thread, &RemoteTextThread::Result, context,
			 [callback = std::move(callback)](const std::string &response,
							  const std::string &networkError) {
				 if (!networkError.empty()) {
					 callback(std::nullopt, QString::fromStdString(networkError));
					 return;
				 }

				 QString error;
				 auto config = parseLiveStreamResponse(response, error);
				 callback(std::move(config), error);
			 });
	QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	thread->start();
}

void HttpLiveStreamProvider::startHeartbeat(QObject *context, const QString &taskId)
{
	stopHeartbeat();
	if (!context || taskId.isEmpty()) {
		return;
	}

	heartbeatContext = context;
	activeTaskId = taskId;
	heartbeatTimer = new QTimer(context);
	heartbeatTimer->setInterval(heartbeatIntervalMs);
	QObject::connect(heartbeatTimer, &QTimer::timeout, context, [this] { sendHeartbeat(); });
	heartbeatTimer->start();
}

void HttpLiveStreamProvider::stopHeartbeat()
{
	if (heartbeatTimer) {
		heartbeatTimer->stop();
		heartbeatTimer->deleteLater();
	}
	heartbeatTimer = nullptr;
	heartbeatContext = nullptr;
	heartbeatInFlight = false;
}

void HttpLiveStreamProvider::sendHeartbeat()
{
	if (!heartbeatContext || activeTaskId.isEmpty() || heartbeatInFlight) {
		return;
	}

	const QString taskId = activeTaskId;
	const QJsonObject requestBody{{QStringLiteral("taskId"), taskId}};
	const std::string postData = QJsonDocument(requestBody).toJson(QJsonDocument::Compact).toStdString();
	std::vector<std::string> headers{
		"DATA-REGION: CN",
		"BLINK-APP-LANG: en_US",
		"Accept: */*",
	};

	heartbeatInFlight = true;
	blog(LOG_INFO, "XBotGo heartbeat request: %s", postData.c_str());
	auto *thread = new RemoteTextThread(heartbeatUrl, std::move(headers), "application/json", postData, 10);
	QObject::connect(thread, &RemoteTextThread::Result, heartbeatContext,
			 [this, taskId](const std::string &response, const std::string &networkError) {
				 if (taskId == activeTaskId) {
					 heartbeatInFlight = false;
				 }
				 if (!networkError.empty()) {
					 blog(LOG_WARNING, "XBotGo heartbeat request failed: %s", networkError.c_str());
					 return;
				 }
				 blog(LOG_INFO, "XBotGo heartbeat response: %s", response.c_str());

				 QJsonParseError parseError;
				 const QJsonDocument document =
					 QJsonDocument::fromJson(QByteArray::fromStdString(response), &parseError);
				 if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
					 blog(LOG_WARNING, "XBotGo heartbeat returned invalid JSON: %s",
					      parseError.errorString().toUtf8().constData());
					 return;
				 }

				 const QJsonObject root = document.object();
				 const QJsonValue code = root.value(QStringLiteral("code"));
				 if (!code.isDouble() || code.toInt() != 200) {
					 const QByteArray message =
						 root.value(QStringLiteral("msg")).toString().toUtf8();
					 blog(LOG_WARNING, "XBotGo heartbeat was rejected: %s",
					      message.isEmpty() ? "unknown server error" : message.constData());
				 }
			 });
	QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	thread->start();
}

void HttpLiveStreamProvider::stopLiveTask(QObject *context)
{
	stopHeartbeat();
	if (!context || activeTaskId.isEmpty()) {
		return;
	}

	const QString taskId = std::exchange(activeTaskId, QString{});
	const QJsonObject requestBody{{QStringLiteral("taskId"), taskId}};
	const std::string postData = QJsonDocument(requestBody).toJson(QJsonDocument::Compact).toStdString();
	std::vector<std::string> headers{
		"DATA-REGION: CN",
		"BLINK-APP-LANG: en_US",
		"Accept: */*",
	};

	blog(LOG_INFO, "XBotGo stop live task request: %s", postData.c_str());
	auto *thread = new RemoteTextThread(liveStopUrl, std::move(headers), "application/json", postData, 10);
	QObject::connect(
		thread, &RemoteTextThread::Result, context,
		[](const std::string &response, const std::string &networkError) {
			if (!networkError.empty()) {
				blog(LOG_WARNING, "XBotGo stop live task request failed: %s", networkError.c_str());
				return;
			}
			blog(LOG_INFO, "XBotGo stop live task response: %s", response.c_str());

			QJsonParseError parseError;
			const QJsonDocument document =
				QJsonDocument::fromJson(QByteArray::fromStdString(response), &parseError);
			if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
				blog(LOG_WARNING, "XBotGo stop live task returned invalid JSON: %s",
				     parseError.errorString().toUtf8().constData());
				return;
			}

			const QJsonObject root = document.object();
			const QJsonValue code = root.value(QStringLiteral("code"));
			if (!code.isDouble() || code.toInt() != 200) {
				const QByteArray message = root.value(QStringLiteral("msg")).toString().toUtf8();
				blog(LOG_WARNING, "XBotGo stop live task was rejected: %s",
				     message.isEmpty() ? "unknown server error" : message.constData());
			}
		});
	QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	thread->start();
}

} // namespace XBotGo
