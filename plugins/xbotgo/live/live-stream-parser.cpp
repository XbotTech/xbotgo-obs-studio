#include "live-stream-parser.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrl>

namespace xbotgo {

namespace {

bool SplitUrl(const QString &value, QString &server, QString &streamKey, QString &error)
{
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
}

QString JoinUrl(const QString &server, const QString &streamKey)
{
	return server.endsWith(QLatin1Char('/')) ? server + streamKey : server + QLatin1Char('/') + streamKey;
}

} // namespace

QString LiveStreamConfig::pushUrl() const
{
	return JoinUrl(pushServer, pushStreamKey);
}

QString LiveStreamConfig::pullUrl() const
{
	return JoinUrl(pullServer, pullStreamKey);
}

std::optional<LiveStreamConfig> ParseLiveStreamUrls(const QString &pushUrl, const QString &pullUrl, QString &error)
{
	error.clear();
	LiveStreamConfig config;
	if (!SplitUrl(pushUrl, config.pushServer, config.pushStreamKey, error) ||
	    !SplitUrl(pullUrl, config.pullServer, config.pullStreamKey, error)) {
		return std::nullopt;
	}
	return config;
}

std::optional<LiveStreamConfig> ParseLiveStreamResponse(const QByteArray &response, QString &error)
{
	error.clear();
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(response, &parseError);
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

	auto config = ParseLiveStreamUrls(pushUrl, pullUrl, error);
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

} // namespace xbotgo
